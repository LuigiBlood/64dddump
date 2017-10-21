//64DD Dump 0.60
//Based on pfs SDK demo

#include	<ultra64.h>
#include	<PR/leo.h>
#include	"thread.h"
#include	"textlib.h"
#include	"ncode.h"
#include	<math.h>
#include	<string.h>

#include	"fat32.h"
#include	"ci.h"

#define	NUM_MESSAGE 	1

#define	CONT_VALID	1
#define	CONT_INVALID	0

#define WAIT_CNT	5

extern OSMesgQueue retraceMessageQ;

extern u16	*bitmap_buf;
extern u16	bitmap_buf1[];
extern u16	bitmap_buf2[];
extern u8	backup_buffer[];

OSMesgQueue		pifMesgQueue;
OSMesg			dummyMessage;
OSMesg			pifMesgBuf[NUM_MESSAGE];

static OSContStatus	contstat[MAXCONTROLLERS];
static OSContPad	contdata[MAXCONTROLLERS];
static int		controller[MAXCONTROLLERS];
OSPfs			pfs0;
int	cont_no;
u8		bitpattern;
u16	button, oldbutton, newbutton;

//DMA PI
#define	DMA_QUEUE_SIZE	2
OSMesgQueue	dmaMessageQ;
OSMesg		dmaMessageBuf[DMA_QUEUE_SIZE];
OSIoMesg	dmaIOMessageBuf;

OSPiHandle *pi_handle;
OSPiHandle *LeoDiskHandle;

#include	"asicdrive.h"
#include	"leotest.h"

//64DD
#define	DISK_MSG_NUM	1
#define	NUM_LEO_MESGS	8
static OSMesg	LeoMessages[NUM_LEO_MESGS];

OSMesgQueue	diskMsgQ;
OSMesg	diskMsgBuf[DISK_MSG_NUM];

LEOStatus		leostat;
LEODiskID		_diskID ,_savedID;
static LEOCapacity	_leoCapacity;
static LEOCmd		_cmdBlk;
static LEOVersion	_leover;

static u32	selectLBA;
static s32	LBAsize;

static u8	blockData[19720];

static u32	error_LBA;
static u16	errorsLBA;
static u16	logData[19720];

static int 	DISKID_READ;
static int	DUMPTYPE;	//0 = SKIP, 1 = FORCE, 2 = SKIP, RAM ONLY, 3 = FORCE, RAM ONLY

//LBA skip stuff
#define MAX_ERROR_SKIP 5
static u32	LBA_ranges[4];
static u32	ERROR_NB;

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

	if( fat_set_size(&user_dump, 0x3DEC800) != 0)
	{
		sprintf(fat_message, "Failed to resize dump"); fail = 4;
		fs_fail(); return;
	}

	loadRamToRom(0x0, user_dump.start_cluster);
}

void fat_startlog(char diskIDstr[])
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
  
	sprintf(console_text, "\n    - 64drive with firmware %.2f found", fw);
	draw_puts(console_text);
  
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

	if( fat_set_size(&user_dump, ((errorsLBA * 16) + 49)) != 0)
	{
		sprintf(fat_message, "Failed to resize dump"); fail = 4;
		fs_fail(); return;
	}

	loadRamToRom(0x0, user_dump.start_cluster);
}

u32 totalLBAsize(u32 LBA)
{
	s32 size = 0;

	if (LBA >= 24)
	{
		LeoLBAToByte(0, (LBA - 24), &size);
		size += 19720 * 24;
	} else {
		size = 19720 * LBA;
	}
	
	return (u32)size;
}

int errorread(int error)
{
	switch (error)
	{
		case LEO_ERROR_GOOD:
		case LEO_ERROR_COMMAND_TERMINATED:
			return 0;
			break;
		case LEO_ERROR_UNRECOVERED_READ_ERROR:
			return 1;
			break;
		case LEO_ERROR_TRACK_FOLLOWING_ERROR:
			return 2;
			break;
		default:
			draw_puts("\n\n\n\n    ");
			errorcheck(error);
			draw_puts("\n    Error! Please reset this program.");
			for (;;);
			break;
	}
}

