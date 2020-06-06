//Access to libleo variables (2.0I)

#ifdef DEBUG
#error Compiling this in this state will break hard. Compile it with FINAL=YES instead.
#endif

#ifdef _LANGUAGE_C_PLUS_PLUS
extern "C" {
#endif

//Variables
extern u32 LEO_country_code;	//Disk Country Code Lock
extern u8 LEOdisk_type;		//Current Disk Type
extern u8 LEO_sys_data[0xE8];	//System Data
extern u8 LEOdrive_flag;	//Is System Area Read (0xFF = Read, 0 = Not Read)

#define haxSystemAreaReadSet()          (LEOdrive_flag = 0xff)
#define haxSystemAreaReadClr()          (LEOdrive_flag = 0)

//Functions
extern void leoRead();			//Read LBAs
extern void leomain();			//Command Thread
extern void leointerrupt();			//Interrupt Thread
extern void leoRead_system_area();	//System Area Read
extern void leoClrUA_MEDIUM_CHANGED();	//Clear Medium Changed Flag
extern void leoSetUA_MEDIUM_CHANGED();	//Set Medium Changed Flag


#ifdef _LANGUAGE_C_PLUS_PLUS
}
#endif

#define LEO_COUNTRY_JPN		0xE848D316
#define LEO_COUNTRY_USA		0x2263EE56
#define LEO_COUNTRY_NONE	0x00000000

//Hack libleo functions in real time (and avoid doing all the hax beforehand)
void haxSystemAreaAccess()
{
	//Force System Area access
	u32 *hax;
	
	hax = &leoRead + 0x18;
	*hax = 0x24040000;		//Don't add +24 to LBAs, Nintendo, please.
}

void haxDriveDetection()
{
	//Force Drive Detection for Dev Drives
	u32 *hax;
	
	hax = &LeoCJCreateLeoManager + 0x34;
	*hax = 0x10000003;
	
	hax = &LeoCJCreateLeoManager + 0x1A0;
	*hax = 0x14610000;
	
	hax = &LeoCJCreateLeoManager + 0x1E0;	//Ignore Region
	*hax = 0x10000000;
	
	hax = &LeoCJCreateLeoManager + 0x1F4;
	*hax = 0x03240019;
}

void haxIDDrive()
{
	//Ignore Drive ID
	u32 *hax;
	
	hax = &leomain + 0x330;
	*hax = 0x10000000;
	
	hax = &leomain + 0x398;
	*hax = 0x00000000;
}

void haxDevDiskAccess()
{
	u32 *hax;
	
	hax = &leoRead_system_area + 0xFC;
	*hax = 0x14400000;
	
	hax = &leoRead_system_area + 0x118;
	*hax = 0x10400000;
	
	hax = &leoRead_system_area + 0x170;
	*hax = 0x14430000;
}

void haxReadErrorRetry(u16 retries)
{
	u32 *hax;
	
	hax = &leointerrupt + 0x6A4;
	*hax = 0x3A820000 | retries;
}

void haxAll()
{
	haxSystemAreaAccess();
	haxDriveDetection();
	haxIDDrive();
	haxDevDiskAccess();
}

void haxMediumChangedClear()
{
	LeoResetClear();
	leoClrUA_MEDIUM_CHANGED();
}
