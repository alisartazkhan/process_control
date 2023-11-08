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



int TERM_LOCK_MBOX_ARRAY[4];
int TERM_READ_MBOX_ARRAY[4];
int TERM_WRITE_MBOX_ARRAY[4];
int READ_BUFFER_SIZE = -1;


int ourSleep(void*);
int ourTermRead(void*);
int ourTermWrite(void*);

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
    systemCallVec[SYS_TERMWRITE] = ourTermWrite;
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
        // USLOSS_Console("INSIDE THE DAEMON MAIN() louroop\n");
        waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        //USLOSS_Console("Wait Device returned ");
        //dumpPriorityQueue();
        //USLOSS_Console("GLOBAL CLOCK TICKS: %d\n", GLOBAL_CLOCK_TICKS);
        GLOBAL_CLOCK_TICKS ++;

        if (GLOBAL_CLOCK_TICKS % 10 == 0 && GLOBAL_CLOCK_TICKS < 4 * 10){
            //USLOSS_Console("Global Clock tick: %d\n",GLOBAL_CLOCK_TICKS/10);
            // dumpPriorityQueue();
        }
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
                MboxSend(TERM_READ_MBOX_ARRAY[unit], (void*)buffer, i);
                //clear buffer
                buffer[0] = '\0'; i =0;
            }
            
            
        }

    }
    return 1;
}
void termMain2(int unit){
    int status = -1;

    char *buffer = (char *)calloc(MAXLINE, sizeof(char));
    int bufferIndex = 0;
    // char* writeBuffer = (char *)calloc(MAXLINE, sizeof(char));
    // memset(writeBuffer, '\0', MAXLINE);  // Fill dataBuffer with a's
    // writeBuffer[254] = '\0';

    while (1) {
        int transmitBit = USLOSS_TERM_STAT_XMIT(status);
        if (transmitBit == USLOSS_DEV_READY){
            MboxCondSend(TERM_WRITE_MBOX_ARRAY[unit],NULL, 0);

            // MboxCondSend(TERM_WRITE_MBOX_ARRAY[unit], NULL, 0);
        } else if (transmitBit == USLOSS_DEV_ERROR){
            USLOSS_Console("ERROR: theres sth wrong with writing the registers\n");
            USLOSS_Halt(1);
        }

        //USLOSS_Console("INSIDE THE TERM MAIN() LOOP | UNIT: %d\n", unit);
        //USLOSS_Console("last thing to happen for this unit: %d\n",unit);
        waitDevice(USLOSS_TERM_DEV, unit, &status);
        //USLOSS_Console("After waitDevice()\n", unit);
        int readBit = USLOSS_TERM_STAT_RECV(status);
        if (readBit == USLOSS_DEV_BUSY){
            char character = (char) USLOSS_TERM_STAT_CHAR(status);
            buffer[bufferIndex] = character;
            bufferIndex ++;
            //USLOSS_Console("CHARS READ: %d | String: %s\n\n", bufferIndex, buffer);

            if (character == '\n'|| bufferIndex == MAXLINE || bufferIndex == READ_BUFFER_SIZE){
                //USLOSS_Console("about to end maybe\n");
                buffer[bufferIndex] = '\0';
                // USLOSS_Console("String: %s\n", buffer);
                MboxCondSend(TERM_READ_MBOX_ARRAY[unit], buffer, bufferIndex);
                free(buffer);
                buffer = (char *)calloc(MAXLINE, sizeof(char));
                bufferIndex = 0;
            } 
        } else if (readBit == USLOSS_DEV_ERROR){
            USLOSS_Console("ERROR: theres sth wrong with reading the registers\n");
            USLOSS_Halt(1);
        }

    }
}


void removeFromPriorityQueue(){
    int curTime = GLOBAL_CLOCK_TICKS * 100;
    while (priorityQueueHead != NULL && curTime >= priorityQueueHead->endSleep){
        //USLOSS_Console("Inside REMOVE PQUEUE -------------------------");
        //dumpPriorityQueue();

        struct Process* p = priorityQueueHead;
        priorityQueueHead = priorityQueueHead -> next;
        //dumpPriorityQueue();
        int mboxID = p->mboxID;
        MboxRecv(mboxID, NULL, NULL);
    }
}


