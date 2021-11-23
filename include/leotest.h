#include	<ultra64.h>
#include	<PR/leo.h>

extern int leoWait_mecha_cmd_done();	//Wait until Command is processed

void errorcheck(int error);					// Output Error Name

u32 ReadLeoReg(u32 address);				// Read from 64DD Register
void WriteLeoReg(u32 address, u32 data);	// Write to 64DD Register
s32 GetSectorSize(s32 lba);					// Get Sector Size of a specific LBA

int isDiskPresent();		//returns 1 if Disk is inserted
int isDiskChanged();		//returns 1 if Disk has been recently inserted
int GetDriveType();			//returns 1 if Disk Drive is Development

u32 GetDriveIPLRegion();	//return Drive IPL Region Byte (0x9FF00)

extern OSPiHandle *LeoDiskHandle;
extern OSPiHandle *DriveRomHandle;
extern LEOVersion	_leover;
