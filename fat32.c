//
// fat32.c
//

#include <ultra64.h>
#include <math.h>

#include "fat32.h"
#include "ci.h"


unsigned char	buffer[512];
unsigned char	buffer_2[512];
unsigned char	fat_message[64];

unsigned char	fat_buffer[512];
uint32_t		fat_buffer_sector = -1;
int				fat_buffer_dirty = 0;

uint32_t		dir_buffer_sector = 0;
int				dir_buffer_dirty = 0;

uint32_t		fs_begin_sector;

typedef struct _fat_fs_t {
    uint32_t begin_sector;
    uint32_t sect_per_clus;
    uint32_t num_fats;
    uint32_t sect_per_fat;
    uint32_t root_cluster;
    uint32_t clus_begin_sector;
    uint32_t total_clusters;
    uint32_t free_clusters;
} fat_fs_t;

fat_fs_t		fat_fs;






/// 2-byte number
uint16_t shortEndian(unsigned char *i)
{
    int t = ((int)*(i+1)<<8) |
            ((int)*(i+0));
    return t;
}

void writeShort(unsigned char *dest, uint16_t val) {
    *(uint16_t *)dest = shortEndian((unsigned char *)&val);
}

// 4-byte number
uint32_t intEndian(unsigned char *i)
{
    uint32_t t = ((int)*(i+3)<<24) |
            ((int)*(i+2)<<16) |
            ((int)*(i+1)<<8) |
            ((int)*(i+0));

    return t;
}

void writeInt(unsigned char *dest, uint32_t val) {
    *(uint32_t *)dest = intEndian((unsigned char *)&val);
}




/************************
 * CONSTANTS AND MACROS *
 ************************/

// first sector of a cluster
#define CLUSTER_TO_SECTOR(X) ( fat_fs.clus_begin_sector + (X - 2) * fat_fs.sect_per_clus )

// dirents per sector
#define DE_PER_SECTOR (512 / 32)




uint32_t fat_cluster_to_sector(uint32_t clus)
{
	return CLUSTER_TO_SECTOR(clus);
}

uint32_t fat_sect_per_clus()
{
	return fat_fs.sect_per_clus;
}

int fat_init()
{
    char fat_systemid[8];
    uint32_t fat_num_resv_sect;
    uint32_t total_sectors, data_offset;

    // read first sector
    ciReadSector(buffer, 0);

    // check for MBR/VBR magic
    if( !(buffer[0x1fe]==0x55 && buffer[0x1ff]==0xAA) ){
        sprintf(fat_message, "No memory card / bad FS");
        return 1;
    }

	/*
    // now speed things up!
    if(compat_mode)
    {
        cfSetCycleTime(30);
    }else{
        cfOptimizeCycleTime();
    }
	*/


    // look for 'FAT'
    if(strncmp((char *)&buffer[82], "FAT", 3) == 0)
    {
        // this first sector is a Volume Boot Record
        fs_begin_sector = 0;
    }else{
        // this is a MBR. Read first entry from partition table
        fs_begin_sector = intEndian(&buffer[0x1c6]);
    }

    ciReadSector(buffer, fs_begin_sector);

    // copy the system ID string
    memcpy(fat_systemid, &buffer[82], 8);
    fat_systemid[7] = 0;

    if(strncmp(fat_systemid, "FAT32", 5) != 0){
        // not a fat32 volume
        sprintf(fat_message, "FAT32 partition not found.");
        return 1;
    }

    fat_fs.sect_per_clus = buffer[0x0d];
    fat_num_resv_sect = shortEndian(&buffer[0x0e]);
    fat_fs.num_fats = buffer[0x10];
    fat_fs.sect_per_fat = intEndian(&buffer[0x24]);
    fat_fs.root_cluster = intEndian(&buffer[0x2c]);

    fat_fs.begin_sector = fs_begin_sector + fat_num_resv_sect;
    data_offset = fat_num_resv_sect + (fat_fs.num_fats * fat_fs.sect_per_fat);
    fat_fs.clus_begin_sector = fs_begin_sector + data_offset;

    total_sectors = intEndian(&buffer[0x20]);
    fat_fs.total_clusters = (total_sectors - data_offset) / fat_fs.sect_per_clus;

    //
    // Load free cluster count
    //
    ciReadSector(buffer, fs_begin_sector + 1);
    fat_fs.free_clusters = intEndian(&buffer[0x1e8]);

    return 0;
}





/**
 * Get the relative sector # and offset into the sector for a given cluster.
 */
void fat_sector_offset(uint32_t cluster, uint32_t *fat_sector, uint32_t *fat_offset) {
    uint32_t index = cluster * 4;   // each cluster is 4 bytes long
    uint32_t sector = index / 512;  // 512 bytes per sector, rounds down

    *fat_sector = sector;
    *fat_offset = index - sector * 512;
}

/**
 * Get the absolute number of a relative fat sector for a given fat.
 */
