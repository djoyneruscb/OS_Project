#ifndef MEMORY_H
#define MEMORY_H
#include "simulator_lab2.h"
#include "jrb.h"
#include "kt.h"
#define MAX_PARTITIONS 8


extern JRB pid_tree;
extern bool memory_partitions[MAX_PARTITIONS];

extern int get_partition();
extern bool ValidAddress(int addr);
extern int get_new_pid();
extern void destroy_pid();
extern struct PCB_struct *Init;

// file desriptor
enum flag{
    READ_FD,
    WRITE_FD,
    CONSOLE_FD,
};

struct file_descriptor{
    int fd;
    struct pipe *pipe;
    enum flag flag; // 1 to use buffer, 2 to use pipe
    bool isConsole;
    int references;
};

extern int find_fd_slot(void* arg);
extern struct file_descriptor *create_fd(int fd, void* pipe, enum flag flag, bool isConsole);


struct pipe{
    int buffer[1024*8];
    int read_pointer;
    int writer_pointer;
    int readers_count;
    int writers_count;
    kt_sem empty_slots_sem;
    kt_sem fill_slots_sem;
    kt_sem lock;
};

extern struct pipe* create_pipe();

#endif