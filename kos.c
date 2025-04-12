/*
 * kos.c -- starting point for student's os.
 * 
 */
#include <stdlib.h>
#include <unistd.h>
#include "simulator_lab2.h"
#include "scheduler.h"
#include "dllist.h"
#include "kt.h"
#include "console_buf.h"
#include "memory.h"

void KOS()
{
	readyq = new_dllist();
	writeok = make_kt_sem(0);
	writers = make_kt_sem(1);
	readers = make_kt_sem(1);
	nslots = make_kt_sem(256);
	nelem = make_kt_sem(0);
	consoleWait = make_kt_sem(0);
	rear = 0;
	front = 0;
	pid_tree = make_jrb();
	for(int i = 0; i < MAX_PARTITIONS; i++){
		memory_partitions[i] = FALSE;
	}
	if(readyq == NULL){
		exit(1);
	}
	kt_fork(InitUserProcess, (void *)kos_argv);
	kt_joinall();

	ScheduleProcess();
	SYSHalt();
}
