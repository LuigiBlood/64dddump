void errorcheck(int error)
{
	setcolor(255,0,0);
	switch (error)
	{
		case LEO_ERROR_GOOD:
			setcolor(0,255,0);
			draw_puts("LEO_ERROR_GOOD                            ");
			break;
		case LEO_ERROR_DRIVE_NOT_READY:
			draw_puts("LEO_ERROR_DRIVE_NOT_READY                 ");
			break;
		case LEO_ERROR_DIAGNOSTIC_FAILURE:
			draw_puts("LEO_ERROR_DIAGNOSTIC_FAILURE              ");
			break;
		case LEO_ERROR_COMMAND_PHASE_ERROR:
			draw_puts("LEO_ERROR_COMMAND_PHASE_ERROR             ");
			break;
		case LEO_ERROR_DATA_PHASE_ERROR:
			draw_puts("LEO_ERROR_DATA_PHASE_ERROR                ");
			break;
		case LEO_ERROR_REAL_TIME_CLOCK_FAILURE:
			draw_puts("LEO_ERROR_REAL_TIME_CLOCK_FAILURE         ");
			break;
		case LEO_ERROR_BUSY:
			draw_puts("LEO_ERROR_BUSY                            ");
			break;
		case LEO_ERROR_INCOMPATIBLE_MEDIUM_INSTALLED:
			draw_puts("LEO_ERROR_INCOMPATIBLE_MEDIUM_INSTALLED   ");
			break;
		case LEO_ERROR_NO_SEEK_COMPLETE:
			draw_puts("LEO_ERROR_NO_SEEK_COMPLETE                ");
			break;
		case LEO_ERROR_WRITE_FAULT:
			draw_puts("LEO_ERROR_WRITE_FAULT                     ");
			break;
		case LEO_ERROR_UNRECOVERED_READ_ERROR:
			draw_puts("LEO_ERROR_UNRECOVERED_READ_ERROR          ");
			break;
		case LEO_ERROR_NO_REFERENCE_POSITION_FOUND:
			draw_puts("LEO_ERROR_NO_REFERENCE_POSITION_FOUND     ");
			break;
		case LEO_ERROR_TRACK_FOLLOWING_ERROR:
			draw_puts("LEO_ERROR_TRACK_FOLLOWING_ERROR           ");
			break;
		case LEO_ERROR_INVALID_COMMAND_OPERATION_CODE:
			draw_puts("LEO_ERROR_INVALID_COMMAND_OPERATION_CODE  ");
			break;
		case LEO_ERROR_LBA_OUT_OF_RANGE:
			draw_puts("LEO_ERROR_LBA_OUT_OF_RANGE                ");
			break;
		case LEO_ERROR_WRITE_PROTECT_ERROR:
			draw_puts("LEO_ERROR_WRITE_PROTECT_ERROR             ");
			break;
		case LEO_ERROR_COMMAND_TERMINATED:
			draw_puts("LEO_ERROR_COMMAND_TERMINATED              ");
			break;
		case LEO_ERROR_QUEUE_FULL:
			draw_puts("LEO_ERROR_QUEUE_FULL                      ");
			break;
		case LEO_ERROR_ILLEGAL_TIMER_VALUE:
			draw_puts("LEO_ERROR_ILLEGAL_TIMER_VALUE             ");
			break;
		case LEO_ERROR_WAITING_NMI:
			draw_puts("LEO_ERROR_WAITING_NMI                     ");
			break;
		case LEO_ERROR_DEVICE_COMMUNICATION_FAILURE:
			draw_puts("LEO_ERROR_DEVICE_COMMUNICATION_FAILURE    ");
			break;
		case LEO_ERROR_MEDIUM_NOT_PRESENT:
			draw_puts("LEO_ERROR_MEDIUM_NOT_PRESENT              ");
			break;
		case LEO_ERROR_POWERONRESET_DEVICERESET_OCCURED:
			draw_puts("LEO_ERROR_POWERONRESET_DEVICERESET_OCCURED");
			break;
		case LEO_ERROR_RAMPACK_NOT_CONNECTED:
			draw_puts("LEO_ERROR_RAMPACK_NOT_CONNECTED           ");
			break;
		case LEO_ERROR_NOT_BOOTED_DISK:
			draw_puts("LEO_ERROR_NOT_BOOTED_DISK                 ");
			break;
		case LEO_ERROR_DIDNOT_CHANGED_DISK_AS_EXPECTED:
			draw_puts("LEO_ERROR_DIDNOT_CHANGED_DISK_AS_EXPECTED ");
			break;
		case LEO_ERROR_MEDIUM_MAY_HAVE_CHANGED:
			draw_puts("LEO_ERROR_MEDIUM_MAY_HAVE_CHANGED         ");
			break;
		case LEO_ERROR_RTC_NOT_SET_CORRECTLY:
			draw_puts("LEO_ERROR_RTC_NOT_SET_CORRECTLY           ");
			break;
		case LEO_ERROR_EJECTED_ILLEGALLY_RESUME:
			draw_puts("LEO_ERROR_EJECTED_ILLEGALLY_RESUME        ");
			break;
		case LEO_ERROR_DIAGNOSTIC_FAILURE_RESET:
			draw_puts("LEO_ERROR_DIAGNOSTIC_FAILURE_RESET        ");
			break;
		case LEO_ERROR_EJECTED_ILLEGALLY_RESET:
			draw_puts("LEO_ERROR_EJECTED_ILLEGALLY_RESET         ");
			break;
		case -1:
			draw_puts("                                          ");
			break;
		default:
			draw_puts("UNKNOWN ERROR                             ");
			break;
	}
}

u32 ReadLeoReg(u32 address)
{
  u32 data = 0;
  //osPiReadIo(address, &data);
  osEPiReadIo(LeoDiskHandle, address, &data);
  return data;
}

void WriteLeoReg(u32 address, u32 data)
{
  //osPiWriteIo(address, data);
  osEPiWriteIo(LeoDiskHandle, address, data);
}

s32 GetSectorSize(s32 lba)
{
  s32 bytes = 0;
  LeoLBAToByte(lba, 1, &bytes);
  return (bytes / USR_SECS_PER_BLK);
}
