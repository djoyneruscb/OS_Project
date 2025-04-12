//#include <unistd.h>
//#include <stdio.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include "simulator_lab2.h"
#include "scheduler.h"
#include "kt.h"
#include "console_buf.h"
#include "memory.h"
#include "dllist.h"



void *do_write(void *arg)
{
	DEBUG('e', "write entry\n");
	struct PCB_struct* pcb = (struct PCB_struct*)arg;
	int fd = (int)pcb->registers[5];
	int length = pcb->registers[7];

	//printf("pcb information:\nId = %d\nbase = %d\nlimit = %d\naddress = %d\n ", pcb->pid, pcb->base, pcb->limit, pcb->registers[6]);
	DEBUG('e', "The value of pcb->register[7] is %d \n", pcb->registers[7]);
	//P_kt_sem(writers);
	//printf("pcb information in the write\npcb->base = %d\npcb->limit = %d\npointer = %d\nUser_Limit = %d\nUser_base = %d\n", pcb->base, pcb->limit, pcb->registers[6] + pcb->base, User_Limit, User_Base);
	
	if(fd < 0 || fd > MAX_NUM_FD){
		DEBUG('e', "ERROR: WRITE bad file NUMBER\n");
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}	
	
	struct file_descriptor *file_descriptor = pcb->fd_table[fd];
	if(file_descriptor == NULL){
		DEBUG('e', "ERROR : Write bad file NUMBER (was --%d--) 2\n", fd);
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}

	if (ValidAddress(pcb->registers[6]) == FALSE) {
			
		DEBUG('e', "ERROR: Write Failure 2\n");
		SysCallReturn(pcb, -EFAULT); //check second
		return NULL;
	}
	else if(pcb->registers[7] < 0){
		DEBUG('e', "ERROR: Write Failure 3");
		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}

	//printf("blocking here\n");
	if(file_descriptor->isConsole == TRUE){ // For writing to the Console. 
		//printf("writing to fd --%d--\n", pcb->registers[5]);
		//DEBUG('e', "Writing to Console!\n");
		int charsWritten = 0;
		//printf("testing 1\n");
		P_kt_sem(writers);
		//printf("testing 2\n");
		while(charsWritten < length){
			//printf("Hello from thread %d\n", pcb->pid);
			console_write(main_memory[pcb->registers[6] + pcb->base +   charsWritten]);
			P_kt_sem(writeok);
			//printf("testing 3\n");

			charsWritten++;
		}


		V_kt_sem(writers);
	}
	else{ // For writing to a pipe
		//DEBUG('e', "Writing to pipe!\n");
		int charsWritten = 0;
		struct pipe* pipe = file_descriptor->pipe;
		if(pipe == NULL){
			DEBUG('e', "ERROR: Attempting to write to non existant PIPE\n");
		}
		while(charsWritten < length){
			//printf("make it here with value --%d--\n", pipe->readers_count);
			if(pipe->readers_count <= 0){
				SysCallReturn(pcb, -EPIPE);
				return NULL;
			}
			P_kt_sem(pipe->empty_slots_sem);
			P_kt_sem(pipe->lock);
			pipe->buffer[pipe->writer_pointer] = main_memory[pcb->registers[6] + pcb->base + charsWritten];
			pipe->writer_pointer = (pipe->writer_pointer + 1) % 256;
			V_kt_sem(pipe->lock);
			charsWritten++;
			V_kt_sem(pipe->fill_slots_sem);
		}
		pipe->buffer[pipe->writer_pointer] = -1;
		//V_kt_sem(pipe->lock);

	}


	DEBUG('e', "write exit\n");	
	SysCallReturn(pcb, length);
	return NULL;
}

