#include	<ultra64.h>
#include	<PR/leo.h>
#include	<nustd/math.h>
#include	<nustd/string.h>
#include	<nustd/stdlib.h>

#include	"thread.h"
#include	"ddtextlib.h"

#include	"asicdrive.h"
#include	"leohax.h"
#include	"cart.h"
#include	"control.h"
#include	"leoctl.h"
#include	"64drive.h"
#include	"process.h"
#include	"version.h"

extern s32 pdisk_drivetype;
s32 pdisk_lba_rom_end;
s32 pdisk_lba_ram_start;
s32 pdisk_lba_ram_end;

s32 pdisk_e_checkbounds()
{
	//Call after Disk ID is read
	//Get Formatting Info

	LEOCapacity capacity;
	LeoReadCapacity(&capacity, OS_WRITE);

	if (pdisk_drivetype == LEO_DRIVE_TYPE_DEV)
	{
		//Dev Disks
		if ((LEO_sys_data[0x05] & 0x0F) < 6)
		{
			pdisk_lba_rom_end = capacity.startLBA - 1 + SYSTEM_LBAS;
			pdisk_lba_ram_start = capacity.startLBA + SYSTEM_LBAS;
			pdisk_lba_ram_end = capacity.endLBA + SYSTEM_LBAS;
		}
		else
		{
			pdisk_lba_rom_end = MAX_P_LBA;
			pdisk_lba_ram_start = MAX_P_LBA;
			pdisk_lba_ram_end = MAX_P_LBA;
		}
	}
	else
	{
		//Retail Disks
		if ((LEO_sys_data[0x05] & 0x0F) < 6)
		{
			pdisk_lba_rom_end = ((LEO_sys_data[0xE0] << 8) | LEO_sys_data[0xE1]) + SYSTEM_LBAS;
			pdisk_lba_ram_start = ((LEO_sys_data[0xE2] << 8) | LEO_sys_data[0xE3]) + SYSTEM_LBAS;
			pdisk_lba_ram_end = ((LEO_sys_data[0xE4] << 8) | LEO_sys_data[0xE5]) + SYSTEM_LBAS;
		}
		else
		{
			pdisk_lba_rom_end = MAX_P_LBA;
			pdisk_lba_ram_start = MAX_P_LBA;
			pdisk_lba_ram_end = MAX_P_LBA;
		}
	}
}
