//64DD Dump 0.9
//Based on pfs SDK demo

#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>

#include	"thread.h"
#include	"textlib.h"

#include	"fat32.h"
#include	"ci.h"

#include	"gamelist.h"
#include	"leohax.h"
#include	"leotest.h"
#include	"asicdrive.h"

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

static u32	selectLBA;
static u32	skipLBA;
static s32	LBAsize;

static u8	blockData[19720];

static u32	error_LBA;
static u16	errorsLBA;
static u16	logData[19720];

static int 	DISKID_READ;
static int	DUMPTYPE;	//0 = SKIP, 1 = FORCE

//LBA skip stuff
#define MAX_ERROR_SKIP 5
static u32	LBA_ranges[4];
static u32	ERROR_NB;

#include "fatrecord.h"

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

void setDefaultLBARange()
{
	LeoReadCapacity(&_leoCapacity, OS_WRITE);
	
	LBA_ranges[0] = _leoCapacity.startLBA - 1 + 24;
	LBA_ranges[1] = _leoCapacity.startLBA + 24;
	LBA_ranges[2] = _leoCapacity.endLBA + 24;
	LBA_ranges[3] = 0x10DC;
}

void setLBARange()
{
	if (GetDriveType() == 0)
	{
		//LBA_ranges[0] = ROM Area LBA End
		//LBA_ranges[1] = RAM Area LBA Start
		//LBA_ranges[2] = RAM Area LBA End
		//LBA_ranges[3] = Max LBA
		
		int LBAsysdata = 0;
		
		LBA_ranges[0] = ((LEO_sys_data[0xE0] << 8) | LEO_sys_data[0xE1]) + 24;
		LBA_ranges[1] = ((LEO_sys_data[0xE2] << 8) | LEO_sys_data[0xE3]) + 24;
		LBA_ranges[2] = ((LEO_sys_data[0xE4] << 8) | LEO_sys_data[0xE5]) + 24;
		LBA_ranges[3] = 0x10DC;
		
		//Checks
		if (LBA_ranges[0] >= LBA_ranges[3])
			LBAsysdata = -1;
		
		if (LBA_ranges[1] >= LBA_ranges[3])
			LBAsysdata = -1;
		
		if (LBA_ranges[2] >= LBA_ranges[3])
			LBAsysdata = -1;
		
		if (LBA_ranges[0] >= LBA_ranges[1])
			LBAsysdata = -1;
		
		if (LBA_ranges[1] >= LBA_ranges[2])
			LBAsysdata = -1;
		
		if (LBA_ranges[0] >= LBA_ranges[2])
			LBAsysdata = -1;
		
		//Restore default LBA range in case of fuck up (such as Super Mario 64 Disk Version)
		if (LBAsysdata == -1)
			setDefaultLBARange();
	}
	else
	{
		setDefaultLBARange();
	}
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
			draw_puts("\n    FATAL ERROR! Reset this program.");
			for (;;);
			break;
	}
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
	//Use newbutton for global
}

void skipToLBA(u32 lba)
{
	//Log Skip
	if (skipLBA == lba)
		return;
	
	errorsLBA++;
	logData[(errorsLBA - 1) * 2] = (u16)selectLBA ^ 0x8000; //OR 0x8000 = SKIP
	logData[((errorsLBA - 1) * 2) + 1] = 0;
	error_LBA = 0xFFFFFFFF;
	
	skipLBA = lba;
}

