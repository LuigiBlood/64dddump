#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"
#include	"leoctl.h"
#include	"asicdrive.h"
#include	"cartaccess.h"

/* Disk */
u8	blockData[USR_SECS_PER_BLK*SEC_SIZE_ZONE0] __attribute__ ((aligned (8)));	//0x4D08
u8	errorData[SIZ_P_LBA];
s32 errorCheck;
s32 errorDiskId;
LEODiskID	_diskId;

s32 diskReadLBA(s32 lba)
{
	/* Read LBA and keep sense info from libleo */
	s32 error;
	LEOCmd _cmdBlk;

	LeoReadWrite(&_cmdBlk, OS_READ, lba, (void*)&blockData, 1, &diskMsgQ);
	osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
	errorData[lba] = (u8)error;
	return error;
}

s32 diskSkipLBA(s32 lba)
{
	/* Skip LBA and keep sense info from libleo */
	s32 error;
	error = LEO_SENSE_SKIPPED_LBA;
	if (!isDiskPresent()) error = LEO_SENSE_MEDIUM_NOT_PRESENT;
	if (isDiskChanged()) error = LEO_SENSE_MEDIUM_MAY_HAVE_CHANGED;

	errorData[lba] = (u8)error;
	return error;
}

s32 diskReadID()
{
	/* This automates certain disk checks on libleo's side */
	s32 error;
	LEOCmd _cmdBlk;

	LeoReadDiskID(&_cmdBlk, &_diskId, &diskMsgQ);
	osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);

	errorDiskId = error;
	return error;
}

s32 diskCheck()
{
	s32 i;
	s32 error;
	const s32 system_lbas[] = {SYS_DATA_LBA1, SYS_DATA_LBA2, SYS_DATA_LBA3, SYS_DATA_LBA4};
	s32 offset_lba = diskGetDriveType() == LEO_DRIVE_TYPE_DEV ? SYS_LBA_DEV_OFFSET : 0;

	/* Check Disk System Info, do diskReadID first, if not working, do manual work here */
	errorCheck = diskReadID();

	//If the read is good, then there's nothing else to do
	if (errorCheck == LEO_SENSE_GOOD)
	{
		errorDiskId = errorCheck;
		return errorCheck;
	}
	
	//If the error is unrelated to the disk that's a problem and we need to stop immediately
	if (!isErrorDiskRelated(errorCheck))
		return errorCheck;

	//If the read is bad however, we need to force things a little bit
	bzero(LEO_sys_data, 0xE8);	//Zero Current System Data
	haxSystemAreaReadSet();		//Set System Area is read to use LeoReadWrite without any checks from the driver
	haxMediumChangedClear();	//Clear Medium Changed Flag in drive and driver

	//Try to read the regular System Data first
	for (i = 0; i < 4; i++)
	{
		errorCheck = diskReadLBA(system_lbas[i] + offset_lba);
		if (errorCheck == LEO_SENSE_GOOD) break;
		if (!isErrorDiskRelated(errorCheck))
			return errorCheck;
	}

	if (errorCheck == LEO_SENSE_GOOD)
	{
		//System Data has been read correctly:

		//Use Formatting Info for dumping
		if (diskGetDriveType() == LEO_DRIVE_TYPE_DEV)
			bcopy(blockData, LEO_sys_data, 0xC0);	//Development Drive
		else
			bcopy(blockData, LEO_sys_data, 0xE8);	//Retail and Writer Drive
		
		//Update Disk Type for library to do its job
		LEOdisk_type = LEO_sys_data[0x05] & 0x0F;
		errorCheck = isSysDataValid() ? LEO_SENSE_GOOD : -1;
	}
	
	if (errorCheck != LEO_SENSE_GOOD)
	{
		//System Data has NOT been read correctly:
		LEOdisk_type = 0;			//Set Disk Type in libleo to 0 by default

		//Read Disk Formatting Info (Try LBA 4, then 5 if it didn't work out)
		errorCheck = diskReadLBA(SYS_DATA_INIT_LBA1);
		if (!isErrorDiskRelated(errorCheck))
			return errorCheck;
		if (errorCheck != LEO_SENSE_GOOD)
		{
			errorCheck = diskReadLBA(SYS_DATA_INIT_LBA2);

			//If you really cannot get those, then it's best to not even try further.
			if (errorCheck != LEO_SENSE_GOOD) return errorCheck;
		}

		//Use Formatting Info for dumping
		if (diskGetDriveType() == LEO_DRIVE_TYPE_DEV)
			bcopy(blockData, LEO_sys_data, 0xC0);	//Development Drive
		else
			bcopy(blockData, LEO_sys_data, 0xE8);	//Retail and Writer Drive
		errorCheck = LEO_SENSE_FMTLOAD;
	}

	//Try to read Disk ID (not required)
	error = diskReadLBA(DISK_ID_LBA1);
	if (!isErrorDiskRelated(error))
		return error;
	if (error != LEO_SENSE_GOOD)
	{
		error = diskReadLBA(DISK_ID_LBA2);
		if (!isErrorDiskRelated(error))
			return error;
	}

	if (error == LEO_SENSE_GOOD)
		bcopy((void*)&_diskId, blockData, sizeof(LEODiskID));	//Copy Disk ID
	else
		bzero((void*)&_diskId, sizeof(LEODiskID));				//Clear Disk ID

	errorDiskId = error;
	return errorCheck;
}

