//FAT
//fat_dirent  de_root;
fat_dirent	de_current;
fat_dirent	de_next;
char		fat_message[50];
short		fail;

int fs_fail()
{
	char wait_message[50];
	sprintf(wait_message, "\n    Error (%i): %s", fail, fat_message);
	draw_puts(wait_message);
	return 0;
}

void fat_start(char diskIDstr[])
{
	char filenamestr[20];
	//char filenamelogstr[20];
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
	
	sprintf(console_text, "\n    - 64drive with firmware %.2f found", fw);
	draw_puts(console_text);
  
	sprintf(filenamestr, "NUD-%c%c%c%c-JPN.ndd", diskIDstr[0], diskIDstr[1], diskIDstr[2], diskIDstr[3]);
	//sprintf(filenamelogstr, "NUD-%c%c%c%c-JPN.log", diskIDstr[0], diskIDstr[1], diskIDstr[2], diskIDstr[3]);

	sprintf(console_text, "\n    - Writing %s to memory card, please wait...", filenamestr);
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
	
	if( fat_find_create(filenamestr, &de_root, &user_dump, 0, 1) != 0)
	{
		sprintf(fat_message, "Failed to create image dump"); fail = 3;
		fs_fail(); return;
	}
	
	if( fat_set_size(&user_dump, 0x3DEC800) != 0)
	{
		sprintf(fat_message, "Failed to resize dump"); fail = 4;
		fs_fail(); return;
	}

	loadRamToRom(0x0, user_dump.start_cluster);
}

void fat_startlog(char diskIDstr[], u32 size)
{
	char filenamestr[20];
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
  
	//sprintf(console_text, "\n    - 64drive with firmware %.2f found", fw);
	//draw_puts(console_text);
  
	sprintf(filenamestr, "NUD-%c%c%c%c-JPN.log", diskIDstr[0], diskIDstr[1], diskIDstr[2], diskIDstr[3]);

	sprintf(console_text, "\n    - Writing %s to memory card", filenamestr);
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
  
  
	if( fat_find_create(filenamestr, &de_root, &user_dump, 0, 1) != 0)
	{
		sprintf(fat_message, "Failed to create image dump"); fail = 3;
		fs_fail(); return;
	}

	if( fat_set_size(&user_dump, size) != 0)
	{
		sprintf(fat_message, "Failed to resize dump"); fail = 4;
		fs_fail(); return;
	}

	loadRamToRom(0x0, user_dump.start_cluster);
}