#ifndef CONSOLE_BUF_H
#define CONSOLE_BUF_H

#include "kt.h"
// for writes
extern kt_sem writeok;
extern kt_sem writers;

// for reads
extern kt_sem readers;
extern kt_sem nelem;
extern kt_sem nslots;
extern kt_sem consoleWait;

extern int front;
extern int rear;
extern int  readBuffer[256];
void* readThread();

#endif

