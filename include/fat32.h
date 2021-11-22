//
// fat32.h
//

#ifndef _FAT32_H
#define _FAT32_H


typedef u32 uint32_t;
typedef s32 int32_t;
typedef u16 uint16_t;
typedef u8 off_t;

#define MAX_LFN (19 + 1)	// 1 shortname plus 19 long segments

typedef struct _lfn_entry_t {
    uint32_t cluster;
    int sector;
    uint32_t offset;
} lfn_entry_t;


typedef struct _fat_dirent {
    // file properties
    char *name;
    char short_name[13];
    char long_name[256];
    int directory;
	int volume_label;
	int hidden;
    uint32_t start_cluster;
    uint32_t size;

    // metadata
    uint32_t index;
    uint32_t cluster;
    uint32_t sector;
	uint32_t phy_sector;

	int num_lfn_entries;
	lfn_entry_t lfn[MAX_LFN];

    uint32_t first_cluster;
} fat_dirent;

struct _fat_file_t {
	fat_dirent de;
	int dir;
  
	uint32_t cluster;   // current cluster
    uint32_t sector;    // current sector in cluster
    int offset;         // offset in sector [0, 512)
	uint32_t position;  // absolute file position
};
typedef struct _fat_file_t fat_file_t;

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

#define FAT_SUCCESS 0
#define FAT_NOSPACE 1
#define FAT_EOF 2
#define FAT_NOEXIST 3
#define FAT_NOTFOUND 3
#define FAT_BADINPUT 128
#define FAT_INCONSISTENT 256

// prototypes

void			loadRomToRam(uint32_t ramaddr, uint32_t clus, int byteswap);
void			loadRamToRom(uint32_t ramaddr, uint32_t clus);

uint32_t		fat_get_fat(uint32_t cluster);
uint32_t		fat_cluster_to_sector(uint32_t clus);
uint32_t		fat_sect_per_clus();
void			fat_sector_offset(uint32_t cluster, uint32_t *fat_sector, uint32_t *fat_offset);
static uint32_t _fat_absolute_sector(uint32_t relative_sector, int fat_num);
static void		_fat_flush_fat(void);
uint32_t		_fat_load_fat(uint32_t cluster);
void			fat_set_fat(uint32_t cluster, uint32_t value);
static int		_fat_find_free_entry(uint32_t start, uint32_t *new_entry);
int				fat_root_dirent(fat_dirent *dirent);
void			fat_sub_dirent(uint32_t start_cluster, fat_dirent *de);
static void		_fat_flush_dir(void);
static void		_dir_read_sector(uint32_t sector);
static int		_fat_load_dir_sector(fat_dirent *dirent);
int				fat_readdir(fat_dirent *dirent);
void			fat_rewind(fat_dirent *dirent);
int				fat_get_sectors(uint32_t start_cluster, uint32_t *sectors, int size);
int				fat_get_sector(uint32_t start_cluster, uint32_t offset, uint32_t *sector, uint32_t *new_offset);
static int		_fat_allocate_cluster(uint32_t last_cluster, uint32_t *new_cluster);
static void		_fat_clear_cluster(uint32_t cluster);
static int		_fat_remaining_dirents(fat_dirent *dirent);
static int		_fat_allocate_dirents(fat_dirent *dirent, int count);
static void		_fat_init_dir(uint32_t cluster, uint32_t parent);
int 			fat_find_create(char *filename, fat_dirent *folder, fat_dirent *result_de, int dir, int create);
static void		_fat_write_dirent(fat_dirent *de);


int 			fat_set_size(fat_dirent *de, uint32_t size);

uint32_t		intEndian(unsigned char *i);
void			writeInt(unsigned char *dest, uint32_t val);
unsigned short	shortEndian(unsigned char *i);
void			writeShort(unsigned char *dest, uint16_t val);

void 			fat_open_from_dirent(fat_file_t *file, fat_dirent *de);
int32_t 		fat_read(fat_file_t *file, unsigned char *buf, int32_t len);
static int 		_fat_load_file_sector(fat_file_t *file) ;
static int 		_fat_seek(fat_file_t *file, uint32_t position);
int 			fat_lseek(fat_file_t *file, int32_t offset, int whence);



#endif