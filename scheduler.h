#if !defined(PROC_SCHED_H)
#define PROC_SCHED_H
#define MAX_NUM_FD 64
/*
 * process scheduing constructs
 * 
 */

#include "dllist.h"
#include "simulator_lab2.h"
#include "kt.h"
#include "jrb.h"
struct PCB_struct
{
	int registers[NumTotalRegs];
	struct file_descriptor *fd_table[MAX_NUM_FD];
	int brk_ptr;
	int base;
	int limit;
	int pid;
	int partition_index;
	int exit_code;
	struct PCB_struct* parent;
	kt_sem waiter_sem;
	Dllist waiters;
	JRB children;
};


extern Dllist readyq;
extern struct PCB_struct *Current_pcb;
extern int Idle;
extern int perform_execve(struct PCB_struct *pcb, char* fname, char** argv);
extern void *InitUserProcess(void *arg);
extern void ScheduleProcess();
extern void SysCallReturn(struct PCB_struct *pcb, int return_val);

#endif
