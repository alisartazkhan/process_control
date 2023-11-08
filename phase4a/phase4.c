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

int GLOBAL_CLOCK_TICKS = 0;
int DAEMON_CLOCK_PID = -1;
int DAEMON_TERMINAL0_PID = -1;
int DAEMON_TERMINAL1_PID = -1;
int DAEMON_TERMINAL2_PID = -1;
int DAEMON_TERMINAL3_PID = -1;
int DAEMON_DISK0_PID = -1;
int DAEMON_DISK1_PID = -1;


int TERM_MBOX_ARRAY[4];



int ourSleep(void*);
int ourTermRead(void*);

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
    systemCallVec[SYS_TERMREAD] = ourTermRead;
    //systemCallVec[SYS_TERMWRITE] = ourTermWrite;
    //systemCallVec[SYS_DISKREAD] = ourDiskRead;
    //systemCallVec[SYS_DISKWRITE] = ourDiskWrite;
    //systemCallVec[SYS_DISKSIZE] = ourDiskSize;
}

/*
  Method enables interrupts using the psr.
  
  Args: NONE
  Returns: void
*/
void enableInterrupts() {

  unsigned int psr = USLOSS_PsrGet();
  psr |= USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);

}

/*
  Method disables interrupts using the psr.
  
  Args: NONE
  Returns: void
*/
void disableInterrupts() {
  unsigned int psr = USLOSS_PsrGet();
  psr &= ~USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);
}

void daemonProcessMain(){
    // USLOSS_Console("INSIDE THE DAEMON MAIN()\n");
    int status = -1;
    while (1) {
        // USLOSS_Console("INSIDE THE DAEMON MAIN() loop\n");
        waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        // USLOSS_Console("GLOBAL CLOCK TICKS: %d\n", GLOBAL_CLOCK_TICKS);
        GLOBAL_CLOCK_TICKS ++;
        removeFromPriorityQueue();
    }


}

void termMain(int unit){
    int status;
    char buffer[MAXLINE];
    int i=0;
    int newLine = '\n';
    while(1) {
        waitDevice(USLOSS_TERM_DEV, unit, &status);
        // debug("status: %d", status);
        int received = USLOSS_TERM_STAT_RECV(status);
        if(received) {
            int ch =  USLOSS_TERM_STAT_CHAR(status);
            debug("received: %d recv status: %d",  ch, received);
            buffer[i] = (char)ch;   i++;
            if(ch == newLine || i == MAXLINE) { // time to deliver
                MboxSend(TERM_MBOX_ARRAY[unit], (void*)buffer, i);
                //clear buffer
                buffer[0] = '\0'; i =0;
            }
            
            
        }

    }
    return 1;
}
void termMain2(int unit){
    // enableInterrupts();
    USLOSS_Console("INSIDE THE TERM MAIN() | UNIT: %d\n", unit);
    int status = -1;

    char *buffer = (char *)calloc(MAXLINE, sizeof(char));
    int bufferIndex = 0;
    while (1) {
        USLOSS_Console("INSIDE THE TERM MAIN() LOOP | UNIT: %d\n", unit);
        waitDevice(USLOSS_TERM_DEV, unit, &status);
        USLOSS_Console("After waitDevice()\n", unit);

        char character = (char) USLOSS_TERM_STAT_CHAR(status);
        buffer[bufferIndex] = character;
        bufferIndex ++;
        USLOSS_Console("CHARS READ: %d | String: %s\n", bufferIndex, buffer);

        if (character == '\n'|| bufferIndex == MAXLINE){
            MboxCondSend(TERM_MBOX_ARRAY[unit], buffer, bufferIndex);
            free(buffer);
            buffer = (char *)calloc(MAXLINE, sizeof(char));
            bufferIndex = 0;
        } 

        // add char received from waitDevice to buffer
        // if buffer is now full or gets to newLine, cond send to mb
        // reset buffer and words
        
    }


}


void removeFromPriorityQueue(){
    int curTime = GLOBAL_CLOCK_TICKS * 100;

    while (priorityQueueHead != NULL && curTime >= priorityQueueHead->endSleep){
        struct Process* p = priorityQueueHead;
        priorityQueueHead = priorityQueueHead -> next;
        int mboxID = p->mboxID;
        MboxRecv(mboxID, NULL, NULL);
    }
}


