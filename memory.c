#include "memory.h"
#include "scheduler.h"
#include "simulator_lab2.h"
#include "jrb.h"
#include <stdlib.h>
JRB pid_tree;

bool memory_partitions[MAX_PARTITIONS];
struct PCB_struct *Init;


int get_partition(){
    for(int i = 0; i < MAX_PARTITIONS; i++){
        if(memory_partitions[i] == FALSE){
            return i;
        }
    }
    return -1;
}

bool ValidAddress(int addr){
    if(addr + Current_pcb->base < User_Base || addr + Current_pcb->base > User_Base + User_Limit){
        return FALSE;
    }

    return TRUE;
}

int get_new_pid(){
    if(pid_tree == NULL){
        return -1;
    }
    JRB cursor;
    int i = 1;
    cursor = jrb_find_int(pid_tree, i);
    while(cursor != NULL){
        i++;
        cursor = jrb_find_int(pid_tree, i);
    }

    Jval payload = new_jval_i(i);
    jrb_insert_int(pid_tree, i, payload);
    return i;
}

void destroy_pid(int pid){
    JRB cursor = jrb_find_int(pid_tree, pid);
    if(cursor == NULL){
        return;
    }
    else{
        jrb_delete_node(cursor);
    }
}

int find_fd_slot(void* arg){
    struct PCB_struct *pcb = (struct PCB_struct*)arg;
    int i;

    for(i = 3; i < MAX_NUM_FD; i++){
        if(pcb->fd_table[i] == NULL){
            return i;
        }
    }
    return -1;
}

struct file_descriptor* create_fd(int fd, void* pipe, enum flag flag, bool isConsole){
    DEBUG('e', "Entering Create_FD\n");
    if(isConsole == FALSE){
        if(pipe == NULL){
            DEBUG('e', "file_descritpro received an invalid pipe\n");
            return NULL;
        }
    }


    struct file_descriptor *file_descriptor = malloc(sizeof(struct file_descriptor));
    if(file_descriptor == NULL){
        DEBUG('e', "Malloc of file_descriptor fails\n");
        return NULL;
    }
    file_descriptor->fd = fd;
    file_descriptor->pipe = (struct pipe*)pipe;
    file_descriptor->flag = flag;
    file_descriptor->references = 1;
    file_descriptor->isConsole = isConsole;
    DEBUG('e', "Exiting Create_FD\n");
    return file_descriptor;
}

struct pipe *create_pipe(){
    // need to implement correclty
    struct pipe* pipe = malloc(sizeof(struct pipe));
    if(pipe == NULL){
        DEBUG('e', "PIPE malloc was NULL\n");
        return NULL;
    }
    pipe->read_pointer = 0;
    pipe->writer_pointer = 0;
    pipe->readers_count = 1;
    pipe->writers_count = 1;
    pipe->empty_slots_sem = make_kt_sem(1024*8);
    pipe->fill_slots_sem = make_kt_sem(0);
    pipe->lock = make_kt_sem(1);

    return pipe;
}