static uint32_t _fat_absolute_sector(uint32_t relative_sector, int fat_num) {
    return fat_fs.begin_sector + (fat_num * fat_fs.sect_per_fat) + relative_sector;
}

// flush changes to the fat
static void _fat_flush_fat(void) {
    uint32_t sector, i;
    unsigned char fs_info[512];
    uint32_t old_free;

    if (fat_buffer_dirty) {
        // write the dirty sector to each copy of the FAT
        for (i = 0; i < fat_fs.num_fats; ++i) {
            sector = _fat_absolute_sector(fat_buffer_sector, i);
            ciWriteSector(fat_buffer, sector);
        }

        fat_buffer_dirty = 0;
    }

    // 
    // Write free cluster count
    //
    ciReadSector(fs_info, fs_begin_sector + 1);
    old_free = intEndian(&fs_info[0x1e8]);
    if (old_free != fat_fs.free_clusters) {
        writeInt(&fs_info[0x1e8], fat_fs.free_clusters);
        ciWriteSector(fs_info, fs_begin_sector + 1);
    }
}

/**
 * Load the sector for a FAT cluster, return offset into sector.
 */
uint32_t _fat_load_fat(uint32_t cluster) {
    uint32_t relative_sector, offset;

    // get the sector of the FAT and offset into the sector
    fat_sector_offset(cluster, &relative_sector, &offset);

    // only read sector if it has changed! saves time
    if (relative_sector != fat_buffer_sector) {
        // flush pending writes
        _fat_flush_fat();

        // read the sector
        ciReadSector(fat_buffer, _fat_absolute_sector(relative_sector, 0));
        fat_buffer_sector = relative_sector;
    }

    return offset;
}


/**
 * Get the FAT entry for a given cluster.
 */
uint32_t fat_get_fat(uint32_t cluster) {
    uint32_t offset = _fat_load_fat(cluster);
    return intEndian(&fat_buffer[offset]);
}

/**
 * Set the FAT entry for a given cluster.
 */
void fat_set_fat(uint32_t cluster, uint32_t value) {
    uint32_t offset = _fat_load_fat(cluster);
    writeInt(&fat_buffer[offset], value);

    fat_buffer_dirty = 1;
}

/**
 * Find the first unused entry in the FAT.
 *
 * Returns:
 *  FAT_SUCCESS on success, with new entry in new_entry
 *  FAT_NOSPACE if it can't find an unused cluster
 */
static int _fat_find_free_entry(uint32_t start, uint32_t *new_entry) {
    uint32_t entry = 1;
    uint32_t num_entries = fat_fs.total_clusters + 2; // 2 unused entries at the start of the FAT

    if (start > 0)
        entry = start;

    while (entry < num_entries && fat_get_fat(entry) != 0)
        ++entry;

    // if we reach the end, loop back to the beginning and try to find an unused entry
    if (entry == num_entries) {
        entry = 1;
        while (entry < start && fat_get_fat(entry) != 0)
            ++entry;
        if (entry == start)
            return FAT_NOSPACE;
    }

    *new_entry = entry;
    return FAT_SUCCESS;
}

/**
 * Get the root directory entry.
 */
int fat_root_dirent(fat_dirent *dirent) {
    dirent->short_name[0] = 0;
    dirent->long_name[0] = 0;
    dirent->directory = 0;
    dirent->start_cluster = 0;
    dirent->size = 0;

    dirent->index = 0;
    dirent->cluster = fat_fs.root_cluster;
    dirent->sector = 0;
    dirent->first_cluster = fat_fs.root_cluster;

    return 0;
}

/*
 * Returns a dirent for the directory starting at start_cluster.
 */
void fat_sub_dirent(uint32_t start_cluster, fat_dirent *de) {
    fat_root_dirent(de);
    de->cluster = de->first_cluster = start_cluster;
}

/**
 * Flush pending changes to a directory.
 */
static void _fat_flush_dir(void) {
    if (dir_buffer_dirty) {
        ciWriteSector(buffer, dir_buffer_sector);
        dir_buffer_dirty = 0;
    }
}

/**
 * Read a sector from a directory. Automatically handles buffering and flushing
 * changes to dirty buffers.
 */
static void _dir_read_sector(uint32_t sector) {
    if (sector != dir_buffer_sector) {
        // flush pending writes
        _fat_flush_dir();

        ciReadSector(buffer, sector);
        dir_buffer_sector = sector;
    }
}

/**
 * Load the current sector pointed to by the dirent. Automatically loads new
 * sectors and clusters.
 * Returns 0 if next cluster is 0x0ffffff8.
 * Returns 1 on success.
 */
