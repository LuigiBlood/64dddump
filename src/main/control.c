#include	<ultra64.h>
#include	"control.h"

//Controller Management
OSMesgQueue		pifMesgQueue;
OSMesg			dummyMessage;
OSMesg			pifMesgBuf[NUM_MESSAGE];

static OSContStatus	contstat[MAXCONTROLLERS];
static OSContPad	contdata[MAXCONTROLLERS];
static s32			controller[MAXCONTROLLERS];
OSPfs				pfs0;
s32					cont_no;
u8					bitpattern;
u16					button, oldbutton, newbutton;

void initController()
{
	u32 i;

	cont_no = 0;
	button = 0;
	oldbutton = 0;
	newbutton = 0;

	//Create PIF/SI Message Queue
	osCreateMesgQueue(&pifMesgQueue, pifMesgBuf, NUM_MESSAGE);
	osSetEventMesg(OS_EVENT_SI, &pifMesgQueue, dummyMessage);
	
	//Initialize & Check Controllers
	osContInit(&pifMesgQueue, &bitpattern, &contstat[0]);
	for (i = 0; i < MAXCONTROLLERS; i++)
	{
		if ((bitpattern & (1<<i)) &&
			((contstat[i].type & CONT_TYPE_MASK) == CONT_TYPE_NORMAL))
		{
			controller[i] = CONT_VALID;
		} else {
			controller[i] = CONT_INVALID;
		}
	}
}

void updateController()
{
	osContStartReadData(&pifMesgQueue);
	osRecvMesg(&pifMesgQueue, NULL, OS_MESG_BLOCK);
	osContGetReadData(&contdata[0]);
	
	if (contdata[cont_no].errno & CONT_NO_RESPONSE_ERROR)
	{
		button = oldbutton;
	} else {
		oldbutton = button;
		button = contdata[cont_no].button;
	}
	
	newbutton = ~oldbutton & button;
	//Use newbutton for global
}

u16 readControllerPressed()
{
	return newbutton;
}

u16 readControllerHold()
{
	return button;
}
