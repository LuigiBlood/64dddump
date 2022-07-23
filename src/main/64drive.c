#include <ultra64.h>
#include "64drive.h"
#include "cartaccess.h"

void ciWait()
{
	long timeout = 0;
	unsigned char buf[4];

	// poll the status register until it's good
	do
	{
		osEPiReadIo(pi_handle, 0xB8000200, (u32 *)buf); while(osPiGetStatus() != 0);
		timeout++;
		if(timeout == 4000000)
		{
			//fail = 1;
			return;
		}
	}
	while (buf[2] != 0);
}

void ciEnableRomWrites()
{
	ciWait();
	osEPiWriteIo(pi_handle, 0xB8000208, 0xf0); while(osPiGetStatus() != 0);
	ciWait();
}
