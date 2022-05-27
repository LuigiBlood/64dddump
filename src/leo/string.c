#include	<ultra64.h>
#include	<PR/leo.h>
#include	"leohax.h"
#include	"leoctl.h"

char* diskErrorString(s32 error)
{
	switch (error)
	{
		case LEO_SENSE_NO_ADDITIONAL_SENSE_INFOMATION:
			return "GOOD";
			break;
		case LEO_SENSE_DRIVE_NOT_READY:
			return "DRIVE_NOT_READY";
			break;
		case LEO_SENSE_DIAGNOSTIC_FAILURE:
			return "DIAGNOSTIC_FAILURE";
			break;
		case LEO_SENSE_COMMAND_PHASE_ERROR:
			return "COMMAND_PHASE_ERROR";
			break;
		case LEO_SENSE_DATA_PHASE_ERROR:
			return "DATA_PHASE_ERROR";
			break;
		case LEO_SENSE_REAL_TIME_CLOCK_FAILURE:
			return "REAL_TIME_CLOCK_FAILURE";
			break;
		case LEO_SENSE_UNKNOWN_FORMAT:
			return "UNKNOWN_FORMAT";
			break;
		case LEO_SENSE_NO_SEEK_COMPLETE:
			return "NO_SEEK_COMPLETE";
			break;
		case LEO_SENSE_WRITE_FAULT:
			return "WRITE_FAULT";
			break;
		case LEO_SENSE_UNRECOVERED_READ_ERROR:
			return "UNRECOVERED_READ_ERROR";
			break;
		case LEO_SENSE_NO_REFERENCE_POSITION_FOUND:
			return "NO_REFERENCE_POSITION_FOUND";
			break;
		case LEO_SENSE_TRACK_FOLLOWING_ERROR:
			return "TRACK_FOLLOWING_ERROR";
			break;
		case LEO_SENSE_INVALID_COMMAND_OPERATION_CODE:
			return "INVALID_COMMAND_OPERATION_CODE";
			break;
		case LEO_SENSE_LBA_OUT_OF_RANGE:
			return "LBA_OUT_OF_RANGE";
			break;
		case LEO_SENSE_WRITE_PROTECT_ERROR:
			return "WRITE_PROTECT_ERROR";
			break;
		case LEO_SENSE_COMMAND_TERMINATED:
			return "COMMAND_TERMINATED";
			break;
		case LEO_SENSE_QUEUE_FULL:
			return "QUEUE_FULL";
			break;
		case LEO_SENSE_ILLEGAL_TIMER_VALUE:
			return "ILLEGAL_TIMER_VALUE";
			break;
		case LEO_SENSE_WAITING_NMI:
			return "WAITING_NMI";
			break;
		case LEO_SENSE_DEVICE_COMMUNICATION_FAILURE:
			return "DEVICE_COMMUNICATION_FAILURE";
			break;
		case LEO_SENSE_MEDIUM_NOT_PRESENT:
			return "MEDIUM_NOT_PRESENT";
			break;
		case LEO_SENSE_POWERONRESET_DEVICERESET_OCCURED:
			return "POWERONRESET_DEVICERESET_OCCURED";
			break;
		case LEO_SENSE_MEDIUM_MAY_HAVE_CHANGED:
			return "MEDIUM_MAY_HAVE_CHANGED";
			break;
		case LEO_SENSE_EJECTED_ILLEGALLY_RESUME:
			return "EJECTED_ILLEGALLY_RESUME";
			break;
		default:
			return "UNKNOWN SENSE";
			break;
	}
}
