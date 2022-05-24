//Access to libleo variables (2.0L)
#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"

#ifdef DEBUG
#error Compiling this in this state will break hard. Compile it with FINAL=YES instead.
#endif

//Functions
extern void leoRead();			//Read LBAs
extern void leomain();			//Command Thread
extern void leointerrupt();		//Interrupt Thread
extern void leoRead_system_area();	//System Area Read
extern void leoClrUA_MEDIUM_CHANGED();	//Clear Medium Changed Flag
extern void leoSetUA_MEDIUM_CHANGED();	//Set Medium Changed Flag
extern u32 LeoDriveExist();	//Read Drive IPL

//Hack libleo functions in real time (and avoid doing all the hax beforehand)
void haxSystemAreaAccess()
{
	//leoread.o

	//Force System Area access
	u32 *hax;
	
	//-LBA +0 instead of +24
	hax = (u32*)&leoRead + 0x18/4;
	*hax = 0x24040000;		//Don't add +24 to LBAs, Nintendo, please.
}

void haxDriveDetection()
{
	//cjcreateleomanager.o

	//Force Drive Detection for Dev Drives
	u32 *hax;
	
	//-Remove Call to LeoDriveExist
	hax = (u32*)&LeoCJCreateLeoManager + 0x2C/4;
	*hax = 0x00000000;
	
	//-Force Branch and pretend a drive is present
	hax = (u32*)&LeoCJCreateLeoManager + 0x34/4;
	*hax = 0x10000003;
	
	//-Do not go to infinite loop (Ignore Drive Type and Region)
	hax = (u32*)&LeoCJCreateLeoManager + 0x1A0/4;
	*hax = 0x14610000;
	
	//-Remove Infinite Loop
	hax = (u32*)&LeoCJCreateLeoManager + 0x1E0/4;
	*hax = 0x10000000;
	
	hax = (u32*)&LeoCJCreateLeoManager + 0x1F4/4;
	*hax = 0x03240019;
}

void haxIDDrive()
{
	//leocmdex.o

	//Ignore Drive ID
	u32 *hax;
	
	//-Remove Infinite Loop (Development Region)
	hax = (u32*)&leomain + 0x330/4;
	*hax = 0x10000000;
	
	//-Ignore Format ID (0x10)
	hax = (u32*)&leomain + 0x398/4;
	*hax = 0x00000000;
}

void haxDevDiskAccess()
{
	//leocmdex.o
	u32 *hax;
	
	//-Remove Infinite Loop (Retail Regions)
	hax = (u32*)&leoRead_system_area + 0xFC/4;
	*hax = 0x14400000;
	
	//-Remove Infinite Loop (Development Region)
	hax = (u32*)&leoRead_system_area + 0x118/4;
	*hax = 0x10400000;
	
	//-Remove Infinite Loop (Region Mismatch)
	hax = (u32*)&leoRead_system_area + 0x170/4;
	*hax = 0x14430000;
}

void haxReadErrorRetry(u16 retries)
{
	//leoint.o
	u32 *hax;
	
	//-Change Amount of Retries
	hax = (u32*)&leointerrupt + 0x6A4/4;
	*hax = 0x3A820000 | retries;
	
	osWritebackDCacheAll();
}

void haxDriveExist()
{
	u32 *hax;
	
	hax = (u32*)&LeoDriveExist + 0x0/4;
	*hax = 0x24020001;	//addiu v0,0,1
	hax = (u32*)&LeoDriveExist + 0x4/4;
	*hax = 0x03E00008;	//jr ra
	hax = (u32*)&LeoDriveExist + 0x8/4;
	*hax = 0x00000000;	//nop
}

void haxAll()
{
	haxSystemAreaAccess();
	haxDriveDetection();
	haxIDDrive();
	haxDevDiskAccess();
	haxDriveExist();
	osWritebackDCacheAll();
}

void haxMediumChangedClear()
{
	LeoResetClear();
	leoClrUA_MEDIUM_CHANGED();
}