void phase4_start_service_processes(){
    int ctrl = 0x6; // 0 is the initial control value, you may use the current value
    // ctrl |= 0x2;
    // ctrl |= 0x7;
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 0, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 2, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 3, (void*)(long)ctrl) == USLOSS_DEV_OK);




    DAEMON_CLOCK_PID = fork1("DAEMON", daemonProcessMain, NULL, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL0_PID = fork1("TERM0", termMain2, 0, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL1_PID = fork1("TERM1", termMain2, 1, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL2_PID = fork1("TERM2", termMain2, 2, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL3_PID = fork1("TERM3", termMain2, 3, USLOSS_MIN_STACK, 1);
    DAEMON_DISK0_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);
    DAEMON_DISK1_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);

    TERM_LOCK_MBOX_ARRAY[0] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[1] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[2] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[3] = MboxCreate(1, 0);


    TERM_READ_MBOX_ARRAY[0] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[1] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[2] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[3] = MboxCreate(10, MAXLINE);

    TERM_WRITE_MBOX_ARRAY[0] = MboxCreate(10, MAXLINE);
    TERM_WRITE_MBOX_ARRAY[1] = MboxCreate(10, MAXLINE);
    TERM_WRITE_MBOX_ARRAY[2] = MboxCreate(10, MAXLINE);
    TERM_WRITE_MBOX_ARRAY[3] = MboxCreate(10, MAXLINE);
    
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

//clean up mailbox
//update endtime

struct Process* getProcess(int pid, int seconds) {
    
    if (pid != shadowProcessTable[pid%MAXPROC].PID){
        // USLOSS_Console("CREATING SHADOW PROC\n");
        createShadowProcess(seconds,pid); 
    }
    (&shadowProcessTable[pid%MAXPROC])->endSleep = seconds;


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
    else if (priority <= priorityQueueHead->endSleep){
        struct Process * temp = priorityQueueHead;
        priorityQueueHead = p;
        p->next = temp;
    }

    else{
        struct Process * current = priorityQueueHead;
        while (current->next != NULL && current->next->endSleep <= priority){
            current = current->next;
            }
        p->next = current->next;
        current->next = p;
        
    }
}

void lockTerminal(int unit){
    // USLOSS_Console("LOCKING TERMINAL %d\n", unit);
    MboxSend(TERM_LOCK_MBOX_ARRAY[unit], NULL, NULL);
}

void unlockTerminal(int unit){
    // USLOSS_Console("UNLOCKING TERMINAL %d\n", unit);
    MboxRecv(TERM_LOCK_MBOX_ARRAY[unit], NULL, NULL);
}


void writeCharToTerminal(char character, int unit){
    int cr_val = 0x1; // this turns on the ’send char’ bit (USLOSS spec page 9)
    cr_val |= 0x2; // recv int enable
    cr_val |= 0x4; // xmit int enable
    cr_val |= (character << 8); // the character to send
    USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void*)(long)cr_val);

}
int ourTermWrite(void* args){
    USLOSS_Sysargs *sysargs = args; 
    char* buffer = sysargs->arg1;
    int bufferSize = sysargs->arg2;
    int unitID = sysargs->arg3;

    lockTerminal(unitID);
    // USLOSS_Console("BUFFER IN WRITE: %s", buffer);
    for(int i = 0; i < bufferSize; i++) {
        MboxRecv(TERM_WRITE_MBOX_ARRAY[unitID], NULL, 0); 
        writeCharToTerminal(buffer[i], unitID); 
    }

    // MboxCondSend(TERM_WRITE_MBOX_ARRAY[unitID], buffer, bufferSize);

    sysargs->arg4 = 0; 
    sysargs->arg2 = strlen(buffer);

    unlockTerminal(unitID);
    return 0;
}

int ourSleep(void *args) {

// dumpProcesses();
// dumpSP();
    int startTime = GLOBAL_CLOCK_TICKS * 100;

    //dumpProcesses();

    USLOSS_Sysargs *sysargs = args; 
    int seconds = sysargs->arg1;
    // USLOSS_Console("Inside Our Sleep %d\n", p->endSleep);


    int endTime = seconds*1000 + startTime;
    //USLOSS_Console("our sleep is called: startTime: %d endtime: %d seconds:%d pid:%d \n",startTime,endTime,seconds,getpid());
    struct Process * p = getProcess(getpid(), endTime);
    addToPriorityQueue(getpid());
    // dumpPriorityQueue();
    
    // USLOSS_Console("BLOCKING PROC\n");
    MboxSend(p->mboxID, NULL, NULL);

    // USLOSS_Console("sleep ended\n");


    return 0;
}


// int ourTermWrite(void* args){
//     USLOSS_Sysargs *sysargs = args; 

// }


int ourTermRead(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int bufferSize = sysargs->arg2;
    char *buffer = sysargs->arg1;
    READ_BUFFER_SIZE = bufferSize;
    int unitID = sysargs->arg3;
    // USLOSS_Console("UNIT: %d\n", unitID);
    // Check for invalid terminal 
    if(unitID < 0 || unitID > 3 || bufferSize <= 0 ) {
        sysargs->arg4 = -1;
        return -1;
    }
    // MboxSend(TERM_READ_MBOX_ARRAY[unitID], "hello there", strlen("hello there")+1);
    
    // memset(buffer, 'x', sizeof(bufferSize)-1);
    // buffer[bufferSize] = '\0';
    memset(buffer, '\0', 256);
    MboxRecv(TERM_READ_MBOX_ARRAY[unitID], buffer, bufferSize);
    // USLOSS_Console("MESSAGE: %s\n", buffer);
    // USLOSS_Console("STRING: %s\n", buffer);

    // if (buffer)
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