void *do_read(void *arg)
{
	DEBUG('e', "Entering Do_Read\n");
	struct PCB_struct* pcb = (struct PCB_struct*)arg;
	int fd = pcb->registers[5];
	
	if (ValidAddress(pcb->registers[6]) == FALSE){
		DEBUG('e', "ERROR: Read Failure 2\n");
		SysCallReturn(pcb, -EFAULT);
		return NULL;
	}
	else if (pcb->registers[7] < 0){
		DEBUG('e', "ERROR: Read Failure 3\n");
		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}

	if(fd < 0 || fd > MAX_NUM_FD){
		DEBUG('e', "ERROR: READ, file descriptor not in valid range.\n");
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}
	
	int length = pcb->registers[7];
	int charsRead = 0;
	int location = pcb->registers[6];
	struct file_descriptor* file_descriptor = pcb->fd_table[fd];
	if(file_descriptor == NULL){
		DEBUG('e', "ERROR: READ, Invalid File Descriptor (was --%d--)\n", pcb->registers[5]);
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}

	if(file_descriptor->isConsole == TRUE){
		P_kt_sem(readers);
		while(charsRead < length){
			if(readBuffer[front] == -1){
				front++;
				DEBUG('e', "Returning here 1\n");
				SysCallReturn(pcb, charsRead);
				return NULL;
			}
			P_kt_sem(nelem);
			main_memory[location + charsRead + pcb->base] = readBuffer[front];
			front++;
			if(front == 256){
				front = 0;
			}
			charsRead++;
			V_kt_sem(nslots);
		}

		V_kt_sem(readers);
	}
	else{
		DEBUG('e', "READ FROM PIPE\n");
		struct pipe *pipe = file_descriptor->pipe;
		if(pipe == NULL){
			DEBUG('e', "ERROR: Attempting to read from a NULL pipe\n");
			SysCallReturn(pcb, -1);\
			return NULL;
		}
		while(charsRead < length){
			//printf("the value of the pipe is:\n refWriters = %d\nrefReaders = %d\n", pipe->writers_count, pipe->readers_count);
			if(pipe->writers_count <= 0){
				SysCallReturn(pcb, charsRead);
				return NULL;
			}
			if(pipe->buffer[pipe->read_pointer] == -1){
				//pipe->read_pointer = (pipe->read_pointer + 1) % 256;
				DEBUG('e', "Returning here 2\n");
				SysCallReturn(pcb, charsRead);
				return NULL;
			}
			//DEBUG('e', "iterations %d\n", charsRead);
			//printf("The value of the semaphore is --%d--\n", kt_getval(pipe->fill_slots_sem));
			P_kt_sem(pipe->fill_slots_sem);
			P_kt_sem(pipe->lock);
			main_memory[location + charsRead + pcb->base] = pipe->buffer[pipe->read_pointer];
			pipe->read_pointer = (pipe->read_pointer + 1) % 256;
			V_kt_sem(pipe->lock);
			charsRead++;
			V_kt_sem(pipe->empty_slots_sem);
		}
		//V_kt_sem(pipe->lock);
	}
	DEBUG('e', "Exiting Do_Read\n");
	SysCallReturn(pcb, charsRead);

	return(NULL);
}

void *do_ioctl(void *arg){
	struct PCB_struct* pcb = (struct PCB_struct*)arg;
	if(pcb->registers[5] != 1){ //first argument must be a 1 for lab 2
		DEBUG('e', "ERROR: IOCTL Failure 1\n");
		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}
	else if (pcb->registers[6] != JOS_TCGETP){ //second argument must be a JOS_TCGEPT for lab 2
		DEBUG('e', "ERROR: IOCTL Failure 2\n");
		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}

	if(ValidAddress(pcb->registers[7]) == FALSE){
		DEBUG('e', "ERROR: IOCTL Failure 3\n");
		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}

	ioctl_console_fill((struct JOStermios*) &main_memory[pcb->registers[7] + pcb->base]);
	SysCallReturn(pcb, 0);
	return NULL;

}

