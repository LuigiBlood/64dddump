#include	<ultra64.h>
#include	"cart.h"

//DMA PI
#define	DMA_QUEUE_SIZE	2
OSMesgQueue	dmaMessageQ;
OSMesg		dmaMessageBuf[DMA_QUEUE_SIZE];

OSPiHandle *pi_handle;

void initCartPi()
{
	//Create PI Message Queue
	osCreateMesgQueue(&dmaMessageQ, dmaMessageBuf, 1);
	
	pi_handle = osCartRomInit();

	osCreatePiManager((OSPri)OS_MESG_PRI_NORMAL, &dmaMessageQ, dmaMessageBuf, DMA_QUEUE_SIZE);
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