int isDiskPresent()
{
	u32 data = ReadLeoReg(ASIC_STATUS);
	if ((data & 0x01000000) == 0x01000000)
		return 1; //Disk Present
	else
		return 0; //Disk NOT present
}

int isDiskChanged()
{
	u32 data = ReadLeoReg(ASIC_STATUS);
	if ((data & 0x00010000) == 0x00010000)
		return 1; //Disk Changed
	else
		return 0; //Disk NOT changed
}

int isDiskDebug()
{
	LeoInquiry(&_leover);
	if ((_leover.drive & 0x0F) == 0x04)
		return 1; //DEVELOPMENT DRIVE
	else
		return 0; //RETAIL DRIVE
}

u16 readController()
{
	osContStartReadData(&pifMesgQueue);
	osRecvMesg(&pifMesgQueue, NULL, OS_MESG_BLOCK);
	osContGetReadData(&contdata[0]);
	
	if (contdata[cont_no].errno & CONT_NO_RESPONSE_ERROR)
	{
		button = oldbutton;
	} else {
		oldbutton = button;
		button = contdata[cont_no].button;
	}
	
	newbutton = ~oldbutton & button;
	return newbutton;
}

void copytoCart(const char *src, const char *dest, const int len)
{
        OSIoMesg dmaIoMesgBuf;
        OSMesg dummyMesg;
        dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_NORMAL;
        dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
        dmaIoMesgBuf.dramAddr = src;
        dmaIoMesgBuf.devAddr = (u32)dest;
        dmaIoMesgBuf.size = len;
        osEPiStartDma(pi_handle, &dmaIoMesgBuf, OS_WRITE);
        osRecvMesg(&dmaMessageQ, &dummyMesg, OS_MESG_BLOCK);
}