void phase4_start_service_processes(){
    int ctrl = 0x1; // 0 is the initial control value, you may use the current value
    ctrl |= 0x2;
    ctrl |= 0x4;
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 0, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 2, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 3, (void*)(long)ctrl) == USLOSS_DEV_OK);


    DAEMON_CLOCK_PID = fork1("DAEMON", daemonProcessMain, NULL, USLOSS_MIN_STACK, 1);
    // DAEMON_TERMINAL0_PID = fork1("TERM0", termMain2, 0, USLOSS_MIN_STACK, 1);
    // DAEMON_TERMINAL1_PID = fork1("TERM1", termMain2, 1, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL2_PID = fork1("TERM2", termMain2, 2, USLOSS_MIN_STACK, 1);
    // DAEMON_TERMINAL3_PID = fork1("TERM3", termMain2, 3, USLOSS_MIN_STACK, 1);
    DAEMON_DISK0_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);
    DAEMON_DISK1_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);


    TERM_MBOX_ARRAY[0] = MboxCreate(10, MAXLINE);
    TERM_MBOX_ARRAY[1] = MboxCreate(10, MAXLINE);
    TERM_MBOX_ARRAY[2] = MboxCreate(10, MAXLINE);
    TERM_MBOX_ARRAY[3] = MboxCreate(10, MAXLINE);
    
}

void printProc(struct Process * cur){

    USLOSS_Console("[PID: %d | MBOXID: %d | ENDSLEEP %d] -->",cur->PID, cur->mboxID, cur->endSleep);

}

int dumpPriorityQueue(){
    
    struct Process * cur = priorityQueueHead;
    USLOSS_Console("\nDUMPING PQueue -------------\n\n");
    while (cur != NULL){
        printProc(cur);
        cur = cur->next;
    }

    USLOSS_Console("\n\nEnding PQueue ------------\n\n");

}

void dumpSP(void){
    USLOSS_Console("\nDUMPING SHADOW PROCTABLE ---------------------------\n\n");
    for (int i = 0; i < MAXPROC; i++){
        if ( shadowProcessTable[i].PID == 0){
            continue;
        }
        printProc(&shadowProcessTable[i]);
        USLOSS_Console("\n");
    }
    USLOSS_Console("\nEND OF DUMP PROC -----------------------\n\n");

}
int createShadowProcess(int es, int pid) {
    struct Process p = {
        .PID = pid, 
        .mboxID = MboxCreate(0,0),
        .endSleep = es,
        .next = NULL

    };
    shadowProcessTable[p.PID%MAXPROC] = p;
    // USLOSS_Console("Created new proc: PID %d | MBOXID: %d\n", p.PID, p.mboxID);
    // dumpSP();
    return p.PID;
}



struct Process* getProcess(int pid, int seconds) {
    
    if (pid != shadowProcessTable[pid%MAXPROC].PID){
        // USLOSS_Console("CREATING SHADOW PROC\n");
        createShadowProcess(seconds,pid); 
    }

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


int ourSleep(void *args) {

// dumpProcesses();
// dumpSP();
    int startTime = GLOBAL_CLOCK_TICKS * 100;

    USLOSS_Sysargs *sysargs = args; 
    int seconds = sysargs->arg1;
    // USLOSS_Console("Inside Our Sleep %d\n", p->endSleep);


    int endTime = seconds*1000 + startTime;

    struct Process * p = getProcess(getpid(), endTime);
    addToPriorityQueue(getpid());
    // dumpPriorityQueue();
    
    // USLOSS_Console("BLOCKING PROC\n");
    MboxSend(p->mboxID, NULL, NULL);



    return 0;
}


// int ourTermWrite(void* args){
//     USLOSS_Sysargs *sysargs = args; 

// }


int ourTermRead(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int bufferSize = sysargs->arg2;
    char *buffer = sysargs->arg1;
    int unitID = sysargs->arg3;

    // Check for invalid terminal 
    if(unitID < 0 || unitID > 3) {
        sysargs->arg4 = -1;
        return -1;
    }
    // MboxSend(TERM_MBOX_ARRAY[unitID], "hello there", strlen("hello there")+1);
    
    // memset(buffer, 'x', sizeof(bufferSize)-1);
    // buffer[bufferSize] = '\0';
    MboxRecv(TERM_MBOX_ARRAY[unitID], buffer, bufferSize);
    USLOSS_Console("MESSAGE: %s\n", buffer);

    sysargs->arg2 = strlen(buffer);
    sysargs->arg4 = 0;   
    return 0;
}
int kernSleep(int seconds) {
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