/*
 * exception.c -- stub to handle user mode exceptions, including system calls
 * 
 * Everything else core dumps.
 * 
 * Copyright (c) 1992 The Regents of the University of California. All rights
 * reserved.  See copyright.h for copyright notice and limitation of
 * liability and disclaimer of warranty provisions.
 */
#include <stdlib.h>

#include "simulator_lab2.h"
#include "scheduler.h"
#include "kt.h"
#include "syscall.h"
#include "console_buf.h"
#include "memory.h"
#include "scheduler.h"
//#include "kt.h"
//#include "console_buf.h"


void
exceptionHandler(ExceptionType which)
{	
	examine_registers(Current_pcb->registers);
	int type = Current_pcb->registers[4];
	int r5 = Current_pcb->registers[5];
	/*
	 * for system calls type is in r4, arg1 is in r5, arg2 is in r6, and
	 * arg3 is in r7 put result in r2 and don't forget to increment the
	 * pc before returning!
	 */

	switch (which) {
	case SyscallException:
		/* the numbers for system calls is in <sys/syscall.h> */
		switch (type) {
		case 0:
			/* 0 is our halt system call number */
			DEBUG('e', "Halt initiated by user program\n");
			SYSHalt();
		case SYS_exit:
			kt_fork(do_exit, (void*)Current_pcb);
			break;
		case SYS_write:
			kt_fork(do_write, (void*)Current_pcb);
			break;
		case SYS_read:
			kt_fork(do_read, (void*)Current_pcb);
			break;
		case SYS_ioctl:
			kt_fork(do_ioctl,(void*)Current_pcb);
			break;
		case SYS_fstat:
			kt_fork(do_fstat,(void*)Current_pcb);
			break;
		case SYS_getpagesize:
			kt_fork(do_getpagesize, (void*) Current_pcb);
			break;
		case SYS_sbrk:
		 	kt_fork(do_sbrk, (void*)Current_pcb);
		 	break;
		case SYS_execve:
			kt_fork(do_execve, (void*)Current_pcb);
			break;
		case SYS_close:
			kt_fork(do_close, (void*) Current_pcb);
			break;
		case SYS_getpid:
			kt_fork(do_getpid, (void*) Current_pcb);
			break;
		case SYS_fork:
			kt_fork(do_fork, (void*) Current_pcb);
			break;
		case SYS_getdtablesize:
			kt_fork(do_gettablesize, (void*) Current_pcb);
			break;
		case SYS_getppid:
			kt_fork(do_getppid, (void*)Current_pcb);
			break;
		case SYS_wait:
			kt_fork(do_wait, (void*) Current_pcb);
			break;
		case SYS_pipe:
			kt_fork(do_pipe, (void*) Current_pcb);
			break;
		case SYS_dup:
			kt_fork(do_dup, (void*) Current_pcb);
			break;
		case SYS_dup2:
			kt_fork(do_dup2, (void*) Current_pcb);
			break;
		default:
			DEBUG('e', "Unknown system call\n");
			printf("Unknown call was %d\n", type);
			SYSHalt();
			break;
		}
		break;
	case PageFaultException:
		DEBUG('e', "Exception PageFaultException\n");
		break;
	case BusErrorException:
		DEBUG('e', "Exception BusErrorException\n");
		break;
	case AddressErrorException:
		DEBUG('e', "Exception AddressErrorException\n");
		break;
	case OverflowException:
		DEBUG('e', "Exception OverflowException\n");
		break;
	case IllegalInstrException:
		DEBUG('e', "Exception IllegalInstrException\n");
		break;
	default:
		printf("Unexpected user mode exception %d %d\n", which, type);
		exit(1);
	}
	kt_joinall();
	ScheduleProcess();
}

void
interruptHandler(IntType which)
{
	if(Current_pcb != NULL){
		examine_registers(Current_pcb->registers);
		dll_append(readyq,new_jval_s((char*)Current_pcb));
	}


	switch (which) {
	case ConsoleReadInt:
		V_kt_sem(consoleWait);
		kt_fork(readThread, NULL);
		//DEBUG('e', "ConsoleReadInt interrupt\n");
		break;
	case ConsoleWriteInt:
		V_kt_sem(writeok);
		//DEBUG('e', "ConsoleWriteInt interrupt\n");
		break;
	case TimerInt:
		//ScheduleProcess();
		break;
	default:

		DEBUG('e', "Unknown interrupt\n");
		break;
	}
	kt_joinall();
	ScheduleProcess();
}
