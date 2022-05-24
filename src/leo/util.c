#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"
#include	"leoctl.h"
#include	"asicdrive.h"

u32 diskReadRegister(u32 address)
{
	u32 data = 0;
	osEPiReadIo(LeoDiskHandle, address, &data);
	return data;
}

void diskWriteRegister(u32 address, u32 data)
{
	osEPiWriteIo(LeoDiskHandle, address, data);
}

s32 diskGetLBASectorSize(s32 lba)
{
	return (diskGetLBABlockSize(lba) / USR_SECS_PER_BLK);
}

s32 diskGetLBABlockSize(s32 lba)
{
	u32 size = 0;

	if (lba >= SYSTEM_LBAS)
	{
		LeoLBAToByte((lba - SYSTEM_LBAS), 1, &size);
	} else {
		size = USR_SECS_PER_BLK * SEC_SIZE_ZONE0;
	}

	return size;
}

u32 diskGetLBAOffset(s32 lba)
{
	u32 size = 0;

	if (lba >= SYSTEM_LBAS)
	{
		LeoLBAToByte(0, (lba - SYSTEM_LBAS), &size);
		size += USR_SECS_PER_BLK * SEC_SIZE_ZONE0 * SYSTEM_LBAS;
	} else {
		size = USR_SECS_PER_BLK * SEC_SIZE_ZONE0 * lba;
	}
	
	return size;
}

u32 diskDoCommand(u32 cmd, u32 arg)
{
	diskWriteRegister(ASIC_DATA, arg);
	diskWriteRegister(ASIC_CMD, cmd);
	leoWait_mecha_cmd_done();
	return diskReadRegister(ASIC_DATA);
}

s32 isDiskPresent()
{
	u32 data = diskReadRegister(ASIC_STATUS);
	if ((data & 0x01000000) == 0x01000000)
		return 1; //Disk Present
	else
		return 0; //Disk NOT present
}

s32 isDiskChanged()
{
	u32 data = diskReadRegister(ASIC_STATUS);
	if ((data & 0x00010000) == 0x00010000)
		return 1; //Disk Changed
	else
		return 0; //Disk NOT changed
}

s32 diskGetDriveType()
{
	LEOVersion _leover;

	if (!isRegisterPresent())
		return LEO_DRIVE_TYPE_NONE; //NO DRIVE
	
	//Get ASIC Version
	u32 asicver = diskDoCommand(0x000A0000, 0);
	if (asicver & 0xF0000000 == 0x70000000)
		return LEO_DRIVE_TYPE_WRITER; //WRITER DRIVE
	
	//Get Drive Version
	LeoInquiry(&_leover);
	if ((_leover.drive & 0x0F) == 0x04)
		return LEO_DRIVE_TYPE_DEV; //DEVELOPMENT DRIVE
	else
		return LEO_DRIVE_TYPE_RETAIL; //RETAIL DRIVE
}

u32 diskGetIPLType()
{
	return iplRead(0x9FF00) >> 24;
}

s32 isRegisterPresent()
{
	return (diskReadRegister(ASIC_STATUS) & 0xFFFF) == 0;
}

s32 isIPLROMPresent()
{
	return iplRead(0x1010) == 0x2129FFF8;
}