void *do_fstat(void* arg){
	struct PCB_struct* pcb = (struct PCB_struct*)arg;
	int fd = pcb->registers[5];
	if(ValidAddress(pcb->registers[6]) == FALSE){
		DEBUG('e', "ERROR: Fstat Failure 1\n");

		SysCallReturn(pcb, -EINVAL);
		return NULL;

	}
	if(fd == 0){
		DEBUG('e', "fstat is running 2\n");
		stat_buf_fill((struct KOSstat*)  &main_memory[pcb->registers[6] + pcb->base], 1);
	}
	else if(fd == 1 || fd == 2){
		DEBUG('e', "fstat is Running 3\n");
		stat_buf_fill((struct KOSstat*)  &main_memory[pcb->registers[6] + pcb->base], 256);
	}
	else{
		DEBUG('e', "ERROR: IOCTL Failure 2\n");


		SysCallReturn(pcb, -EINVAL);
		return NULL;
	}

 	SysCallReturn(pcb, 0);
	return NULL;
}

void *do_getpagesize(void* arg){ //getpagesize has zero arguments.
	DEBUG('e', "get pagesize is Running Entry\n");
	struct PCB_struct* pcb = (struct PCB_struct*)arg;
	DEBUG('e', "get pagesize exiting\n");
	SysCallReturn(pcb, PageSize);
	return NULL;
}

void *do_sbrk(void *arg) {
	DEBUG('e', "sbrk Entry\n");

    struct PCB_struct* pcb = (struct PCB_struct*)arg;
    int increment = pcb->registers[5];  // Amount to grow heap

    if (ValidAddress(pcb->brk_ptr + increment) == FALSE) {
		DEBUG('e', "ERROR: SBRK\n");
        SysCallReturn(pcb, -EINVAL);
        return NULL;
    }


    int prev_brk = pcb->brk_ptr;
    pcb->brk_ptr += increment;
	DEBUG('e', "sbrk Exit\n");


    SysCallReturn(pcb, prev_brk);
    return NULL;  
}

void* do_execve(void *arg) {
	DEBUG('e', "Execve entry\n");

    struct PCB_struct *pcb = (struct PCB_struct*)arg;
    char* pathname = (char*)&main_memory[pcb->registers[5] + pcb->base];
    char** argv = (char**) &main_memory[pcb->registers[6] + pcb->base];
	//printf("file name is --%s--\n", pathname);
    int num_of_strings = 0;
	while ((char*)argv[num_of_strings] != NULL) {	
		num_of_strings++;
	}
	char* pathCopy = malloc(strlen(pathname) + 1);
	pathCopy[strlen(pathname)] = '\0';
	char** argvCopy;
	if(num_of_strings != 0)
		argvCopy = malloc(num_of_strings * sizeof(char*));
	else{
		argvCopy = NULL;
	}

	if(pathCopy == NULL){
		SysCallReturn(pcb, -1);
		return NULL;
	}

	strcpy(pathCopy, pathname);

	for(int j = 0; j < num_of_strings; j++){
		argvCopy[j] = (char*)malloc(strlen(&main_memory[ pcb->base + (int)argv[j]]) + 1);
		strcpy(argvCopy[j], (char*)&main_memory[pcb->base +(int)argv[j]]);
	}

	argvCopy[num_of_strings] = NULL;

	int j = 0;
	if(argvCopy != NULL)
	while(argvCopy[j] != NULL){
		j++;
	}

	for(int i = 0; i < NumTotalRegs; i++){
		pcb->registers[i] = 0;
	}
	pcb->registers[PCReg] = 0;
	//pcb->registers[NextPCReg] = 4;
	//pcb->registers[StackReg] = User_Limit - 12;

	
	
	int res = perform_execve(pcb, pathname, argvCopy);
	if(res == -1){
		DEBUG('e', "do_execve error. perform_execve returned an error.");
		SysCallReturn(pcb, -1);
		return NULL;
	}
	DEBUG('e', "exceve exit\n");

    SysCallReturn(pcb, 0);
    return NULL;
}