void mainproc(void *arg)
{
	int i, j;
	int error = 0;
	char console_text[50];
	int menumode = 0;
	int menuselect = 0;
	
	u8 system_area_good[4];
	u8 PAUSE = 0;
	
	cont_no = 0;
	button = 0;
	oldbutton = 0;
	newbutton = 0;
	
	system_area_good[0] = 0xFF;
	system_area_good[1] = 0xFF;
	system_area_good[2] = 0xFF;
	system_area_good[3] = 0xFF;
	
	//Create PIF/SI Message Queue
	osCreateMesgQueue(&pifMesgQueue, pifMesgBuf, NUM_MESSAGE);
	osSetEventMesg(OS_EVENT_SI, &pifMesgQueue, dummyMessage);
	
	//Initialize & Check Controllers
	osContInit(&pifMesgQueue, &bitpattern, &contstat[0]);
	for (i = 0; i < MAXCONTROLLERS; i++)
	{
		if ((bitpattern & (1<<i)) &&
			((contstat[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL))
		{
			controller[i] = CONT_VALID;
		} else {
			controller[i] = CONT_INVALID;
		}
	}
	
	//Create Disk Message Queue
	osCreateMesgQueue(&diskMsgQ, diskMsgBuf, DISK_MSG_NUM);
	//Create PI Message Queue
	osCreateMesgQueue(&dmaMessageQ, dmaMessageBuf, 1);
	
	pi_handle = osCartRomInit();
	LeoDiskHandle = osLeoDiskInit();
	
	selectLBA = 0;
	error_LBA = 0xFFFFFFFF;
	errorsLBA = 0;
	bzero(blockData, 19720);
	bzero(logData, 19720);
	
	DISKID_READ = 0;
	ERROR_NB = 0;
	
	//Initalize Console-type screen
	init_draw();
	
	//Initial Text
	setcolor(0,255,0);
	draw_puts("If you see this for a long period of time,\nsomething fucked up. Sorry.");
	
	//Init PI Manager
	osCreatePiManager((OSPri)OS_MESG_PRI_NORMAL, &dmaMessageQ, dmaMessageBuf, DMA_QUEUE_SIZE);
	
	//Initialize Leo Manager (64DD)
	LeoCJCreateLeoManager((OSPri)OS_PRIORITY_LEOMGR-1, (OSPri)OS_PRIORITY_LEOMGR, LeoMessages, NUM_LEO_MESGS);
	LeoResetClear();
	
	setbgcolor(15,15,15);
	clear_draw();
	
	while(1)
	{
		//Read Controller
		readController();
		
		//Draw Main Text (check if Disk is development unit or not)
		setcolor(0,255,0);
		if (isDiskDebug() == 0)
			draw_puts("\f\n    64DD dump v0.60 (Gray disk) by LuigiBlood & marshallh\n    ----------------------------------------\n");
		else
			draw_puts("\f\n    64DD dump v0.60 (Blue disk) by LuigiBlood & marshallh\n    ----------------------------------------\n");
		setcolor(255,255,255);
		
		//LeoInquiry
		LeoInquiry(&_leover);
		sprintf(console_text, "    Drive Ver: %02X / Driver Ver: %02X / Drive Type: %02X\n", _leover.drive, _leover.driver, _leover.deviceType);
		draw_puts(console_text);
		
		//Check if Disk is present
		if (isDiskPresent() == 0)
			DISKID_READ == 0;
		
		//Check if Disk was changed
		if (isDiskChanged() == 1)
			DISKID_READ == 0;
		
		//Read Disk ID-------------		
		if (DISKID_READ == 0)
		{
			LeoReadDiskID(&_cmdBlk, &_diskID, &diskMsgQ);
			osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
		
			switch (error)
			{
				case LEO_ERROR_COMMAND_TERMINATED:
				case LEO_ERROR_GOOD:
					//It read Disk ID normally
					sprintf(console_text, "    Disk ID: %c%c%c%c\n", _diskID.gameName[0], _diskID.gameName[1], _diskID.gameName[2], _diskID.gameName[3]);
					draw_puts(console_text);
					DISKID_READ = 1;
					break;
				case LEO_ERROR_MEDIUM_NOT_PRESENT:
					//Disk is missing
					draw_puts("    INSERT DISK          \n                                                 \n                                                  ");
					break;
				default:
					//Error
					sprintf(console_text, "    Disk ID: ????\n");
					draw_puts(console_text);
					break;
			}
		}
		else
		{
			//Disk is already known
			sprintf(console_text, "    Disk ID: %c%c%c%c\n", _diskID.gameName[0], _diskID.gameName[1], _diskID.gameName[2], _diskID.gameName[3]);
			draw_puts(console_text);
		}
		
		//MENU CODE
		if (menumode == 0)
		{
			//Select between SKIP or FORCE
			if (newbutton & D_JPAD || newbutton & U_JPAD)
				menuselect ^= 1;
			
			if (newbutton & A_BUTTON)
			{
				if (menuselect == 0)
					DUMPTYPE = 0;	//SKIP
				else
					DUMPTYPE = 1;	//FORCE
				menumode++;
				menuselect = 0;
			}
			
			setcolor(255,255,255);
			        
			if (menuselect == 0) setcolor(0,255,0);
			else setcolor(255,255,255);
			draw_puts("     SKIP MODE (faster)\n");
			if (menuselect == 1) setcolor(0,255,0);
			else setcolor(255,255,255);
			draw_puts("     FORCE MODE (all blocks dumped, slower)\n");
			setcolor(255,255,255);
		}
		else if (menumode == 1)
		{
			//Select between ALL BLOCKS, RAM ONLY
			if (newbutton & D_JPAD || newbutton & U_JPAD)
				menuselect ^= 1;
				
			if (newbutton & A_BUTTON)
			{
				if (menuselect == 1)
					DUMPTYPE += 2;	//RAM ONLY
				menumode++;
			}
			
			if (menuselect == 0) setcolor(0,255,0);
			else setcolor(255,255,255);
			draw_puts("     ALL BLOCKS                               \n");
			if (menuselect == 1) setcolor(0,255,0);
			else setcolor(255,255,255);
			draw_puts("     RAM AREA ONLY (save area)                \n");
			setcolor(255,255,255);
		}
		
		//DUMPING CODE
		if ((menumode == 2) && (DISKID_READ > 0) && (isDiskPresent() == 1))
		{
			//DUMP!!
			if (DUMPTYPE & 1 == 0)
				DISKID_READ = 1;
			if (DUMPTYPE & 1 == 1)
				DISKID_READ = 2;
			
			//64drive, enable write to SDRAM/ROM
			srand(osGetCount()); // necessary to generate unique short 8.3 filenames on memory card
			ciEnableRomWrites();
			
			if (DISKID_READ == 1)
				draw_puts("\f\n\n\n\n\n    Let's dump! Don't turn off the console or eject the disk! (Skip Mode)\n    *** DUMP IN PROCESS *** Press A to pause the dump.\n\n");
			else if (DISKID_READ == 2)
				draw_puts("\f\n\n\n\n\n    Let's dump! Don't turn off the console or eject the disk! (Force Mode)\n    *** DUMP IN PROCESS *** Press A to pause the dump.\n\n");
			
			//Dump
			for (selectLBA = 0; selectLBA < 0x10DC; selectLBA += 1)
			{
				readController();
				if (newbutton & A_BUTTON && PAUSE == 0)
					PAUSE = 1;
				
				while (PAUSE == 1)
				{
					sprintf(console_text, "\f\n\n\n\n\n\n    *** DUMP PAUSED *** Press A to continue. Press B to stop the dump.");
					draw_puts(console_text);
					
					readController();
					if (newbutton & A_BUTTON)
					{
						sprintf(console_text, "\f\n\n\n\n\n\n    *** DUMP IN PROCESS *** Press A to pause the dump.                ");
						draw_puts(console_text);
						PAUSE = 0;
					}
					else if (newbutton & B_BUTTON)
					{
						sprintf(console_text, "\f\n\n\n\n\n\n    *** DUMP STOPPED ***                                              ");
						draw_puts(console_text);
						PAUSE = 2;
					}
				}
				
				if (PAUSE == 2)
				{
					//STOP DUMPING
					//Log the skip
					errorsLBA++;
					logData[(errorsLBA - 1) * 2] = (u16)selectLBA ^ 0x8000; //or 0x8000 = SKIP
					logData[((errorsLBA - 1) * 2) + 1] = 0;
					error_LBA = 0xFFFFFFFF;
					
					//If it's >= to the MAX, then write 00s instead and skip to the end of the range.
					bzero(blockData, 19720);
					for (selectLBA = selectLBA; selectLBA < 0x10DC; selectLBA += 1)
					{
						sprintf(console_text, "\f\n\n\n\n\n\n\n\n    SKIPPING LBA: %4d/4315 (%.1f%%)  \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
						draw_puts(console_text);
						
						LeoLBAToByte(selectLBA - 24, 1, &LBAsize);  //range is always after the System Area. It's safe.
						
						osWritebackDCacheAll();
						osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
						osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
						
						logData[((errorsLBA - 1) * 2) + 1]++;
					}
					ERROR_NB = 0;
					break;
				}
				
				sprintf(console_text, "\f\n\n\n\n\n\n\n\n    DUMPING LBA: %4d/4315 (%.1f%%)            \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
				draw_puts(console_text);
				
				bzero(blockData, 19720);
				LeoReadWrite(&_cmdBlk, OS_READ, selectLBA, (void*)&blockData, 1, &diskMsgQ);
				osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
				
				if (selectLBA < 24)
					LBAsize = 19720;
				else
					LeoLBAToByte(selectLBA - 24, 1, &LBAsize);
				
				switch (errorread(error))
				{
					case 2:
						//TRACK_FOLLOWING_ERROR, try again one more time
						LeoReadWrite(&_cmdBlk, OS_READ, selectLBA, (void*)&blockData, 1, &diskMsgQ);
						osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
						
						if (errorread(error) != 0)
						{
							sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n    **LBA READ ERROR AT %4d**\n", selectLBA);
							draw_puts(console_text);
							if (error_LBA == 0xFFFFFFFF)
							{
								errorsLBA++;
								logData[(errorsLBA - 1) * 2] = (u16)selectLBA;
								logData[((errorsLBA - 1) * 2) + 1] = 1;
							}
							else if (error_LBA == (selectLBA - 1))
							{
								logData[((errorsLBA - 1) * 2) + 1]++;
							}
							
							error_LBA = selectLBA;
						}
						else
							error_LBA = 0xFFFFFFFF;
						
						osWritebackDCacheAll();
						//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
						//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
						copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
						
						if (selectLBA == (0 + (2 * isDiskDebug())))
							system_area_good[0] = 0x00; //Not Read
						else if (selectLBA == (1 + (2 * isDiskDebug())))
							system_area_good[1] = 0x00; //Not Read
						else if (selectLBA == (8 + (2 * isDiskDebug())))
							system_area_good[2] = 0x00; //Not Read
						else if (selectLBA == (9 + (2 * isDiskDebug())))
							system_area_good[3] = 0x00; //Not Read
						break;
					case 1:
						//UNRECOVERED_READ_ERROR, don't even try again
						sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n    * LBA READ ERROR AT %4d *\n", selectLBA);
						draw_puts(console_text);
						if (error_LBA == 0xFFFFFFFF)
						{
							errorsLBA++;
							logData[(errorsLBA - 1) * 2] = (u16)selectLBA;
							logData[((errorsLBA - 1) * 2) + 1] = 1;
						}
						else if (error_LBA == (selectLBA - 1))
						{
							logData[((errorsLBA - 1) * 2) + 1]++;
						}
						error_LBA = selectLBA;
						
						osWritebackDCacheAll();
						copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
						//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
						//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
						
						if (selectLBA == (0 + (2 * isDiskDebug())))
							system_area_good[0] = 0x00; //Not Read
						else if (selectLBA == (1 + (2 * isDiskDebug())))
							system_area_good[1] = 0x00; //Not Read
						else if (selectLBA == (8 + (2 * isDiskDebug())))
							system_area_good[2] = 0x00; //Not Read
						else if (selectLBA == (9 + (2 * isDiskDebug())))
							system_area_good[3] = 0x00; //Not Read
						break;
					case 0:
						sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n    **LBA LAST GOOD READ AT %4d**\n", selectLBA);
						draw_puts(console_text);
						
						osWritebackDCacheAll();
						copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
						//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
						//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
						
						if (error == 0)
							error_LBA = 0xFFFFFFFF;
						
						if (selectLBA == (0 + (2 * isDiskDebug())))
							system_area_good[0] = 0x01; //Read
						else if (selectLBA == (1 + (2 * isDiskDebug())))
							system_area_good[1] = 0x01; //Read
						else if (selectLBA == (8 + (2 * isDiskDebug())))
							system_area_good[2] = 0x01; //Read
						else if (selectLBA == (9 + (2 * isDiskDebug())))
							system_area_good[3] = 0x01; //Read
						break;
				}
				
				//Prepare SKIP stuff
				if (selectLBA == 24)           //0 = Retail, 2 = Blue
				{
					int LBAsysdata = -1;
					u32 data1 = 0;
					u32 data2 = 0;
					
					//Check which System Data was fully loaded
					if (system_area_good[3] == 0x01)
						LBAsysdata = 9 + (2 * isDiskDebug());
					else if (system_area_good[2] == 0x01)
						LBAsysdata = 8 + (2 * isDiskDebug());
					else if (system_area_good[1] == 0x01)
						LBAsysdata = 1 + (2 * isDiskDebug());
					else if (system_area_good[0] == 0x01)
						LBAsysdata = 0 + (2 * isDiskDebug());
					
					if (LBAsysdata != -1)
					{
						osWritebackDCacheAll();
						osEPiReadIo(pi_handle, 0xB00000E0 + (19720 * LBAsysdata), &data1);
						
						osWritebackDCacheAll();
						osEPiReadIo(pi_handle, 0xB00000E4 + (19720 * LBAsysdata), &data2);
						
						//LBA_ranges[0] = ROM Area LBA End
						//LBA_ranges[1] = RAM Area LBA Start
						//LBA_ranges[2] = RAM Area LBA End
						//LBA_ranges[3] = Max LBA
						
						LBA_ranges[1] = (data1 & 0xFFFF) + 24;
						LBA_ranges[0] = (data1 >> 16) + 24;
						LBA_ranges[2] = (data2 >> 16) + 24;
						LBA_ranges[3] = 0x10DC;
						
						if (LBA_ranges[0] > LBA_ranges[3])
							LBAsysdata = -1;
						
						if (LBA_ranges[1] > LBA_ranges[3])
							LBAsysdata = -1;
						
						if (LBA_ranges[2] > LBA_ranges[3])
							LBAsysdata = -1;
						
						if (LBA_ranges[0] > LBA_ranges[1])
							LBAsysdata = -1;
						
						if (LBA_ranges[1] > LBA_ranges[2])
							LBAsysdata = -1;
						
						if (LBA_ranges[0] > LBA_ranges[2])
							LBAsysdata = -1;
					}
					
					if (LBAsysdata == -1 || isDiskDebug() == 1)
					{
						//Use LeoReadCapacity if there is an issue, or if it is a blue disk for skip mode
						LeoReadCapacity(&_leoCapacity, OS_WRITE);
						
						LBA_ranges[0] = _leoCapacity.startLBA - 1 + 24;
						LBA_ranges[1] = _leoCapacity.startLBA + 24;
						LBA_ranges[2] = _leoCapacity.endLBA + 24;
						LBA_ranges[3] = 0x10DC;
					}
					ERROR_NB = 0;
					//SKIP TO RAM
					if (DUMPTYPE >= 2) //SKIP stuff (RAM ONLY DUMP)
					{
						//Log the skip
						errorsLBA++;
						logData[(errorsLBA - 1) * 2] = (u16)selectLBA ^ 0x8000; //or 0x8000 = SKIP
						logData[((errorsLBA - 1) * 2) + 1] = 0;
						error_LBA = 0xFFFFFFFF;
							
						//If it's >= to the MAX, then write 00s instead and skip to the end of the range.
						bzero(blockData, 19720);
						for (selectLBA = selectLBA; selectLBA < (LBA_ranges[1] - 1); selectLBA += 1)
						{
							sprintf(console_text, "\f\n\n\n\n\n\n\n\n    SKIPPING LBA: %4d/4315 (%.1f%%)  \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
							draw_puts(console_text);
							
							LeoLBAToByte(selectLBA - 24, 1, &LBAsize);  //range is always after the System Area. It's safe.
							
							osWritebackDCacheAll();
							copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
							//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
							//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
							
							logData[((errorsLBA - 1) * 2) + 1]++;
						}
					}
				}
				
				//MANAGE SKIP (only above LBA 24)
				if (DISKID_READ == 1 && selectLBA >= 24)
				{
					if (selectLBA > LBA_ranges[1] && selectLBA < LBA_ranges[2])
						ERROR_NB = 0;
					
					if ((selectLBA > LBA_ranges[0]) && (selectLBA < LBA_ranges[1]) && (errorread(error) != 0)) //SKIP stuff (1st range)
					{
						//if there's an error in the first/second range, first +1 ERROR
						ERROR_NB++;
						if (ERROR_NB >= 5)
						{
							//Log the skip
							errorsLBA++;
							logData[(errorsLBA - 1) * 2] = (u16)selectLBA ^ 0x8000; //or 0x8000 = SKIP
							logData[((errorsLBA - 1) * 2) + 1] = 0;
							error_LBA = 0xFFFFFFFF;
							
							//If it's >= to the MAX, then write 00s instead and skip to the end of the range.
							bzero(blockData, 19720);
							for (selectLBA = selectLBA; selectLBA < (LBA_ranges[1] - 1); selectLBA += 1)
							{
								sprintf(console_text, "\f\n\n\n\n\n\n\n\n    SKIPPING LBA: %4d/4315 (%.1f%%)  \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
								draw_puts(console_text);
								
								LeoLBAToByte(selectLBA - 24, 1, &LBAsize);  //range is always after the System Area. It's safe.
								
								osWritebackDCacheAll();
								copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
								//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
								//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
								
								logData[((errorsLBA - 1) * 2) + 1]++;
							}
							ERROR_NB = 0;
						}
					}
					else if ((selectLBA > LBA_ranges[2]) && (selectLBA < LBA_ranges[3]) && (errorread(error) != 0)) //SKIP stuff (2nd range)
					{
						//if there's an error in the first/second range, first +1 ERROR
						ERROR_NB++;
						if (ERROR_NB >= 5)
						{
							//Log the skip
							errorsLBA++;
							logData[(errorsLBA - 1) * 2] = (u16)selectLBA ^ 0x8000; //or 0x8000 = SKIP
							logData[((errorsLBA - 1) * 2) + 1] = 0;
							error_LBA = 0xFFFFFFFF;
							
							//If it's >= to the MAX, then write 00s instead and skip to the end of the range.
							bzero(blockData, 19720);
							for (selectLBA = selectLBA; selectLBA < (LBA_ranges[3] - 1); selectLBA += 1)
							{
								sprintf(console_text, "\f\n\n\n\n\n\n\n\n    SKIPPING LBA: %4d/4315 (%.1f%%)  \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
								draw_puts(console_text);
								
								LeoLBAToByte(selectLBA - 24, 1, &LBAsize);  //range is always after the System Area. It's safe.
								
								osWritebackDCacheAll();
								copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
								//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000 + totalLBAsize(selectLBA), (void *)&blockData, LBAsize, &dmaMessageQ);
								//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
								
								logData[((errorsLBA - 1) * 2) + 1]++;
							}
							ERROR_NB = 0;
						}
					}
				}
			}
			draw_puts("\n");
			
			//DONE!! NOW WRITE TO SD
			if (_diskID.gameName[0] >= 0x20 && _diskID.gameName[1] >= 0x20 && _diskID.gameName[2] >= 0x20 && _diskID.gameName[3] >= 0x20)
				fat_start(_diskID.gameName);
			else
				fat_start("TEST");
			
			//DO THE LOG
			if (errorsLBA > 0)
			{
				char logstr[(errorsLBA * 16) + 49];
				if (isDiskDebug() == 0)
				{
					if (_diskID.gameName[0] >= 0x20 && _diskID.gameName[1] >= 0x20 && _diskID.gameName[2] >= 0x20 && _diskID.gameName[3] >= 0x20)
						sprintf(console_text, "64DDdump 0.60 (Gray)\r\nNUD-%c%c%c%c-JPN.ndd LOG\r\n---", _diskID.gameName[0], _diskID.gameName[1], _diskID.gameName[2], _diskID.gameName[3]);
					else
						sprintf(console_text, "64DDdump 0.60 (Gray)\r\nNUD-TEST-JPN.ndd LOG\r\n---");
				}
				else
				{
					if (_diskID.gameName[0] >= 0x20 && _diskID.gameName[1] >= 0x20 && _diskID.gameName[2] >= 0x20 && _diskID.gameName[3] >= 0x20)
						sprintf(console_text, "64DDdump 0.60 (Blue)\r\nNUD-%c%c%c%c-JPN.ndd LOG\r\n---", _diskID.gameName[0], _diskID.gameName[1], _diskID.gameName[2], _diskID.gameName[3]);
					else
						sprintf(console_text, "64DDdump 0.60 (Blue)\r\nNUD-TEST-JPN.ndd LOG\r\n---");
				}
				
				strcpy(logstr, console_text);
				
				for (i = 1; i <= errorsLBA; i++)
				{
					if ((logData[(i - 1) * 2] & 0x8000) == 0)
						sprintf(console_text, "\r\n%04d-%04d BAD ", logData[(i - 1) * 2], (logData[(i - 1) * 2]) + (logData[((i - 1) * 2) + 1] - 1));
					else
						sprintf(console_text, "\r\n%04d-%04d SKIP", (logData[(i - 1) * 2] & 0x7FFF), (logData[(i - 1) * 2] & 0x7FFF) + (logData[((i - 1) * 2) + 1] - 1));
					
					//0000-1111 BAD /SKIP (0000 = LBA, 1111 = number of blocks after that one that has an error too)
					
					strcat(logstr, console_text);
				}
				osWritebackDCacheAll();
				copytoCart((void *)&logstr, 0xB0000000, ((errorsLBA * 16) + 49));
				//osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB0000000, (void *)&logstr, ((errorsLBA * 16) + 49), &dmaMessageQ);
				//osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
				
				if (_diskID.gameName[0] >= 0x20 && _diskID.gameName[1] >= 0x20 && _diskID.gameName[2] >= 0x20 && _diskID.gameName[3] >= 0x20)
					fat_startlog(_diskID.gameName);
				else
					fat_startlog("TEST");
			}
			
			draw_puts("\n    - DONE !!\n");
			
			for(;;);
		}
	}
}
