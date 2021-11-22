//FAT
//fat_dirent  de_root;
fat_dirent	de_current;
fat_dirent	de_next;
char		fat_message2[50];
short		fail;

char		filename_ndd[256];
char		filename_log[256];

void set_filenames(char* diskIDstr, char* region)
{
	if (_diskID.gameName[0] >= 0x20 && _diskID.gameName[1] >= 0x20 && _diskID.gameName[2] >= 0x20 && _diskID.gameName[3] >= 0x20
	&& _diskID.gameName[0] < 0x7B && _diskID.gameName[1] < 0x7B && _diskID.gameName[2] < 0x7B && _diskID.gameName[3] < 0x7B)
	{
		sprintf(filename_ndd, "NUD-%c%c%c%c-%s.ndd", diskIDstr[0], diskIDstr[1], diskIDstr[2], diskIDstr[3], region);
		sprintf(filename_log, "NUD-%c%c%c%c-%s.log", diskIDstr[0], diskIDstr[1], diskIDstr[2], diskIDstr[3], region);
	}
	else
	{
		sprintf(filename_ndd, "NUD-DUMP-%s.ndd", region);
		sprintf(filename_log, "NUD-DUMP-%s.log", region);
	}
}

int fs_fail()
{
	char wait_message[50];
	sprintf(wait_message, "\n    Error (%i): %s", fail, fat_message2);
	draw_puts(wait_message);
	return 0;
}

void fat_start(char* filename_use, u32 size)
{
	char console_text[50];
	fat_dirent de_root;
	fat_dirent user_dump;
	float fw = ((float)ciGetRevision())/100.0f;
	u32 device_magic = ciGetMagic();
	u32 is64drive = (ciGetMagic() == 0x55444556);

	if(fw < 2.00 || fw > 20.00 || !is64drive)
	{
		draw_puts("\n    - ERROR WRITING TO MEMORY CARD:\n    64DRIVE with FIRMWARE 2.00 or later is required!");
		while(1);
	}

	sprintf(console_text, "\n    - Writing %s to memory card", filename_use);
	draw_puts(console_text);
  
	// get FAT going
	if(fat_init() != 0)
	{
		fail = 100;
		fs_fail();
		return;
	}
  
	// start in root directory
	fat_root_dirent(&de_root);
	if(fail != 0)
	{
		fs_fail();
		return;
	}
  
  
	if( fat_find_create(filename_use, &de_root, &user_dump, 0, 1) != 0)
	{
		sprintf(fat_message2, "Failed to create image dump"); fail = 3;
		fs_fail(); return;
	}

	if( fat_set_size(&user_dump, size) != 0)
	{
		sprintf(fat_message2, "Failed to resize dump"); fail = 4;
		fs_fail(); return;
	}

	loadRamToRom(0x0, user_dump.start_cluster);
}