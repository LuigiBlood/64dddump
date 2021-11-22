//
// ci.c
//

#include <ultra64.h>
#include "ci.h"

extern			OSPiHandle *pi_handle;
extern			u32				fail;

extern			unsigned char	fat_message[64];
extern			unsigned char	buffer[512];
extern			unsigned char	fat_buffer[512];

extern			OSIoMesg		dmaIOMessageBuf;
extern			OSMesgQueue		dmaMessageQ;

void ciWait()
{
	long timeout = 0;
	unsigned char buf[4];

	// poll the status register until it's good
	do{
		osEPiReadIo(pi_handle, 0xB8000200, (u32 *)buf); while(osPiGetStatus() != 0);
		timeout++;
		if(timeout == 4000000){
			fail = 1;
			return;
		}
	}while( buf[2] != 0);
}

void ciSectorsToRam(unsigned int ramaddr, unsigned int lba, unsigned int sectors)
{
	ciWait();

	// write LBA to the fpga register
	osEPiWriteIo(pi_handle, 0xB8000210, lba ); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000004, ramaddr); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000218, sectors); while(osPiGetStatus() != 0);

	// write "read sectors to ram" command
	osEPiWriteIo(pi_handle, 0xB8000208, 0x3); while(osPiGetStatus() != 0);

	ciWait();
}

void ciRamToSectors(unsigned int ramaddr, unsigned int lba, unsigned int sectors)
{
	ciWait();

	// write LBA to the fpga register
	osEPiWriteIo(pi_handle, 0xB8000210, lba ); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000004, ramaddr); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000218, sectors); while(osPiGetStatus() != 0);

	// write "write ram to sectors" command
	osEPiWriteIo(pi_handle, 0xB8000208, 0x13); while(osPiGetStatus() != 0);

	ciWait();
}

void ciSetCycleTime(int cycletime)
{
	ciWait();
	// write cycletime to the fpga register
	osEPiWriteIo(pi_handle, 0xB8000000, cycletime/2); while(osPiGetStatus() != 0);
	// write "set cycle time" command
	osEPiWriteIo(pi_handle, 0xB8000208, 0xfd); while(osPiGetStatus() != 0);
	ciWait();
}

void ciSetByteSwap(int byteswap)
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000208, byteswap == 1 ? 0xe1 : 0xe0); while(osPiGetStatus() != 0);
	ciWait();
}

int ciGetRevision()
{
	int ret = 0;

	ciWait();
	osEPiReadIo(pi_handle, 0xB80002FC, (u32 *)&ret); while(osPiGetStatus() != 0);
	ciWait();

	return ret & 0xffff;	// first 16 bits
}

int ciGetMagic()
{
	int ret = 0;

	ciWait();
	osEPiReadIo(pi_handle, 0xB80002EC, (u32 *)&ret); while(osPiGetStatus() != 0);
	ciWait();

	return ret;	
}

void ciSetSave(int savetype)
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000000, savetype); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000208, 0xd0); while(osPiGetStatus() != 0);
	ciWait();
}

void ciStartUpgrade()
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000000, 0x55504752); while(osPiGetStatus() != 0);
	osEPiWriteIo(pi_handle, 0xB8000208, 0xFA); while(osPiGetStatus() != 0);
	ciWait();
}

int ciGetUpgradeStatus()
{
	int ret = 0;
	ciWait();
	osEPiReadIo(pi_handle, 0xB80002F8, (u32 *)&ret); while(osPiGetStatus() != 0);
	ciWait();

	return ret & 0xffff;	// first 16 bits
}

void ciEnableRomWrites()
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000208, 0xf0); while(osPiGetStatus() != 0);
	ciWait();
}

void ciDisableRomWrites()
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000208, 0xf1); while(osPiGetStatus() != 0);
	ciWait();
}

void ciReadSector(unsigned char *buf, unsigned int lba)
{
	ciWait();

	// write LBA to the fpga register
	osEPiWriteIo(pi_handle, 0xB8000210, lba ); while(osPiGetStatus() != 0);
	// write "read sector" command
	osEPiWriteIo(pi_handle, 0xB8000208, 0x01); while(osPiGetStatus() != 0);

	ciWait();

	// DANGER WILL ROBINSON
	// We are DMAing... if we don't write back all cached data, WE'RE FUCKED
	osWritebackDCacheAll();

	// read the 512-byte onchip buffer (m4k on the fpga)
	//osPiRawStartDma(OS_READ, 0xB8000000, (u32)buf, 512); while(osPiGetStatus() != 0);
	osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_READ, 0xB8000000, buf, 512, &dmaMessageQ); //while(osPiGetStatus() != 0);
	(void)osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);

	// inval ALL cache lines
	osInvalDCache((void *)0x80000000, 8192);
}

void ciWriteSector(unsigned char *buf, unsigned int lba)
{
	ciWait();

	osWritebackDCacheAll();
	// write the 512-byte onchip buffer (m4k on the fpga)
	//osPiRawStartDma(OS_WRITE, 0xB8000000, buf, 512); while(osPiGetStatus() != 0);
	osPiStartDma(&dmaIOMessageBuf, OS_MESG_PRI_NORMAL, OS_WRITE, 0xB8000000, buf, 512, &dmaMessageQ); //while(osPiGetStatus() != 0);
	(void)osRecvMesg(&dmaMessageQ, NULL, OS_MESG_BLOCK);
	// write LBA to the fpga register
	osEPiWriteIo(pi_handle, 0xB8000210, lba ); while(osPiGetStatus() != 0);
	// write "write sector" command
	osEPiWriteIo(pi_handle, 0xB8000208, 0x10); while(osPiGetStatus() != 0);

	ciWait();
}