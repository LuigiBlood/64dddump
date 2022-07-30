#include	<ultra64.h>
#include	"cartaccess.h"

//DMA PI
#define	DMA_QUEUE_SIZE	2
OSMesgQueue	dmaMessageQ;
OSMesg		dmaMessageBuf[DMA_QUEUE_SIZE];

OSPiHandle *pi_handle;

FATFS FatFs;
TCHAR DumpPath[256];
TCHAR LogPath[256];

void initCartPi()
{
	//Create PI Message Queue
	osCreateMesgQueue(&dmaMessageQ, dmaMessageBuf, 1);
	
	pi_handle = osCartRomInit();

	osCreatePiManager((OSPri)OS_MESG_PRI_NORMAL, &dmaMessageQ, dmaMessageBuf, DMA_QUEUE_SIZE);
}

FRESULT initFatFs()
{
	//Mount SD Card
	return f_mount(&FatFs, "", 0);
}

FRESULT makeUniqueFilename(const TCHAR *path, const TCHAR *ext)
{
	FRESULT fr;
	int num = 0;

	//Path as is
	sprintf(DumpPath, "%s.%s", path, ext);
	sprintf(LogPath, "%s.log", path);
	fr = f_stat(DumpPath, 0);
	if (fr == FR_NO_FILE) return fr;
	else if (fr != FR_OK) return fr;

	//Add numbers
	while (num < 500)
	{
		sprintf(DumpPath, "%s_%i.%s", path, num, ext);
		sprintf(LogPath, "%s_%i.log", path, num);
		fr = f_stat(DumpPath, 0);
		if (fr == FR_NO_FILE) return fr;
		else if (fr != FR_OK) return fr;
		num++;
	}
}

FRESULT writeFileROM(const TCHAR *path, const int size, int *proc)
{
	FIL fp;
	FRESULT fr;
	unsigned int fw;

	//Write File
	*proc = WRITE_ERROR_FOPEN;
	fr = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_FWRITE;
	fr = f_write(&fp, (void*)0xB0000000, size, &fw);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_FCLOSE;
	fr = f_close(&fp);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_OK;
	return fr;
}

FRESULT writeFileRAM(const void *ram, const TCHAR *path, const int size, int *proc)
{
	FIL fp;
	FRESULT fr;
	unsigned int fw;

	//Write File
	*proc = WRITE_ERROR_FOPEN;
	fr = f_open(&fp, path, FA_WRITE | FA_CREATE_ALWAYS);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_FWRITE;
	fr = f_write(&fp, ram, size, &fw);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_FCLOSE;
	fr = f_close(&fp);
	if (fr != FR_OK) return fr;

	*proc = WRITE_ERROR_OK;
	return fr;
}

//RAM -> Cart
void copyToCartPi(char *src, char *dest, const int len)
{
	OSIoMesg dmaIoMesgBuf;
	OSMesg dummyMesg;
	dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_NORMAL;
	dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
	dmaIoMesgBuf.dramAddr = src;
	dmaIoMesgBuf.devAddr = (u32)dest;
	dmaIoMesgBuf.size = len;
	osEPiStartDma(pi_handle, &dmaIoMesgBuf, OS_WRITE);
	osRecvMesg(&dmaMessageQ, &dummyMesg, OS_MESG_BLOCK);
}

//Cart -> RAM
void copyFromCartPi(char *src, char *dest, const int len)
{
	OSIoMesg dmaIoMesgBuf;
	OSMesg dummyMesg;
	dmaIoMesgBuf.hdr.pri = OS_MESG_PRI_NORMAL;
	dmaIoMesgBuf.hdr.retQueue = &dmaMessageQ;
	dmaIoMesgBuf.dramAddr = dest;
	dmaIoMesgBuf.devAddr = (u32)src;
	dmaIoMesgBuf.size = len;
	osEPiStartDma(pi_handle, &dmaIoMesgBuf, OS_READ);
	osRecvMesg(&dmaMessageQ, &dummyMesg, OS_MESG_BLOCK);
}

u32 cartRead(u32 addr)
{
	u32 data;
	osEPiReadIo(pi_handle, addr, &data);
	return data;
}

void cartWrite(u32 addr, u32 data)
{
	osEPiWriteIo(pi_handle, addr, data);
}