static int _fat_load_dir_sector(fat_dirent *dirent) {
    uint32_t sector;
    uint32_t fat_entry;

    if (dirent->index == DE_PER_SECTOR) {

        // load the next cluster once we reach the end of this
        if (dirent->sector + 1 == fat_fs.sect_per_clus) {
            // look up the cluster number in the FAT
            fat_entry = fat_get_fat(dirent->cluster);
            if (fat_entry >= 0x0ffffff8) // last cluster
                return 0; // end of dir

            dirent->cluster = fat_entry;
            dirent->sector = 0;
			dirent->index = 0;

        }else {
			++dirent->sector;
			dirent->index = 0;
		}
    }

    // sector may or may not have changed, but buffering makes this efficient
    sector = CLUSTER_TO_SECTOR(dirent->cluster) + dirent->sector;
	////////////////////////////////////////////////////////////////////////////////////
	dirent->phy_sector = sector;
	////////////////////////////////////////////////////////////////////////////////////
    _dir_read_sector(sector);

    return 1;
}

//////////////////////////////////////////////////////////
/**
 * Delete a file, given its directory entry.
 * returns:
 *   1  success
 *   -1 error (usually INCONSISTENT)
 */
 int fat_deletefile(fat_dirent *dirent) {
	int ret, i;
	lfn_entry_t *lfn;
	
	// zero out the file's fat chain
	ret = fat_set_size(dirent, 0);
	if(ret != FAT_SUCCESS){
		return -1;
	}
	// mark out all of its directory entries
	for(i = 0; i < dirent->num_lfn_entries; i++){
		if(i > MAX_LFN){
			// unpossible!
			return -1;
		}
		lfn = &dirent->lfn[i];
		_dir_read_sector(lfn->sector);
		buffer[lfn->offset] = 0xE5;	
		dir_buffer_dirty = 1;
	}
	_fat_flush_dir();
	return 1; 
 }
 //////////////////////////////////////////////////////////
 
/**
 * Read a directory.
 * returns:
 *   1  success
 *   0  end of dir
 *  -1  error
 */
int fat_readdir(fat_dirent *dirent) {
    int found_file = 0;
    int ret, i, j;

    uint32_t offset;
    uint32_t attributes;
    uint32_t segment;

    char *dest;
	//////////////////////////////////////////////////////////
	lfn_entry_t *lfn;
	//////////////////////////////////////////////////////////

    if (dirent->index > DE_PER_SECTOR) {
        sprintf(fat_message, "Invalid directory");
        return -1;
    }
	//////////////////////////////////////////////////////////
	dirent->num_lfn_entries = 0;
	//////////////////////////////////////////////////////////
	
    if (dirent->long_name)
        memset(dirent->long_name, 0, 256);

    do {
        ret = _fat_load_dir_sector(dirent);
        if (ret == 0) // end of dir
            return 0;

        offset = dirent->index * 32;

        // end of directory reached
        if (buffer[offset] == 0)
            return 0;

		++dirent->index;
		
        // deleted file, skip
        if (buffer[offset] == 0xe5)
            continue;

        attributes = buffer[offset + 0x0b];

        // long filename, copy the bytes and move along
        if (attributes == 0x0f) {
            segment = (buffer[offset] & 0x1F) - 1;
            if (segment < 0 || segment > 19)
                continue; // invalid segment

            dest = (dirent->long_name + segment * 13);

            for (i = 0; i < 5; ++i)
                dest[i] = buffer[offset + 1 + i * 2];

            for (j = 0; j < 3; ++j)
                dest[i+j] = buffer[offset + 0xe + j * 2];

            // last segment can only have 9 characters
            if (segment == 19) {
                dest[i+j] = 0;
                continue;
            }

            for ( ; j < 6; ++j)
                dest[i+j] = buffer[offset + 0xe + j * 2];

            i += j;

            for (j = 0; j < 2; ++j)
                dest[i+j] = buffer[offset + 0x1c + j * 2];

			//////////////////////////////////////////////////////////
			// add a lfn entry and set up the values
			lfn = &dirent->lfn[dirent->num_lfn_entries++];
			lfn->offset = offset;
			lfn->sector = dirent->phy_sector;
			lfn->cluster = dirent->cluster;
			//////////////////////////////////////////////////////////
            continue;
        }
		
		
        dirent->directory = attributes & 0x10 ? 1 : 0;
		dirent->volume_label = attributes & 0x08 ? 1 : 0;
		dirent->hidden = attributes & 0x02 ? 1 : 0;

        // you can thank FAT16 for this
        dirent->start_cluster = shortEndian(buffer + offset + 0x14) << 16;
        dirent->start_cluster |= shortEndian(buffer + offset + 0x1a);

        dirent->size = intEndian(buffer + offset + 0x1c);

        // copy the name
        memcpy(dirent->short_name, buffer + offset, 8);

		//////////////////////////////////////////////////////////
		// add a lfn entry for the shortname
		lfn = &dirent->lfn[dirent->num_lfn_entries++];
		lfn->offset = offset;
		lfn->sector = dirent->phy_sector;
		lfn->cluster = dirent->cluster;
		//////////////////////////////////////////////////////////
			
        // kill trailing space
        for (i = 8; i > 0 && dirent->short_name[i-1] == ' '; --i)
            ;

        // get the extension
        dirent->short_name[i++] = '.';
        memcpy(dirent->short_name + i, buffer + offset + 8, 3);

        // kill trailing space
        for (j = 3; j > 0 && dirent->short_name[i+j-1] == ' '; --j)
            ;

        // hack! kill the . if there's no extension
        if (j == 0) --j;

        dirent->short_name[i+j] = 0;

        found_file = 1;
    }
    while (!found_file);

    if (dirent->long_name[0] != '\0')
        dirent->name = dirent->long_name;
    else
        dirent->name = dirent->short_name;

    return 1;
}