void copytoCart(char *src, char *dest, const int len)
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
	DriveRomHandle = osDriveRomInit();
	
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
	
	//HAX 64DD
	haxAll();
	
	//Initialize Leo Manager (64DD)
	error = LeoCJCreateLeoManager((OSPri)OS_PRIORITY_LEOMGR-1, (OSPri)OS_PRIORITY_LEOMGR, LeoMessages, NUM_LEO_MESGS);
	LeoResetClear();
	
	setbgcolor(15,15,15);
	clear_draw();
	
	//Render Text
	setcolor(255,255,255);
	draw_puts("\f\n    64DD Disk Dumper v0.10 - by LuigiBlood & marshallh\n    ----------------------------------------\n");

	//Check if Leo Manager is indeed active
	if (error != LEO_ERROR_GOOD)
	{
		draw_puts("\f\n\n    Leo Manager Error: ");
		errorcheck(error);
		for (;;);
	}
	else if (__leoActive == 0)
	{
		draw_puts("\f\n\n    Leo Manager Error!");
		for (;;);
	}
	
	//Show Drive Type
	if (GetDriveType() == 0)
	{
		setcolor(128,128,128);
		draw_puts("    --- Retail Drive ");
	}
	else if (GetDriveType() == 1)
	{
		setcolor(100,100,255);
		draw_puts("    --- Development Drive ");
	}
	else
	{
		setcolor(170,170,170);
		draw_puts("    --- Writer Drive ");
	}
	
	//Show Drive IPL Region
	if (GetDriveIPLRegion() == 0xC3)
	{
		draw_puts("/ JPN ");
	}
	else if (GetDriveIPLRegion() == 0x04)
	{
		draw_puts("/ USA ");
	}
	else
	{
		sprintf(console_text, "/ %02X ", GetDriveIPLRegion());
		draw_puts(console_text);
	}
	draw_puts("---\n");
	
	setcolor(255,255,255);

	//Show LeoInquiry Info
	LeoInquiry(&_leover);
	sprintf(console_text, "    Drive Ver: %02X / Driver Ver: %02X / Drive Type: %02X\n", _leover.drive, _leover.driver, _leover.deviceType);
	draw_puts(console_text);
	
	menumode = 0;
	menuselect = 0;
	
	while(1)
	{
		//Read Controller
		readController();

		setcolor(255,255,255);
		draw_puts("\f\n\n\n\n\n");
		//Disk Present Test		
		if (isDiskPresent() == 0)
		{
			DISKID_READ = 0;
			draw_puts("    PLEASE INSERT DISK                                                \n");
			draw_puts("                                                                      \n");
			draw_puts("                                                                      \n");
			draw_puts("                                                                      \n");
			draw_puts("                                                                      \n");
			draw_puts("                                                                      \n");
			draw_puts("                                                                      \n");
		}
		
		//Has Disk been newly inserted? Then read the Disk ID.
		if (isDiskChanged() == 1)
		{
			DISKID_READ = 0;
		}
		
		//Read Disk ID
		if (DISKID_READ == 0 && isDiskPresent() == 1)
		{
			draw_puts("\f\n\n\n\n\n");
			LeoReadDiskID(&_cmdBlk, &_diskID, &diskMsgQ);
			osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
			
			switch (error)
			{
				case LEO_ERROR_GOOD:
					//It reads Disk ID normally

					//Show Disk ID
					sprintf(console_text, "    Disk ID: %c%c%c%c", _diskID.gameName[0], _diskID.gameName[1], _diskID.gameName[2], _diskID.gameName[3]);
					draw_puts(console_text);
					draw_puts(" - ");
					printGame(_diskID.gameName);
					DISKID_READ = 1;
					
					//Show Disk Format Type and Region
					sprintf(console_text, "\n    Disk Type: %02X - Disk Region: ", LEOdisk_type);
					draw_puts(console_text);
					if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_JPN)
						draw_puts("JAPAN             ");
					else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_USA)
						draw_puts("USA               ");
					else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_NONE)
						draw_puts("NONE              ");
					else
					{
						sprintf(console_text, "UNKNOWN (%08X)", *((u32*)&LEO_sys_data[0]));
						draw_puts(console_text);
					}
					draw_puts("                                                                      \n");
					draw_puts("                                                                      \n");
					draw_puts("                                                                      \n");
					draw_puts("                                                                      \n");
					break;
				default:
					//Error, this means the disk might not be used or is corrupted
					//Perform checks if the disk is in good shape.
					haxSystemAreaReadSet();	//Set System Area is read to use LeoReadWrite without any checks from the driver
					haxMediumChangedClear(); //Clear Medium Changed Flag in drive and driver

					//Clear sys data
					bzero(LEO_sys_data, 0xE8);

					//Clear Disk ID
					bzero((void*)&_diskID, 4);

					//Read Formatting Information (System LBA 4)
					LeoReadWrite(&_cmdBlk, OS_READ, 4, (void*)&blockData, 1, &diskMsgQ);
					osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
					if (error != LEO_ERROR_GOOD)
					{
						//Read Formatting Information (System LBA 5)
						LeoReadWrite(&_cmdBlk, OS_READ, 5, (void*)&blockData, 1, &diskMsgQ);
						osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);

						if (error != LEO_ERROR_GOOD)
						{
							draw_puts("    Disk Error: ");
							errorcheck(error);
							draw_puts("\n    Couldn't read Disk Information.                                   \n");
							draw_puts("                                                                      \n");
							draw_puts("                                                                      \n");
							DISKID_READ = 2;
							break;
						}
					}

					//Use Formatting Info for dumping
					if (GetDriveType() == 1)
						bcopy(blockData, LEO_sys_data, 0xC0);	//Development Drive
					else
						bcopy(blockData, LEO_sys_data, 0xE8);	//Retail and Writer Drive
					
					DISKID_READ = 1;
					LEOdisk_type = 0;	//Read in Disk Type 0 by default
					draw_puts("    Disk seems to be unused or corrupted. You can still dump it.      \n");
					draw_puts("    Disk will be dumped as Disk Type 0.                               \n");
					draw_puts("                                                                      \n");
					draw_puts("                                                                      \n");
					break;
			}
			
			menumode = 0;
			//Skip Option is not accessible on Dev Drives
			if (GetDriveType() == 1)
				menuselect = 1;
		}
		
		draw_puts("\f\n\n\n\n\n\n\n");
		// MENU, only show if a disk is present
		if (DISKID_READ == 1)
		{
			if (menumode == 0)
			{
				//Render Menu
				setcolor(255,255,255);			
				if (menuselect == 0) setcolor(0,255,0);
				if (GetDriveType() == 1) setcolor(100,100,100);
				draw_puts("    - SKIP DUMP (Skips unformatted blocks, retail disk only)          \n");
				setcolor(255,255,255);
				if (menuselect == 1) setcolor(0,255,0);
				draw_puts("    - FULL DUMP (Do not skip any blocks, slower)                      \n");
				setcolor(255,255,255);
				if (menuselect == 2) setcolor(0,255,0);
				draw_puts("    - FAST FULL DUMP (may be less accurate)                           \n");
				
				if (newbutton & D_JPAD)
				{
					menuselect++;
					if (menuselect > 2)
						menuselect = 0;
					if (GetDriveType() == 1 && menuselect == 0)
						menuselect = 1;
				}
				
				if (newbutton & U_JPAD)
				{
					if (menuselect <= 0)
						menuselect = 3;
					menuselect--;
					if (GetDriveType() == 1 && menuselect == 0)
						menuselect = 2;
				}
				
				if (newbutton & A_BUTTON)
				{
					if (menuselect == 2)
					{	
						DUMPTYPE = 1;
						menumode = 1;
						menuselect = 0;
					}
					else
					{
						DUMPTYPE = menuselect;
						menumode = 2;
						menuselect = 0;
					}
				}
			}
			else if (menumode == 1)
			{
				//Render Menu
				setcolor(255,255,255);			
				if (menuselect == 0) setcolor(0,255,0);
				draw_puts("    - 32 RETRIES, 1 second per bad block                              \n");
				setcolor(255,255,255);
				if (menuselect == 1) setcolor(0,255,0);
				draw_puts("    - 16 RETRIES, 0.5 second per bad block                            \n");
				setcolor(255,255,255);
				if (menuselect == 2) setcolor(0,255,0);
				draw_puts("    - 8 RETRIES, 0.25 second per bad block                            \n");
				
				if (newbutton & D_JPAD)
				{
					menuselect++;
					if (menuselect > 2)
						menuselect = 0;
				}
				
				if (newbutton & U_JPAD)
				{
					if (menuselect <= 0)
						menuselect = 3;
					menuselect--;
				}
				
				if (newbutton & A_BUTTON)
				{
					if (menuselect == 0)
						haxReadErrorRetry(32);
					else if (menuselect == 1)
						haxReadErrorRetry(16);
					else if (menuselect == 2)
						haxReadErrorRetry(8);
				
					menumode = 2;
				}
			}
			else if (menumode == 2)
			{
				menumode = 10;
			}
			else if (menumode == 10)
			{
				setcolor(255,255,255);
				draw_puts("                                                                      \n");
				draw_puts("    DO NOT TURN OFF THE SYSTEM OR REMOVE THE DISK                     \n");
				draw_puts("    --- DUMPING --- Press START to pause the dump.                    \n\n");
				
				//Init Dump
				skipLBA = 0;	//Tells up to which LBA number to skip reading
				PAUSE = 0;
				
				//64drive, enable write to SDRAM/ROM
				srand(osGetCount()); // necessary to generate unique short 8.3 filenames on memory card
				ciEnableRomWrites();

				//Set Filenames
				if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_JPN)
					set_filenames(_diskID.gameName, "JPN");
				else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_USA)
					set_filenames(_diskID.gameName, "USA");
				else if (*((u32*)&LEO_sys_data[0]) == LEO_COUNTRY_NONE)
					set_filenames(_diskID.gameName, "DEV");
				else
					set_filenames(_diskID.gameName, "UNK");
				
				//Set LBA ranges (already loaded thanks to ReadDiskID)
				setLBARange();
				
				//Dumping Disk
				for (selectLBA = 0; selectLBA < 0x10DC; selectLBA += 1)
				{
					//Read Controller to get button presses
					readController();
					
					//Pressing Start pauses the dump
					if (newbutton & START_BUTTON && PAUSE == 0)
						PAUSE = 1;
					
					while (PAUSE != 0)
					{
						//PAUSE CODE
						readController();
						draw_puts("\f\n\n\n\n\n\n\n\n\n    --- PAUSED --- Press START to resume the dump.");
						
						if (selectLBA < LBA_ranges[1])
						{
							draw_puts(" Press B to skip to RAM Area.\n\n");
						}
						else
						{
							draw_puts(" Press B to stop the dumping.\n\n");
						}
						
						if (newbutton & START_BUTTON)
						{
							draw_puts("\f\n\n\n\n\n\n\n\n\n    --- DUMPING --- Press START to pause the dump.                              ");
							PAUSE = 0;
						}
						else if (newbutton & B_BUTTON)
						{
							PAUSE = 0;
							draw_puts("\f\n\n\n\n\n\n\n\n\n    --- DUMPING --- Press START to pause the dump.                              ");
							if (selectLBA < LBA_ranges[1])
								skipToLBA(LBA_ranges[1]);
							else
								skipToLBA(LBA_ranges[3]);
						}
					}
					
					//Get current LBA size
					if (selectLBA < 24)
						LBAsize = 19720;
					else
						LeoLBAToByte(selectLBA - 24, 1, &LBAsize);
					
					//Zeroes LBA Data Memory
					bzero(blockData, 19720);
					
					//Check if skipLBA is higher than selectLBA
					if (selectLBA >= skipLBA)
					{
						//Read LBA
						
						//Prints current LBA
						sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n    DUMPING LBA: %4d/4315 (%.1f%%)            \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
						draw_puts(console_text);
						
						//Read LBA Data
						LeoReadWrite(&_cmdBlk, OS_READ, selectLBA, (void*)&blockData, 1, &diskMsgQ);
						osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
						
						//Wait until read, check if error
						switch (errorread(error))
						{
							case 2:
								//TRACK_FOLLOWING_ERROR
							case 1:
								//UNRECOVERED_READ_ERROR
								sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n\n\n    * LBA READ ERROR AT %4d *\n", selectLBA);
								draw_puts(console_text);
								
								//Log errors
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
								
								//Skip
								if (DUMPTYPE == 0 && selectLBA >= 24)
								{
									if (selectLBA > LBA_ranges[1] && selectLBA < LBA_ranges[2])
										ERROR_NB = 0;
									
									if ((selectLBA > LBA_ranges[0]) && (selectLBA < LBA_ranges[1]))
										ERROR_NB++;
										
									if ((selectLBA > LBA_ranges[2]) && (selectLBA < LBA_ranges[3]))
										ERROR_NB++;
									
									//If more than 5 errors outside the formatted LBAs, then skip to RAM
									if (ERROR_NB >= 5)
									{
										if (selectLBA < LBA_ranges[1])
											skipToLBA(LBA_ranges[1]);
										else if (selectLBA < LBA_ranges[3])
											skipToLBA(LBA_ranges[3]);
										ERROR_NB = 0;
									}
								}
								break;
							case 0:
								//GOOD
								sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n\n    **LBA LAST GOOD READ AT %4d**\n", selectLBA);
								draw_puts(console_text);
								error_LBA = 0xFFFFFFFF;
								break;
						}
					}
					else
					{
						//Skip LBA
						logData[((errorsLBA - 1) * 2) + 1]++;
						//Prints current LBA
						sprintf(console_text, "\f\n\n\n\n\n\n\n\n\n\n    SKIPPING LBA: %4d/4315 (%.1f%%)           \n", selectLBA, (float)selectLBA / 4315.0f * 100.0f);
						draw_puts(console_text);
					}
					
					//Copy Data to ROM
					osWritebackDCacheAll();
					copytoCart((void *)&blockData, 0xB0000000 + totalLBAsize(selectLBA), LBAsize);
				}
				
				//DUMP IS DONE
				draw_puts("\n");
				
				//Write Dump to SD Card
				fat_start(filename_ndd, 0x3DEC800);
				
				//Write Log to SD Card
				if (errorsLBA > 0)
				{
					char logstr[(errorsLBA * 16) + 49];
					if (GetDriveType() != 1)
						sprintf(console_text, "64DDdump 0.10 (Gray)\r\n%s LOG\r\n---", filename_ndd);
					else
						sprintf(console_text, "64DDdump 0.10 (Blue)\r\n%s LOG\r\n---", filename_ndd);
					
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

					fat_start(filename_log, ((errorsLBA * 16) + 49));
				}
				
				draw_puts("\n    - DONE, please power off the console.\n");
			
				for(;;);
			}
		}
	}
}