s32 diskBreakMotor()
{
	/* Apply the brakes to the motor */
	s32 error;
	LEOCmd _cmdBlk;

	LeoSpdlMotor(&_cmdBlk, LEO_MOTOR_BRAKE, &diskMsgQ);
	osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
}

/* IPL ROM */
void iplCopy(char *src, char *dest, const int len)
{
	OSIoMesg dmaIoMesgBuf;
	OSMesg dummyMesg;
	dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_NORMAL;
	dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
	dmaIoMesgBuf.dramAddr = dest;
	dmaIoMesgBuf.devAddr = (u32)src;
	dmaIoMesgBuf.size = len;
	osEPiStartDma(DriveRomHandle, &dmaIoMesgBuf, OS_READ);
	osRecvMesg(&dmaMessageQ, &dummyMesg, OS_MESG_BLOCK);
}

void iplBlockRead(s32 addr)
{
	iplCopy((char*)addr, blockData, 0x4000);
}

u32 iplRead(s32 addr)
{
	u32 data;
	osEPiReadIo(DriveRomHandle, addr, &data);
	return data;
}

/* H8 Drive Control ROM */
u8 h8Read(u16 addr)
{
	//Set Address
	diskDoCommand(0x00300000, addr << 16);

	//Read Byte
	return (diskDoCommand(0x00310000, 0) >> 16) & 0xFF;
}

void h8BlockRead(s32 addr)
{
	//Use Block Data, read 0x1000
	s32 i;
	for (i = 0; i < 0x1000; i++)
		blockData[i] = h8Read(i + addr);
}

/* EEPROM */
u8 eepromRead(u8 addr)
{
	//0x1C (Retail) and 0x44 (Retail & Dev)
	return (diskDoCommand(0x00440000, addr << 24) >> 16) & 0xFF;
}

void eepromBlockRead()
{
	//Use Block Data, read 0x80
	s32 i;
	for (i = 0; i < 0x80; i++)
		blockData[i] = eepromRead(i);
}

/* Register Set */
void registerBlockDump()
{
	s32 i;
	u32 tmp;
	for (i = 0; i < 0x1000; i += 4)
	{
		tmp = diskReadRegister(ASIC_IO_BASE + i);
		blockData[i+0] = (tmp >> 24) & 0xFF;
		blockData[i+1] = (tmp >> 16) & 0xFF;
		blockData[i+2] = (tmp >> 8) & 0xFF;
		blockData[i+3] = (tmp >> 0) & 0xFF;
	}
}

/* RTC */
s32 rtcRead(LEODiskTime *ret)
{
	s32 error;
	LEOCmd _cmdBlk;
	
	LeoReadRTC(&_cmdBlk, &diskMsgQ);
	osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);

	if (error == LEO_SENSE_GOOD)
		bcopy(&_cmdBlk.data.time, ret, sizeof(LEODiskTime));
	
	return error;
}

u32 rtcReadFat()
{
	s32 error;
	LEODiskTime time;
	
	error = rtcRead(&time);

	if (error == LEO_SENSE_GOOD)
	{
		s32 year = convertBCD((time.yearhi << 8) | time.yearlo);
		s32 month = convertBCD(time.month);
		s32 day = convertBCD(time.day);
		s32 hour = convertBCD(time.hour);
		s32 minute = convertBCD(time.minute);
		s32 second = convertBCD(time.second);
		return (year - 1980) << 25 | month << 21 | day << 16 | hour << 11 | minute << 5 | second >> 1;
	}
	else
	{
		//YEAR | MONTH | DAY | HOUR | MINUTE | SECOND /2
		return (2022 - 1980) << 25 | (7) << 21 | (28) << 16 | (22) << 11 | (54) << 5 | (30) >> 1;
	}
}
