#ifndef _leoctl_h_
#define _leoctl_h_

#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"

extern OSMesgQueue	diskMsgQ;
extern OSMesg		diskMsgBuf[];

extern OSPiHandle	*LeoDiskHandle;
extern OSPiHandle	*DriveRomHandle;

extern u8			blockData[];
extern u8			errorData[];
extern LEODiskID	_diskId;

#define LEO_DRIVE_TYPE_NONE   -1
#define LEO_DRIVE_TYPE_RETAIL 0
#define LEO_DRIVE_TYPE_DEV    1
#define LEO_DRIVE_TYPE_WRITER 2

#define LEO_IPL_TYPE_JP  0xC3
#define LEO_IPL_TYPE_US  0x04

#define LEO_SENSE_GOOD			0
#define LEO_SENSE_SKIPPED_LBA	99

//libleo
extern int leoWait_mecha_cmd_done();

//setup
extern s32 initDisk();
extern void unlockDisk();

//access
//-disk
extern s32 diskReadLBA(s32 lba);
extern s32 diskSkipLBA(s32 lba);
extern s32 diskReadID();
extern s32 diskCheck();
extern s32 diskBreakMotor();
//-ipl
extern void iplCopy(char *src, char *dest, const int len);
extern void iplBlockRead(s32 addr);
extern u32 iplRead(s32 addr);
//-h8
extern u8 h8Read(u16 addr);
extern void h8BlockRead(s32 addr);
//-eeprom
extern u8 eepromRead(u8 addr);
extern void eepromBlockRead();
//-register
extern void registerBlockDump();

//util
extern u32 diskReadRegister(u32 address);
extern void diskWriteRegister(u32 address, u32 data);
extern s32 diskGetLBASectorSize(s32 lba);
extern s32 diskGetLBABlockSize(s32 lba);
extern u32 diskGetLBAOffset(s32 lba);
extern u32 diskDoCommand(u32 cmd, u32 arg);
extern s32 isDiskPresent();
extern s32 isDiskChanged();
extern s32 diskGetDriveType();
extern u32 diskGetIPLType();
extern s32 isRegisterPresent();
extern s32 isIPLROMPresent();

//string
extern char* diskErrorString(s32 error);

#endif
