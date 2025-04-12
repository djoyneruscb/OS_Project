#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h> 
#include "dllist.h" 
#include "kt.h"
#include "simulator_lab2.h"
#include "console_buf.h"

// this file contains the code and data structures necessary to implement the
// console

//writes
kt_sem writeok;
kt_sem writers;

//reads
kt_sem readers;
kt_sem nelem;
kt_sem nslots;
kt_sem consoleWait;
int  readBuffer[256];
int front;
int rear;

void* readThread(){
  P_kt_sem(consoleWait);
  P_kt_sem(nslots);
  readBuffer[rear] = console_read();
  rear++;

  if(rear == 256){
    rear = 0;
  }
  V_kt_sem(nelem);
}

