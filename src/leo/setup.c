#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"
#include	"leoctl.h"

#define	DISK_MSG_NUM	1
#define	NUM_LEO_MESGS	8
static OSMesg	LeoMessages[NUM_LEO_MESGS];

OSMesgQueue	diskMsgQ;
OSMesg	diskMsgBuf[DISK_MSG_NUM];

OSPiHandle *LeoDiskHandle;
OSPiHandle *DriveRomHandle;

u8 isUnlocked = 0;

/* Initialize Disk Related Functionality and Library */
s32 initDisk()
{
	s32 error;

	osCreateMesgQueue(&diskMsgQ, diskMsgBuf, DISK_MSG_NUM);

	LeoDiskHandle = osLeoDiskInit();
	DriveRomHandle = osDriveRomInit();

	haxAll();
	error = LeoCJCreateLeoManager((OSPri)OS_PRIORITY_LEOMGR-1, (OSPri)OS_PRIORITY_LEOMGR, LeoMessages, NUM_LEO_MESGS);
	LeoResetClear();

	if (isRegisterPresent())
		unlockDisk();

	return error;
}

/* Unlock Hidden Commands */
void unlockDisk()
{
	if (isUnlocked == 1)
		return;
	
	/* LEO! */
	diskDoCommand(0x00500000, 0x4C450000);
	diskDoCommand(0x00500000, 0x4F210000);

	isUnlocked = 1;
}
