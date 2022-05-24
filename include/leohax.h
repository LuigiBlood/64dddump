#ifndef _leohax_h_
#define _leohax_h_

#include	<ultra64.h>
#include	<PR/leo.h>

#define LEO_COUNTRY_JPN		0xE848D316
#define LEO_COUNTRY_USA		0x2263EE56
#define LEO_COUNTRY_NONE	0x00000000

//Variables
extern u32	LEO_country_code;	//Disk Country Code Lock
extern u8	LEOdisk_type;		//Current Disk Type
extern u8	LEO_sys_data[0xE8];	//System Data
extern u8	LEOdrive_flag;		//Is System Area Read (0xFF = Read, 0 = Not Read)
extern u32	__leoActive;		//Is Leo Manager Active?

#define haxSystemAreaReadSet()			(LEOdrive_flag = 0xff)
#define haxSystemAreaReadClr()			(LEOdrive_flag = 0)
#define haxSetCountryCode(x)			(LEO_country_code = x)

//Hacking
void haxReadErrorRetry(u16 retries);
void haxAll();
void haxMediumChangedClear();

#endif