void *do_close(void *arg){
	DEBUG('e', "ENTERING CLOSE\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	int fd = pcb->registers[5];

	if(fd < 0 || fd > MAX_NUM_FD){
		//
		DEBUG('e', "ERROR CLOSE: File Number outside of range\n");
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}

	struct file_descriptor *file_descriptor = pcb->fd_table[fd];
	if(file_descriptor == NULL){
		DEBUG('e', "ERROR CLOSE: Attempting to close Non-existant File Descriptor\n");
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}
	struct pipe *pipe = file_descriptor->pipe;
	if(file_descriptor->isConsole == FALSE){
		if(file_descriptor->flag == WRITE_FD){
			pipe->writers_count--;
			if(pipe->writers_count <= 0 && pipe->readers_count > 0){
			V_kt_sem(pipe->fill_slots_sem);
			}

		}
		else if(file_descriptor->flag == READ_FD){
			pipe->readers_count--;
			if(pipe->readers_count <= 0 && pipe->writers_count > 0){
				V_kt_sem(pipe->empty_slots_sem);
			}
		}
	}
	
	if(pipe != NULL){
		if(pipe->readers_count == 0 && pipe->writers_count == 0){
			free(pipe);
		}
	}


	file_descriptor->references--;

	if(file_descriptor->references == 0){
		//free(file_descriptor);
	}
	
	pcb->fd_table[fd] = NULL;

	DEBUG('e', "EXITING CLOSE\n");
	SysCallReturn(pcb, 0);
	return NULL;
}

void *do_getpid(void *arg){
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	DEBUG('e', "ENTERING GetPid\n");
	DEBUG('e', "Exiting GetPid\n");
	SysCallReturn(pcb, pcb->pid);
	return NULL;
}

void *do_finishFork(void *arg){
	DEBUG('e', "Entering Finish fork\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	DEBUG('e', "Exiting Finish Fork\n");
	SysCallReturn(pcb, 0);
	return NULL;
}

void *do_fork(void *arg) {
    DEBUG('e', "Entering Fork\n");

    struct PCB_struct *origPCB = (struct PCB_struct*)arg;
    int partition_index = get_partition();
    if (partition_index == -1) {
        SysCallReturn(origPCB, -EAGAIN);
        return NULL; 
    }

    memory_partitions[partition_index] = TRUE;

    struct PCB_struct *newPCB = (struct PCB_struct *)malloc(sizeof(struct PCB_struct));
    if (!newPCB) {
        SysCallReturn(origPCB, -ENOMEM);
        return NULL;
    }

    int partition_size = (int)MemorySize / 8;
    newPCB->base = partition_index * partition_size;
    newPCB->pid = get_new_pid();
    //pcb->limit = (partition_index + 1) * partition_size;
    newPCB->limit = MemorySize / 8;
	newPCB->partition_index = partition_index;
    newPCB->parent = origPCB;
    newPCB->waiter_sem = make_kt_sem(0);
    newPCB->waiters = new_dllist();
    newPCB->children = make_jrb();
    newPCB->brk_ptr = origPCB->brk_ptr;  // Consider validating
	for(int i = 0; i < MAX_NUM_FD; i++){
		newPCB->fd_table[i] = origPCB->fd_table[i];
		if(newPCB->fd_table[i] != NULL){
			newPCB->fd_table[i]->references++;
			// if(newPCB->fd_table[i]->pipe != NULL){
			// 	if(newPCB->fd_table[i]->flag == WRITE_FD){
			// 		newPCB->fd_table[i]->pipe->writers_count++;
			// 	}
			// 	else{
			// 		newPCB->fd_table[i]->pipe->readers_count++;
			// 	}
			// }
		}
	}

    jrb_insert_int(origPCB->children, newPCB->pid, new_jval_v((void *)newPCB));

    for (int i = 0; i < NumTotalRegs; i++) {
        newPCB->registers[i] = origPCB->registers[i];
    }

    int copy_size = partition_size;  // Prevent buffer overflows
    for (int i = 0; i < copy_size; i++) {
        main_memory[newPCB->base + i] = main_memory[origPCB->base + i];
    }

    DEBUG('e', "Checkpoint test\n");
    DEBUG('e', "Exiting Fork\n");

    kt_fork(do_finishFork, (void *)newPCB);
    SysCallReturn(origPCB, newPCB->pid);

    return NULL;
}

void *do_gettablesize(void *arg){
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	DEBUG('e', "Entering Get Table Size\n");
	SysCallReturn(pcb, 64);
	return NULL;
}

void *do_exit(void *arg){
	DEBUG('e', "Entry -> Do_Exit\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;

	pcb->exit_code = pcb->registers[5];
	//printf("EXIT CODE IS: --%d--\n", pcb->registers[5]);
	JRB cursor;
	struct PCB_struct *child = NULL;
	jrb_traverse(cursor, pcb->children){
		//printf("DOEs this ever run?");
		child =  jval_v(jrb_val(cursor));
		//DEBUG('e', "parent value is --%d--\n", child->parent->pid);
		child->parent = Init;
		jrb_insert_int(Init->children, child->pid, new_jval_v((void *)child));

	}

	//	if(pcb->parent->pid != 0)
	dll_append(pcb->parent->waiters, new_jval_v((void*)pcb));
	V_kt_sem(pcb->parent->waiter_sem);

	//printf("My PID is --%d--. My Parent PPID is --%d--\n", pcb->pid, pcb->parent->pid);
	jrb_delete_node(jrb_find_int(pcb->parent->children, pcb->pid));
	memory_partitions[pcb->partition_index] = FALSE;
	DEBUG('e', "Exit -> DO_exit\n");
	struct PCB_struct *zombie = NULL;
	Dllist cursor2;
	// dll_traverse(cursor2, pcb->waiters){
	// 	zombie = jval_v(dll_val(cursor2));
	// 	dll_delete_node(cursor2);
	// 	free(zombie);
	// }
	
	// if(pcb->parent->pid == 0){
	// 	destroy_pid(pcb->pid);
	// 	free(pcb);
	// }
	for(int i = 0; i < MAX_NUM_FD; i++){
		struct file_descriptor *fd = pcb->fd_table[i];
		if(fd != NULL){
			fd->references--;
			if(fd->isConsole == FALSE){
				if(fd->flag == WRITE_FD){
					fd->pipe->writers_count--;
				}
				else{
					fd->pipe->readers_count--;
				}
			}
		}
	}
	

	kt_exit();
	return NULL;
}

void *do_wait(void *arg){
	DEBUG('e', "Entering Wait\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	if(jrb_empty(pcb->children) && dll_empty(pcb->waiters)){
		DEBUG('e', "PCB HAS NO CHILDREN\n");
		SysCallReturn(pcb, -ECHILD);
		return NULL;
	}
	DEBUG('e', "information on the PCB is:\npid=%d\nppid=%d\n", pcb->pid, pcb->parent->pid);
	
	P_kt_sem(pcb->waiter_sem);
	struct PCB_struct *child = (struct PCB_struct*)jval_v(dll_val(dll_last(pcb->waiters)));
	dll_delete_node(dll_last(pcb->waiters));
	int ret = child->pid;
	int exit_code = child->exit_code;
	if(child->exit_code != 0){
		//printf("this runs");
		ret = child->exit_code;
	}

	destroy_pid(child->pid);
	DEBUG('e', "Exiting Wait\n");
	//("return value is : %d\n", ret);
	free(child);
	SysCallReturn(pcb, ret);
	return NULL;
}

void *do_getppid(void *arg){
	DEBUG('e', "Entering getppid\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	DEBUG('e', "Exiting getppid\n");
	SysCallReturn(pcb, pcb->parent->pid);
	return NULL;
}

void *do_pipe(void * arg){
	DEBUG('e', "Entering Do_Pipe\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	int *pipefd = (int*)&main_memory[pcb->base + pcb->registers[5]];

	struct pipe* pipe = create_pipe();
	if(pipe == NULL){
		//comeback & implement correctly
		DEBUG('e', "ERROR: DO_PIPE COULDN'T FIND A FILE DESCRIPTOR SLOT\n");

		SysCallReturn(pcb, -1);
		return NULL;
	}


	int fd1 = find_fd_slot((void*)pcb);
	if(fd1 == -1){
		DEBUG('e', "couldn't find valid fd\n");
		//free(pipe);
		return NULL;
	}
	struct file_descriptor* readfd = create_fd(fd1, pipe, READ_FD, FALSE);
	pcb->fd_table[fd1] = readfd;

	int fd2 = find_fd_slot((void*)pcb);
	if(fd2 == -1){
		DEBUG('e', "Couldn't find valid fd2\n");
		//free(pipe);
		return NULL;
	}
	struct file_descriptor* writefd = create_fd(fd2, pipe, WRITE_FD, FALSE);
	pcb->fd_table[fd2] = writefd;

	if(readfd == NULL || writefd == NULL){
		DEBUG('e', "ERROR: fd were NULL in do_pipe\n");
		//free(pipe);
		SysCallReturn(pcb, -1);
		return NULL;
	}

	pipefd[0] = fd1;
	pipefd[1] = fd2;
	//printf("expected file descriptors are --%d-- & --%d--\n", fd1, fd2);
	DEBUG('e', "Exiting Do_Pipe\n");

	SysCallReturn(pcb, 0);
	return NULL;
}

void *do_dup(void *arg){
	DEBUG('e', "Entering Do_dup\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	int oldfd = pcb->registers[5];
	if(oldfd < 0 || oldfd > MAX_NUM_FD){
		// outside the range of possible file descriptors.
		SysCallReturn(pcb, -1);
		return NULL;
	}

	if(pcb->fd_table[oldfd] == NULL){
		SysCallReturn(pcb, -1);
		return NULL;
	}

	int fd = find_fd_slot((void*)pcb);
	if(fd == -1){
		// come back & implement this return correclty;
		DEBUG('e', "ERROR: DO_DUP COULDN'T FIND A FILE DESCRIPTOR SLOT\n");
		SysCallReturn(pcb, -1);
		return NULL;
	}

	pcb->fd_table[fd] = pcb->fd_table[oldfd];
	if(pcb->fd_table[fd]->isConsole == FALSE){
		if(pcb->fd_table[fd]->flag == WRITE_FD){
			pcb->fd_table[fd]->pipe->writers_count++;
		}
		else{
			pcb->fd_table[fd]->pipe->readers_count++;
		}
	}


	DEBUG('e', "Exiting DO_dup\n");
	SysCallReturn(pcb, fd);
	return NULL;
}

void *do_dup2(void *arg){
	DEBUG('e', "Entering DO_DUP2\n");
	struct PCB_struct *pcb = (struct PCB_struct*)arg;
	int oldfd = pcb->registers[5];
	int newfd = pcb->registers[6];

	if( oldfd < 0 || oldfd > MAX_NUM_FD){
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}
	if(newfd < 0 || newfd > MAX_NUM_FD){
		SysCallReturn(pcb, -EBADF);
		return NULL;
	}

	struct file_descriptor *old_file_descritpor = pcb->fd_table[oldfd];
	struct file_descriptor *new_file_descriptor = pcb->fd_table[newfd];
	if(oldfd == newfd){
		SysCallReturn(pcb, newfd);
		return NULL;
	}
	if(old_file_descritpor == new_file_descriptor){
		SysCallReturn(pcb, newfd);
		return NULL;
	}

	if(pcb->fd_table[newfd] != NULL){
		//pcb->registers[5] = newfd;
		//do_close((void*)pcb);
		new_file_descriptor->references--;
		if(new_file_descriptor->isConsole == FALSE){
			if(new_file_descriptor->flag == WRITE_FD){
				new_file_descriptor->pipe->writers_count--;
			}
			else{
				new_file_descriptor->pipe->readers_count--;
			}
		}
		pcb->fd_table[newfd] = NULL;
	}
	pcb->fd_table[newfd] = old_file_descritpor;


	DEBUG('e', "Exiting DO_DUP2\n");
	SysCallReturn(pcb, newfd);
	return NULL;
}

