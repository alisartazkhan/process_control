#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
#include <phase4.h>


int ourSleep(void*);

struct Process {
    int PID;
    int mboxID;
    int endSleep;
    struct Process * next;
};


static struct Process shadowProcessTable[MAXPROC];
static struct Process * priorityQueueHead = NULL;

void phase4_init(void) {
    systemCallVec[SYS_SLEEP] = ourSleep;
    //systemCallVec[SYS_TERMREAD] = ourTermRead;
    //systemCallVec[SYS_TERMWRITE] = ourTermWrite;
    //systemCallVec[SYS_DISKREAD] = ourDiskRead;
    //systemCallVec[SYS_DISKWRITE] = ourDiskWrite;
    //systemCallVec[SYS_DISKSIZE] = ourDiskSize;
}

void daemonProcess(){

}


void phase4_start_service_processes(){}

int printProc(struct Process * cur){

    USLOSS_Console(" id: %d endSleep %d -->",cur->PID,cur->endSleep);

}

int dumpPriorityQueue(){
    
    struct Process * cur = priorityQueueHead;
    USLOSS_Console("\nPrinting Queue\n");
    while (cur != NULL){
        printProc(cur);
        cur = cur->next;
    }

    USLOSS_Console("\nEnding p Queue\n\n");

}

int createShadowProcess(int es, int pid) {
    struct Process p = {
        .PID = pid, 
        .mboxID = MboxCreate(0,0),
        .endSleep = es,
        .next = NULL

    };


    shadowProcessTable[p.PID%MAXPROC] = p;
    dumpSP();
    return p.PID;
}

void dumpSP(void){
    USLOSS_Console("\n DUMPING PROC\n");
    for (int i = 0; i < MAXPROC; i++){
         if ( shadowProcessTable[i].PID == 0){
             continue;
         }
        USLOSS_Console("PID: %d endSleep: %d \n",shadowProcessTable[i].PID, shadowProcessTable[i].endSleep);

    }
    USLOSS_Console(" END OF DUMP PROC\n\n");

}

struct Process* getProcess(int pid, int seconds) {
    
    if (pid != shadowProcessTable[pid%MAXPROC].PID){createShadowProcess(seconds,pid); }  
  return &shadowProcessTable[pid%MAXPROC];
}

struct Process* getProcess1(int pid) {
    
  return &shadowProcessTable[pid%MAXPROC];
}

void addToPriorityQueue(int pid){
    struct Process * p = getProcess1(pid);

    int priority = p->endSleep;


    // # If the list is empty or the new node has higher priority than the current head
    if (priorityQueueHead == NULL){
        priorityQueueHead = p;

    }
    // else if (priority < priorityQueueHead->endSleep){
    //     struct Process * temp = priorityQueueHead;
    //     priorityQueueHead = p;
    //     p->next = temp;
    // }

    else{
        struct Process * current = priorityQueueHead;
        while (current->next != NULL && current->next->endSleep <= priority){
            current = current->next;
            }
        p->next = current->next;
        current->next = p;
        
    }
}

int kernSleep(int seconds) {
    return 0;
}

int ourSleep(void *args) {
    int startTime = currentTime();

    USLOSS_Sysargs *sysargs = args; 
    int seconds = sysargs->arg1;

    int endTime = seconds*1000000+startTime;

    struct Process * p = getProcess(getpid(),endTime);


    USLOSS_Console("Inside Our Sleep %d\n", p->endSleep);

    //dumpSP();

    addToPriorityQueue(getpid());
    //addToPriorityQueue(3);


    dumpPriorityQueue();

    //MboxSend(p->mboxID,NULL,0);


//// kernSleep(5) laisdjflasjdf alsdfj  kernSLeep(5)


    return 0;
}

int kernDiskRead(void *diskBuffer, int unit, int track, int first,int sectors, int *status) {

    // Kernel mode disk read implementation
    return 0; // Placeholder return value
}

int kernDiskWrite(void *diskBuffer, int unit, int track, int first, int sectors, int *status) {

    // Kernel mode disk write implementation
    return 0; // Placeholder return value
}

int kernDiskSize(int unit, int *sector, int *track, int *disk) {

    // Kernel mode disk size implementation
    return 0; // Placeholder return value
}

int kernTermRead(char *buffer, int bufferSize, int unitID, int *numCharsRead) {
    // USLOSS_Sysargs *sysargs = args; 

    // Kernel mode terminal read implementation
    return 0; // Placeholder return value
}

int kernTermWrite(char *buffer, int bufferSize, int unitID, int *numCharsRead) {

    // Kernel mode terminal write implementation
    return 0; // Placeholder return value
}