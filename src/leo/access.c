#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"
#include	"leoctl.h"
#include	"asicdrive.h"
#include	"cart.h"

/* Disk */
u8	blockData[USR_SECS_PER_BLK*SEC_SIZE_ZONE0] __attribute__ ((aligned (8)));	//0x4D08
u8	errorData[MAX_P_LBA];
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

s32 diskReadID()
{
	/* This automates certain disk checks on libleo's side */
	s32 error;
	LEOCmd _cmdBlk;

	LeoReadDiskID(&_cmdBlk, &_diskId, &diskMsgQ);
	osRecvMesg(&diskMsgQ, (OSMesg *)&error, OS_MESG_BLOCK);
	return error;
}

s32 diskCheck()
{
	/* Check Disk System Info, do diskReadID first, if not working, do manual work here */
	s32 error;
	error = diskReadID();

	//TODO: Actual checks
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