void fat_rewind(fat_dirent *dirent) {
    // reset the metadata to point to the beginning of the dir
    dirent->index = 0;
    dirent->cluster = dirent->first_cluster;
    dirent->sector = 0;
}



/**
 * Get an array of all the sectors a file owns. Returns all sectors of all
 * clusters even if the file does not occupy all the sectors (i.e., a 512 byte
 * file will still return 4 sectors in a 4 sector-per-cluster FS).
 *
 * Return 0 on succes, -1 on fail
 * Only failure mode is if the size of the sectors array is too small.
 */
int fat_get_sectors(uint32_t start_cluster, uint32_t *sectors, int size) {
    int i, num = 0;
    uint32_t cluster, sector;

    cluster = start_cluster;

    while (cluster < 0x0ffffff8) {
        sector = CLUSTER_TO_SECTOR(cluster);
        for (i = 0; i < fat_fs.sect_per_clus; ++i) {
            // make sure we have enough space for the sectors
            if (num >= size)
                return -1;

            sectors[num++] = sector + i;
        }
        cluster = fat_get_fat(cluster);
    }

    return 0;
}


/**
 * Return the sector and offset into the sector of a file given start cluster
 * and offset.
 *
 * Returns:
 *  FAT_SUCCESS on success
 *  FAT_EOF when the end of file is reached
 */
int fat_get_sector(uint32_t start_cluster, uint32_t offset, uint32_t *sector, uint32_t *new_offset) 
{
    uint32_t bytes_per_clus = fat_fs.sect_per_clus * 512;
    uint32_t cluster = start_cluster;

	// skip to the right cluster
	while (offset >= bytes_per_clus) {
		cluster = fat_get_fat(cluster);

		// hit the end of the file
		if (cluster >= 0x0ffffff7)
			return FAT_EOF;

		offset -= bytes_per_clus;
	}
	
    *sector = CLUSTER_TO_SECTOR(cluster);
    while (offset >= 512) {
        ++*sector;
        offset -= 512;
    }
    *new_offset = offset;

    return FAT_SUCCESS;
}


/**
 * Allocate a new cluster after the last cluster. Sets the end of file marker
 * in the FAT as a bonus.
 * 
 * If last_cluster is 0, it assumes this is the first cluster in the file and
 * WILL NOT set the FAT entry on cluster 0.
 * 
 * Returns:
 *  FAT_SUCCESS on success, new cluster in new_cluster
 *  FAT_NOSPACE when the FS has no free clusters
 *  FAT_INCONSISTENT when the fs needs to be checked
 */
static int _fat_allocate_cluster(uint32_t last_cluster, uint32_t *new_cluster) {
    int ret;
    uint32_t new_last;

    if (fat_fs.free_clusters == 0)
        return FAT_NOSPACE;

    ret = _fat_find_free_entry(last_cluster, &new_last);

    // according to the free cluster count, we should be able to find a free
    // cluster. since _fat_find_free_entry couldn't, this means the FS must be
    // in an inconistent state
    if (ret == FAT_NOSPACE)
        return FAT_INCONSISTENT;

    // see comment above for explanation
    if (last_cluster != 0)
        fat_set_fat(last_cluster, new_last);

    fat_set_fat(new_last, 0x0ffffff8);
    --fat_fs.free_clusters;

    *new_cluster = new_last;
    return FAT_SUCCESS;
}

/**
 * Fills a cluster with 0's.
 */
static void _fat_clear_cluster(uint32_t cluster) {
    unsigned char clear_buffer[512] = { 0, };
    uint32_t i, sector = CLUSTER_TO_SECTOR(cluster);

    for (i = 0; i < fat_fs.sect_per_clus; ++i)
        ciWriteSector(clear_buffer, sector + i);
}

/**
 * Fills a cluster with FF's.
 */
static void _fat_clear_ff_cluster(uint32_t cluster) {
    unsigned char clear_buffer[512] = { 0xff, };
    uint32_t i, sector = CLUSTER_TO_SECTOR(cluster);

    for (i = 0; i < fat_fs.sect_per_clus; ++i)
        ciWriteSector(clear_buffer, sector + i);
}

/**
 * Calculate remaining dirents in a directory at EOD.
 * If the dirent isn't EOD, this value is bogus.
 */
