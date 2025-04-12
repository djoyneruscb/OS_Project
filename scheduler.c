#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <errno.h>
#include "dllist.h" 
#include "kt.h"
#include "memory.h"
#include "simulator_lab2.h"
#include "scheduler.h"

Dllist readyq;
extern char *Argv[];
struct PCB_struct *Current_pcb;
int Idle;

int perform_execve(struct PCB_struct *pcb, char* fname, char** argv){
	//unclear if this function should return an integer or a void* + SysCallReturn
	int i;
	int *user_argv;

	pcb->brk_ptr = load_user_program(fname);
	if (pcb->brk_ptr< 0) {
		fprintf(stderr,"Can't load program.\n");
		//exit(1);
		SysCallReturn(pcb, -1);
		return -1;
	}

	User_Base = pcb->base;
	User_Limit = MemorySize / 8;
	pcb->registers[StackReg] = User_Limit - 12;
	user_argv = MoveArgsToStack(pcb->registers,argv,pcb->base);
	InitCRuntime(user_argv,pcb->registers,argv,pcb->base);
	return 0;
}

void *InitUserProcess(void *arg)
{
	//int registers[NumTotalRegs];
    int i;
	struct PCB_struct *pcb;
	char *fname;
	char **argv;

	argv = (char **)arg;
	fname = argv[0];
	
        bzero(main_memory, MemorySize);
	pcb = (struct PCB_struct *)malloc(sizeof(struct PCB_struct));
	Init = (struct PCB_struct *)malloc(sizeof(struct PCB_struct));
	/* Allocating initial Process to contain the first section of memory*/

	pcb->base = 0;
	pcb->limit = MemorySize / 8;
	pcb->pid = get_new_pid();
	pcb->partition_index = 0;
	pcb->waiter_sem = make_kt_sem(0);
	pcb->waiters = new_dllist();
	pcb->children = make_jrb();
	//sentinal parent of the original process.;
	pcb->parent = Init;
	pcb->parent->pid = 0;
	pcb->parent->base = -1;
	pcb->parent->limit = -1;
	pcb->parent->waiter_sem = make_kt_sem(0);
	pcb->parent->waiters = new_dllist();
	pcb->parent->children = make_jrb();
	pcb->fd_table[0] = create_fd(0, NULL, READ_FD, TRUE);
	pcb->fd_table[1] = create_fd(1, NULL, WRITE_FD, TRUE);
	pcb->fd_table[2] = create_fd(2, NULL, WRITE_FD, TRUE);

	jrb_insert_int(pcb->parent->children, pcb->pid, new_jval_v((void *)pcb));

	for(int i = 0; i < NumTotalRegs; i++){
		pcb->parent->registers[i] = -1;
	}

	for(int i = 0; i < NumTotalRegs; i++){
		pcb->registers[0] = 0;
	}
	for(int i = 3; i < MAX_NUM_FD; i++){
		pcb->fd_table[i] = NULL;
	}

	User_Base = 0;
	User_Limit = MemorySize / 8;
	memory_partitions[0] = TRUE;
	pcb->registers[PCReg] = 0;
	pcb->registers[NextPCReg] = 4;
	pcb->registers[StackReg] =  User_Limit -12;
	if( perform_execve(pcb, fname, argv) == -1){
		exit(1);
	}
	dll_append(readyq,new_jval_v((void *)pcb));
	start_timer(10);
	kt_exit(NULL);
	return(NULL);
}

void ScheduleProcess()
{
	if(jrb_empty(Init->children)){
		SYSHalt();
	}	

	if(dll_empty(readyq)){		
		Current_pcb = NULL;
		noop();
	}
	else{
		Current_pcb = (struct PCB_struct*) jval_s(dll_val(dll_last(readyq))); 
		dll_delete_node(dll_last(readyq));
		User_Base = Current_pcb->base;
		run_user_code(Current_pcb->registers);
	}
	return;
}
	
void
SysCallReturn(struct PCB_struct *pcb, int return_val)
{
	//DEBUG('e', "Entering Sys Call Return\n");
	pcb->registers[PCReg] = pcb->registers[NextPCReg];
	pcb->registers[NextPCReg] = pcb->registers[PCReg] + 4;
	pcb->registers[2] = return_val;
	dll_append(readyq, new_jval_v((void*)pcb));
	//DEBUG('e', "exiting Sys Call return\n");
	kt_exit(NULL);
}
