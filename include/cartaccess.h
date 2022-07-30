#ifndef __CARTACCESS_H__
#define __CARTACCESS_H__

#include	"64drive.h"
#include	"ff.h"

#define WRITE_ERROR_OK		0
#define WRITE_ERROR_FOPEN	1
#define WRITE_ERROR_FWRITE	2
#define WRITE_ERROR_FCLOSE	3
#define WRITE_ERROR_FSTAT	4

extern OSMesgQueue dmaMessageQ;
extern OSPiHandle *pi_handle;
extern FATFS FatFs;
extern TCHAR DumpPath[256];
extern TCHAR LogPath[256];

extern void initCartPi();
extern FRESULT initFatFs();
extern FRESULT makeUniqueFilename(const TCHAR *path, const TCHAR *ext);
extern FRESULT writeFileROM(const TCHAR *path, const int size, int *proc);
extern FRESULT writeFileRAM(const void *ram, const TCHAR *path, const int size, int *proc);

extern void copyToCartPi(char *src, char *dest, const int len);
extern void copyFromCartPi(char *src, char *dest, const int len);
extern u32 cartRead(u32 addr);
extern void cartWrite(u32 addr, u32 data);

#define io_read(x)	cartRead(x)
#define io_write(x, y)	cartWrite(x, y)

#endif /* __CARTACCESS_H__ */