static int _fat_remaining_dirents(fat_dirent *dirent) {
    int remaining;
    uint32_t cluster = dirent->cluster;

    // DE_PER_SECTOR for each unused sector in the cluster
    remaining = DE_PER_SECTOR * (fat_fs.sect_per_clus - (dirent->sector + 1));


    // remaining in the current sector
    remaining += DE_PER_SECTOR - dirent->index;

    // add the rest of the clusters in the directory
    while ((cluster = fat_get_fat(cluster)) < 0x0ffffff7)
        remaining += DE_PER_SECTOR * fat_fs.sect_per_clus;

    return remaining;
}

/**
 * Allocate a bunch of dirents at the end of a directory. If there aren't
 * enough available, allocate a new cluster.
 *
 * Preconditions:
 *  dirent is at end of directory
 *  count >= 0
 *
 * Return:
 *  FAT_SUCCESS     success
 *  FAT_NOSPACE     file system full
 */
static int _fat_allocate_dirents(fat_dirent *dirent, int count) {
    int remaining, ret;
    uint32_t cluster;

    remaining = _fat_remaining_dirents(dirent);
    if (count > remaining) {
        ret = _fat_allocate_cluster(dirent->cluster, &cluster);
        if (ret == FAT_NOSPACE)
            return FAT_NOSPACE;

        _fat_flush_fat();
        _fat_clear_cluster(cluster);
    }

    return FAT_SUCCESS;
}


/**
 * Find a file by name in a directory. Create it if it doesn't exist.
 * Initialize the first cluster of a directory. Creates the . and .. entries.
 */

static void _fat_init_dir(uint32_t cluster, uint32_t parent) {

    unsigned char buf[512] = { 0, };
    char name[11];
    uint32_t i, sector = CLUSTER_TO_SECTOR(cluster);

    uint16_t top16, bottom16;

    memset(name, 0x20, sizeof(name));

    // FAT32 hack: if parent is root, cluster should be 0
    if (parent == fat_fs.root_cluster)
        parent = 0;

    //
    // .
    //
    name[0] = '.';
    memcpy(&buf[0], name, sizeof(name));
    buf[11] = 0x10; // dir

    // current dir start cluster
    top16 = (cluster >> 16 & 0xffff);
    bottom16 = (cluster & 0xffff);
    writeShort(&buf[0x14], top16);
    writeShort(&buf[0x1a], bottom16);
    //
    // ..
    //
    name[1] = '.';
    memcpy(&buf[32], name, sizeof(name));
    buf[32+11] = 0x10; // dir

    // parent start cluster
    top16 = (parent >> 16 & 0xffff);
    bottom16 = (parent & 0xffff);
    writeShort(&buf[32 + 0x14], top16);
    writeShort(&buf[32 + 0x1a], bottom16);

    //
    // write first sector
    //
    ciWriteSector(buf, sector);

    //
    // zero the rest of the sectors
    //
    memset(buf, 0, 32 * 2);

    for (i = 1; i < fat_fs.sect_per_clus; ++i)
        ciWriteSector(buf, sector + i);

}

static const unsigned char charmap[] = {
        '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
        '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
        '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
        '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
        '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
        '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
        '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
        '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
        '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
        '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
        '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
        '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
        '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
        '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
        '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
        '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
        '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
        '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
        '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
        '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
        '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
        '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
        '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
        '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
        '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
        '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
        '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

/*
 * strcasecmp --
 *      Do strcmp(3) in a case-insensitive manner.
 *
 * PUBLIC: #ifndef HAVE_STRCASECMP
 * PUBLIC: int strcasecmp __P((const char *, const char *));
 * PUBLIC: #endif
 */
int strcasecmp(s1, s2)
        const char *s1, *s2;
{
        register const unsigned char *cm = charmap,
                        *us1 = (const unsigned char *)s1,
                        *us2 = (const unsigned char *)s2;

        while (cm[*us1] == cm[*us2++])
                if (*us1++ == '\0')
                        return (0);
        return (cm[*us1] - cm[*--us2]);
}

/**
 * Find a file by name in a directory. Create it if it doesn't exist.
 * Return:
 *  FAT_SUCCESS     success
 *  FAT_NOSPACE     file system full
 */
int fat_find_create(char *filename, fat_dirent *folder, fat_dirent *result_de, int dir, int create) {
    int ret, segment, i, num_dirents, total_clusters;
    int len;
    char long_name[256];
    char short_name[12];
    char segment_chars[26];
    unsigned char crc, *buf;
    uint16_t date_field, time_field;
	uint32_t start_cluster;


    //
    // Try to find the file in the dir, return it if found
    //
    while ((ret = fat_readdir(folder)) > 0)
        if (strcasecmp(filename, folder->name) == 0) {
            // found, return it
            *result_de = *folder;
            return FAT_SUCCESS;
        }

	if(!create){
		return FAT_NOEXIST;
	}
    //
    // File not found. Create a new file
    //

    // short name is a random alphabetic string
    for (i = 0; i < 11; ++i)
        short_name[i] = 'A' + (rand() % ('Z' - 'A' + 1));
    short_name[11] = 0; // for display purposes

    // calc the CRC
    crc = 0;
    for (i = 0; i < 11; ++i)
        crc = ((crc<<7) | (crc>>1)) + short_name[i];

    // trunc long name to 255 bytes
	memset(long_name, 0, 255);
    strncpy(long_name, filename, 255);
    long_name[255] = '\0';
    len = strlen(long_name);

    num_dirents  = 1; // short filename
    num_dirents += (len - 1) / 13 + 1; // long filename
	total_clusters = 0;

	// add a cluster if we have to extend the directory for new dirents
	if (num_dirents > _fat_remaining_dirents(folder))
		++total_clusters;
	// add a cluster if we're creating a dir
	if (dir)
		++total_clusters;
	
	// make sure we've got enough room
	if (fat_fs.free_clusters < total_clusters)
		return FAT_NOSPACE;

    ret = _fat_allocate_dirents(folder, num_dirents);
    if (ret == FAT_NOSPACE)
        return FAT_INCONSISTENT;
		
	// since we may have allocated a new cluster we must reload the dir
	_fat_load_dir_sector(folder);

    *result_de = *folder;

    // copy it 13 bytes at a time
    for (segment = len / 13; segment >= 0; --segment) {
		memset(segment_chars, 0, 26);
        // copy up to (and including) the first ASCII NUL
        for (i = 0; i < 13; ++i) {

            segment_chars[2*i] = long_name[segment * 13 + i];

            if (segment_chars[2*i] == '\0') {
                ++i; // make sure we FF after the \0
                break;
            }
        }
        // 0xFF the rest
        for ( ; i < 13; ++i) {
            segment_chars[2*i] = 0xff;
            segment_chars[2*i+1] = 0xff;
        }

        buf = &buffer[folder->index * 32];
        memset(buf, 0, 32);

		memcpy(&buf[1], &segment_chars[0], 10);
        memcpy(&buf[14], &segment_chars[10], 12);
        memcpy(&buf[28], &segment_chars[22], 4);

        buf[0] = (segment + 1) | ((segment == len / 13) << 6);
        buf[11] = 0x0f;
        buf[13] = crc;

        dir_buffer_dirty = 1;
		_fat_flush_fat();

        // TODO check for inconsistency here
        ++folder->index;
        _fat_load_dir_sector(folder);
    }

    //
    // 8.3 dirent
    //
    buf = &buffer[folder->index * 32];
    memset(buf, 0, 32);

    // copy the short name 
    memcpy(buf, short_name, 11);

    // directory: allocate the first cluster and init it
    if (dir) {
        uint16_t top16, bottom16;

        // attribute: archive and dir flags
        buf[11] = 0x30;
        ret = _fat_allocate_cluster(0, &start_cluster);
        if (ret == FAT_NOSPACE)
            return FAT_INCONSISTENT;

        _fat_init_dir(start_cluster, folder->first_cluster);
		
		//////////////////////////////////////////////////////////////////
		// MARSHALLH ADD DEC 31 2010
		//////////////////////////////////////////////////////////////////
		// Unless you flush the changes, the directory entry will be added
		// but the cluster will not show as allocated in the FATs
		//////////////////////////////////////////////////////////////////
		_fat_flush_fat();

        // start cluster
        top16 = (start_cluster >> 16 & 0xffff);
        writeShort(&buf[0x14], top16);
        bottom16 = (start_cluster & 0xffff);
        writeShort(&buf[0x1a], bottom16);
    }
    // regular file
    else {
        // attribute: archive
        buf[11] = 0x20;
    }


    // dates and times
    //date_field = (11) | (9 << 5) | (21 << 9); // 9/11/01
    //time_field = (46 << 5) | (8 << 11); // 8:46 AM

	date_field = (29) | (9 << 5) | (16 << 9); // 9/29/96
    time_field = (37 << 5) | (13 << 11); // 12:00 AM

    writeShort(&buf[14], time_field); // create
    writeShort(&buf[16], date_field); // create
    writeShort(&buf[18], date_field); // access
    writeShort(&buf[22], time_field); // modify
    writeShort(&buf[24], date_field); // modify

    dir_buffer_dirty = 1;
    _fat_flush_dir();

    fat_readdir(result_de);

    return FAT_SUCCESS;
}

/**
 * Write a dirent back to disk.
 */
static void _fat_write_dirent(fat_dirent *de) {
    uint32_t sector = CLUSTER_TO_SECTOR(de->cluster) + de->sector;
    uint32_t offset = (de->index - 1) * 32;
    uint16_t top16, bottom16;

    _dir_read_sector(sector);
    // size
    writeInt(&buffer[offset + 0x1c], de->size);

    // start cluster
    top16 = (de->start_cluster >> 16 & 0xffff);
    writeShort(&buffer[offset + 0x14], top16);

    bottom16 = (de->start_cluster & 0xffff);
    writeShort(&buffer[offset + 0x1a], bottom16);

    dir_buffer_dirty = 1;
}

/**
 * Sets file size, adding and removing clusters as necessary.
 * Returns:
 *  FAT_SUCCESS on success
 *  FAT_NOSPACE if the file system is full
 *  FAT_INCONSISTENT if the file system needs to be checked
 */
int fat_set_size(fat_dirent *de, uint32_t size) {
    int ret;
    float bytes_per_clus = fat_fs.sect_per_clus * 512;
	float cur_size = de->size, new_size = size;

    uint32_t current_clusters, new_clusters;
	uint32_t count;
	uint32_t current;
	uint32_t prev;
	uint32_t next;

    // true NOP, no change in size whatsoever
    if (de->size == size)
        return 0;

	current_clusters = ceilf(cur_size / bytes_per_clus);
	new_clusters = ceilf(new_size / bytes_per_clus);

    // expand file
    if (new_clusters > current_clusters) {
        count = 0;
        current = de->start_cluster;

        // first make sure we have enough clusters
        if (new_clusters - current_clusters > fat_fs.free_clusters)
            return FAT_NOSPACE;

        // if the file's empty it will have no clusters, so create the first
        if (current == 0) {
            ret = _fat_allocate_cluster(0, &current);

            // see comment below about FAT_INCONSISTENT for details
            if (ret == FAT_NOSPACE)
                return FAT_INCONSISTENT;

            de->start_cluster = current;
            ++count;
        }

        // otherwise skip to last entry
        else {
            prev = current;
            while ((current = fat_get_fat(current)) < 0x0ffffff8) {
                ++count;
                prev = current;
            }
            current = prev;
            ++count; // account for terminating sector
        }

        // add new clusters
        while (count < new_clusters) {
            ret = _fat_allocate_cluster(current, &current);

            // we already made sure there would be enough clusters according
            // to metadata, so if we can't allocate a cluster then the file
            // system must be inconsistent
            if (ret == FAT_NOSPACE)
                return FAT_INCONSISTENT;

            ++count;
        }
    }

    // remove sectors
    else if (new_clusters < current_clusters) {
        current = de->start_cluster;
        prev = current;

        // skip to the first entry past the new last FAT entry
        for (count = 0; count < new_clusters; ++count) {
            prev = current;
            current = fat_get_fat(current);
        }

        // if the file's not empty, set the last FAT entry to END
        if (count > 0)
            fat_set_fat(prev, 0x0ffffff8);
        else
            de->start_cluster = 0;

        // zero the rest of the FAT entries
        do {
            next = fat_get_fat(current);
            fat_set_fat(current, 0);
            current = next;
            ++fat_fs.free_clusters;
        } while (current < 0x0ffffff6 && current > 0);
    }

    else // (new_clusters == current_clusters), NOP but still need to update dirent
        ;

    // update size in dirent
    de->size = size;

    // write it back to disk
    _fat_write_dirent(de);

    _fat_flush_fat();
    _fat_flush_dir();

    return 0;
}


/**
 * Load the file beginning at clus to the RAM beginning at ramaddr.
 */
void loadRomToRam(uint32_t ramaddr, uint32_t clus, int byteswap)
{
    uint32_t ram = ramaddr;

    uint32_t start_cluster = clus;
    uint32_t current_cluster = start_cluster;
    uint32_t next_cluster;

    uint32_t start_sector;
    uint32_t clusters;
    
    while (start_cluster < 0x0ffffff8) {
        next_cluster = fat_get_fat(current_cluster);

        // contiguous so far, keep copying
        if (next_cluster != current_cluster + 1)  
        {
            start_sector = CLUSTER_TO_SECTOR(start_cluster);
            clusters = current_cluster - start_cluster + 1;

            ciSetByteSwap(byteswap);
            ciSectorsToRam(ram, start_sector, clusters * fat_fs.sect_per_clus);
			ciSetByteSwap(0);

            start_cluster = next_cluster;
            ram += clusters * fat_fs.sect_per_clus * 512 / 2;
        }

        current_cluster = next_cluster;
    }

    return;
}

/**
 * Writed the file beginning at clus from the RAM beginning at ramaddr.
 */
void loadRamToRom(uint32_t ramaddr, uint32_t clus)
{
    uint32_t ram = ramaddr;

    uint32_t start_cluster = clus;
    uint32_t current_cluster = start_cluster;
    uint32_t next_cluster;

    uint32_t start_sector;
    uint32_t clusters;
    
    while (start_cluster < 0x0ffffff8) {
        next_cluster = fat_get_fat(current_cluster);

        // contiguous so far, keep copying
        if (next_cluster != current_cluster + 1)  
        {
            start_sector = CLUSTER_TO_SECTOR(start_cluster);
            clusters = current_cluster - start_cluster + 1;

            ciRamToSectors(ram, start_sector, clusters * fat_fs.sect_per_clus);

            start_cluster = next_cluster;
            ram += clusters * fat_fs.sect_per_clus * 512 / 2;
        }

        current_cluster = next_cluster;
    }

    return;
}

// sector read from file
unsigned char file_buffer[512];
uint32_t file_buffer_sector = 0;
 
// helper functions
static int _fat_load_file_sector(fat_file_t *file);
  
/**
 * Read len bytes out of file into buf.
 *
 * Returns the number of bytes read.
 * If return value == 0, file is at EOF.
 *
 * Return of -1 indicates error.
 */
int32_t fat_read(fat_file_t *file, unsigned char *buf, int32_t len) {
    uint32_t bytes_read = 0;
    int ret;
	int bytes_left;
	
    if (len < 0)
        return -1;

    if (file->position + len > file->de.size)
        len = file->de.size - file->position;

    while (bytes_read < len) {
 
        ret = _fat_load_file_sector(file);
        // FIXME: fs_error() function
        if (ret != FAT_SUCCESS)
            return -1;

        bytes_left = 512 - file->offset;
        if (bytes_read + bytes_left > len)
            bytes_left = len - bytes_read;

        memcpy(buf + bytes_read, file_buffer + file->offset, bytes_left);
        bytes_read += bytes_left;
        file->position += bytes_left;

        file->offset += bytes_left;
    }

    return bytes_read;
}

/**
 * Seek to absolute position in file.
 * Returns:
 *  FAT_SUCCESS on success
 *  FAT_INCONSISTENT if the file system needs to be checked
 */
static int _fat_seek(fat_file_t *file, uint32_t position) {
    uint32_t bytes_per_clus = fat_fs.sect_per_clus * 512;
    uint32_t seek_left = position;
	uint32_t cluster;
	
    // trunc position
    if (position > file->de.size) position = file->de.size;

    file->sector = 0;

    cluster = file->de.start_cluster;

    while (seek_left > 512) {
        cluster = fat_get_fat(cluster);
        if (cluster >= 0x0ffffff8)
            return FAT_INCONSISTENT;

        // found the right cluster, so find the right sector and offset
        if (seek_left < bytes_per_clus) {
            file->cluster = cluster;
            while (seek_left >= 512) {
                seek_left -= 512;
                ++file->sector;
            }
        }

        // more clusters to go
        else
            seek_left -= bytes_per_clus;
    }

    file->cluster = cluster;
    file->offset = seek_left;
    file->position = position;
    return FAT_SUCCESS;
}


/**
 * Seek a la lseek
 *
 * Returns
 *  FAT_SUCCESS on success
 *  FAT_BADINPUT if whence is not one of SEEK_SET, SEEK_CUR, or SEEK_END
 *  FAT_INCONSISTENT if the file system needs to be checked
 */
int fat_lseek(fat_file_t *file, int32_t offset, int whence) {
    int32_t position;

    switch(whence)
    {
        case SEEK_SET:
            /* From the beginning */
            if(offset < 0){
                position = 0;
            } else {
                position = offset;
            }

            break;
        case SEEK_CUR:
        {
            /* From the current position */
            position = file->position + offset;

            if(position < 0) {
                position = 0;
            }

            break;
        }
        case SEEK_END:
        {
            /* From the end of the file */
            position = (int32_t)file->de.size + offset;

            if(position < 0){
                position = 0;
            }

            break;
        }
        default:
            /* Impossible */
            return FAT_BADINPUT;
    }

    /* Let's get some bounds checking */
    if(position > file->de.size){
        position = file->de.size;
    }

    return _fat_seek(file, position);
}

// load the current sector from the file
// cut & paste code from _dir_load_sector :(
//
// returns:
//  FAT_SUCCESS         success
//  FAT_INCONSISTENT    fs inconsistent
static int _fat_load_file_sector(fat_file_t *file) {
    uint32_t sector;
    uint32_t fat_entry;

    if (file->offset == 512) {
        if (file->sector + 1 == fat_fs.sect_per_clus) {
            // look up the cluster number in the FAT
            fat_entry = fat_get_fat(file->cluster);
            if (fat_entry >= 0x0ffffff8) // last cluster
                return FAT_INCONSISTENT;

            file->cluster = fat_entry;
            file->sector = 0;
            file->offset = 0;
        }
        else {
            ++file->sector;
            file->offset = 0;
        }
    }

    // sector may or may not have changed, but buffering makes this efficient
    sector = CLUSTER_TO_SECTOR(file->cluster) + file->sector;

    // TODO dirty file cluster?
    if (file_buffer_sector != sector) {
        ciReadSector(file_buffer, sector);
        file_buffer_sector = sector;
    }

    return FAT_SUCCESS;
}

 /**
 * Open a file from a dirent. Assumes you've found the file you desire using
 * fat_find_create.
 */
void fat_open_from_dirent(fat_file_t *file, fat_dirent *de) 
{
    file->de = *de;
    file->dir = 0; // FIXME ?

    file->cluster = de->start_cluster;
    file->sector = 0;
    file->offset = 0;
    file->position = 0;
}
