/**
 * Author: Samuel Richardson and Ali Sartaz Khan
 * Course: CSC 452
 * Phase: 4a
 * Description: This implements the device drivers for the clock and 
 * terminals. The kernel can call sleep for a number of seconds, and 
 * then our usermode code is called, blocking the process for the correct
 * number of seconds. The terminal functions are also used in a driver. 
 * We create mailboxes for each process to allow for blocking, as well 
 * as creating daemon functions for each driver (7 in total).
 * */


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

// intialize global variables

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

int DISK_SIZE_MBOX_ARRAY[2];

int DISK_LOCK_ARRAY[2];
int DISK_MBOX_ARRAY[2];

int ourSleep(void*);
int ourTermRead(void*);
int ourTermWrite(void*);
int ourDiskSize(void *args);
int ourDiskRead(void *args);
int ourDiskWrite(void *args);
void diskDaemon(unit);

int DISK_TRACK[2];

typedef struct {
  int currentTrack;
  USLOSS_DeviceRequest *request;  
  int status;
  int isComplete;
} Disk;

Disk disks[2];

// shadow process struct that keeps track of the sleep time, next proceses 
// in the queue, and the correponding mutex mailbox ID for blocking
struct Process {
    int PID;
    int mboxID;
    int endSleep;
    struct Process * next;
};


static struct Process shadowProcessTable[MAXPROC];
static struct Process * priorityQueueHead = NULL;


/*
    Description: initialize the syscall array valyes before the program 
    starts running.

    Param: None
    Return: None

*/
void phase4_init(void) {
    systemCallVec[SYS_SLEEP] = ourSleep;
    systemCallVec[SYS_TERMREAD] = ourTermRead;
    systemCallVec[SYS_TERMWRITE] = ourTermWrite;
    systemCallVec[SYS_DISKSIZE] = ourDiskSize;
    systemCallVec[SYS_DISKREAD] = ourDiskRead;
    systemCallVec[SYS_DISKWRITE] = ourDiskWrite;
}




/*
    Description: daemon process that handles the clock. This function has
    an infinite loop that calls waitDevice, which waits for the clock
    interrupt to occur, and then removes the process from the sleep queue

    Param: None
    Return: None

*/
void daemonProcessMain(){
    int status = -1;
    while (1) {
        // //USLOSS_Console("HERE\n");
        waitDevice(USLOSS_CLOCK_DEV, 0, &status);
        GLOBAL_CLOCK_TICKS ++;
        if (GLOBAL_CLOCK_TICKS % 10 == 0 && GLOBAL_CLOCK_TICKS < 4 * 10){
        }
        removeFromPriorityQueue();
    }
}


/*
    Description: since each terminal can only write one process
    at a time, we treat it as a mutex, this function gains the lock

    Param: int unit, the terminal to lock
    Return: None

*/
void lockTerminal(int unit){
    MboxSend(TERM_LOCK_MBOX_ARRAY[unit], NULL, NULL);
}



/*
    Description: since each terminal can only write one process
    at a time, we treat it as a mutex, this function relreases the lock

    Param: int unit, the terminal to unlock
    Return: None

*/
void unlockTerminal(int unit){
    MboxRecv(TERM_LOCK_MBOX_ARRAY[unit], NULL, NULL);
}



/*
    Description: The terminal daemon function that handles reading and writing
    from the terminal. THe specific terminal number is given by the unit, and then
    the function calls an infinte loop that constantly waits for the given terminal
    to send an interrupt. Each terminal has its own version of this function for each
    daemon functions func and args.

    Param: int unit: the unit specifying which terminal to use 
    Return: None

*/
void termMain2(int unit) {
    int status = -1;
    int sendCount = 0;

    // Statically allocate memory for the buffer
    char buffer[MAXLINE+1];
    int bufferIndex = 0;

    while (1) {
        waitDevice(USLOSS_TERM_DEV, unit, &status);

        int transmitBit = USLOSS_TERM_STAT_XMIT(status);
        if (transmitBit == USLOSS_DEV_READY) {
            MboxCondSend(TERM_WRITE_MBOX_ARRAY[unit], NULL, 0);
        } else if (transmitBit == USLOSS_DEV_ERROR) {
            //USLOSS_Console("ERROR: there's something wrong with writing the registers\n");
            USLOSS_Halt(1);
        }

        int readBit = USLOSS_TERM_STAT_RECV(status);
        if (readBit == USLOSS_DEV_BUSY) {
            char character = (char) USLOSS_TERM_STAT_CHAR(status);
            buffer[bufferIndex] = character;
            bufferIndex++;
            if (character == '\n' || bufferIndex == MAXLINE || bufferIndex == READ_BUFFER_SIZE) {
                buffer[bufferIndex] = '\0';
                MboxCondSend(TERM_READ_MBOX_ARRAY[unit], buffer, bufferIndex);
                // No need to free memory or allocate new memory here
                bufferIndex = 0;
            }
        } else if (readBit == USLOSS_DEV_ERROR) {
            //USLOSS_Console("ERROR: there's something wrong with reading the registers\n");
            USLOSS_Halt(1);
        }
    }
}



/*
    Description: checks to see if the enough time has elapased to remove
    from the head of the priortiy queue. If so keep removing until the
    new head of the queue has a time that has not hit yet, or is null

    Param: None
    Return: None

*/
void removeFromPriorityQueue(){
    int curTime = GLOBAL_CLOCK_TICKS * 100;
    while (priorityQueueHead != NULL && curTime >= priorityQueueHead->endSleep){
        struct Process* p = priorityQueueHead;
        priorityQueueHead = priorityQueueHead -> next;
        int mboxID = p->mboxID;
        MboxRecv(mboxID, NULL, NULL);
    }
}


/*
    Description: this function enables each terminal's interupts and creates
    the 7 daemon processes, and then creates the 4 mailboxes for reading and 
    writing to the terminal. This function is called before the dispatcher
    starts running processes.

    Param: None
    Return: None

*/
void phase4_start_service_processes(){
    // int ctrl = 0x7; 
    int ctrl = 0x6; 
        //USLOSS_Console("TESt in start processes\n");

    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 0, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 1, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 2, (void*)(long)ctrl) == USLOSS_DEV_OK);
    assert( USLOSS_DeviceOutput(USLOSS_TERM_DEV, 3, (void*)(long)ctrl) == USLOSS_DEV_OK);
    DAEMON_CLOCK_PID = fork1("DAEMON", daemonProcessMain, NULL, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL0_PID = fork1("TERM0", termMain2, 0, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL1_PID = fork1("TERM1", termMain2, 1, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL2_PID = fork1("TERM2", termMain2, 2, USLOSS_MIN_STACK, 1);
    DAEMON_TERMINAL3_PID = fork1("TERM3", termMain2, 3, USLOSS_MIN_STACK, 1);
    DAEMON_DISK0_PID = fork1("DISK", diskDaemon, 0, USLOSS_MIN_STACK, 1);
    DAEMON_DISK1_PID = fork1("DISK", diskDaemon, 1, USLOSS_MIN_STACK, 1);
    //DAEMON_DISK0_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);
   // DAEMON_DISK1_PID = fork1("DISK", NULL, NULL, USLOSS_MIN_STACK, 1);
    TERM_LOCK_MBOX_ARRAY[0] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[1] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[2] = MboxCreate(1, 0);
    TERM_LOCK_MBOX_ARRAY[3] = MboxCreate(1, 0);

    TERM_READ_MBOX_ARRAY[0] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[1] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[2] = MboxCreate(10, MAXLINE);
    TERM_READ_MBOX_ARRAY[3] = MboxCreate(10, MAXLINE);

    TERM_WRITE_MBOX_ARRAY[0] = MboxCreate(1, 1);
    TERM_WRITE_MBOX_ARRAY[1] = MboxCreate(1, 1);
    TERM_WRITE_MBOX_ARRAY[2] = MboxCreate(1, 1);
    TERM_WRITE_MBOX_ARRAY[3] = MboxCreate(1, 1);

    DISK_SIZE_MBOX_ARRAY[0] = MboxCreate(0,0);
    DISK_SIZE_MBOX_ARRAY[1] = MboxCreate(0,0);

    disks[0].currentTrack = 0;
    disks[0].request = NULL;

    disks[1].currentTrack = 0;
    disks[1].request = NULL;

    DISK_LOCK_ARRAY[0] = MboxCreate(1, 0);
    DISK_LOCK_ARRAY[1] = MboxCreate(1, 0);

    DISK_MBOX_ARRAY[0] = MboxCreate(1, 0);
    DISK_MBOX_ARRAY[1] = MboxCreate(1, 0);
}


/*
    Description: print a process for debugging

    Param: struct Process * cur, the pointer to the process to print
    Return: None

*/
void printProc(struct Process * cur){
    //USLOSS_Console("[PID: %d | MBOXID: %d | ENDSLEEP %d] -->",cur->PID, cur->mboxID, cur->endSleep);
}


/*
    Description: dumps the priorty queue for debugging

    Param: None
    Return: None

*/
int dumpPriorityQueue(){
    struct Process * cur = priorityQueueHead;
    //USLOSS_Console("\nDUMPING PQueue -------------\n\n");
    while (cur != NULL){
        printProc(cur);
        cur = cur->next;
    }
    //USLOSS_Console("\n\nEnding PQueue ------------\n\n");

}


/*
    Description: dumps the shadow process table for debugging

    Param: None
    Return: None

*/
void dumpSP(void){
    //USLOSS_Console("\nDUMPING SHADOW PROCTABLE ---------------------------\n\n");
    for (int i = 0; i < MAXPROC; i++){
        if ( shadowProcessTable[i].PID == 0){
            continue;
        }
        printProc(&shadowProcessTable[i]);
        //USLOSS_Console("\n");
    }
    //USLOSS_Console("\nEND OF DUMP PROC -----------------------\n\n");

}


/*
    Description: creates the shadow process, adds it to the table,
    and then returns the processes shadow pid.

    Param: int es, the end time of the process
    Param: int pid, the pid of the process
    Return: int pid

*/
int createShadowProcess(int es, int pid) {
    struct Process p = {
        .PID = pid, 
        .mboxID = MboxCreate(0,0),
        .endSleep = es,
        .next = NULL
    };
    shadowProcessTable[p.PID%MAXPROC] = p;
    return p.PID;
}


/*
    Description: our function to create the shadow process,
    overlap any old shadow processes, and return the pid

    Param: int seconds, the end time of the process
    Param: int pid, the pid of the process
    Return: struct Process

*/
struct Process* getProcess(int pid, int seconds) {
    if (pid != shadowProcessTable[pid%MAXPROC].PID){
        createShadowProcess(seconds,pid); 
    }
    (&shadowProcessTable[pid%MAXPROC])->endSleep = seconds;
    return &shadowProcessTable[pid%MAXPROC];
}


/*
    Description: creates the shadow process, adds it to the table,
    and then returns the processes shadow pid.

    Param: int es, the end time of the process
    Param: int pid, the pid of the process
    Return: struct Process

*/
struct Process* getProcess1(int pid) {    
  return &shadowProcessTable[pid%MAXPROC];
}


/*
    Description: adds the process that had just been told
    to sleep in the correct spot in the priority queue. If 
    it is the next process to wake up, make it the head, other
    wise, add it to the correct spot.

    Param: int pid, the pid of the process to add
    Return: None

*/
void addToPriorityQueue(int pid){
    struct Process * p = getProcess1(pid);
    int priority = p->endSleep;
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




/*
    Description: writes an individual character to the right register

    Param: int unit, the terminal to write to
    Param: char, the charcter to write

    Return: None

*/
void writeCharToTerminal(char character, int unit){
    int cr_val = 0x1; 
    cr_val |= 0x2; 
    cr_val |= 0x4; 
    cr_val |= (character << 8); 

    USLOSS_DeviceOutput(USLOSS_TERM_DEV, unit, (void*)(long)cr_val);
}

/*
    Description: our function that gets called when termWrite gets
    called. We created our own so that we can pass in the parameter 
    void * that represents the syscall struct that we need. This
    function adds the locking mchanism, and loops through the
    given string to write to our mailbox.

    Param: void*args, the struct holding the data from the syscall
    Return: None

*/
int ourTermWrite(void* args){
    USLOSS_Sysargs *sysargs = args; 
    char* buffer = sysargs->arg1;
    int bufferSize = sysargs->arg2;
    int unitID = sysargs->arg3;
    lockTerminal(unitID);

    int recvCount = 0;

    for(int i = 0; i < strlen(buffer); i++) {
        MboxRecv(TERM_WRITE_MBOX_ARRAY[unitID], NULL, 0);
        writeCharToTerminal(buffer[i], unitID);
    }

    unlockTerminal(unitID);

    sysargs->arg4 = 0; 
    sysargs->arg2 = strlen(buffer);
    return 0;
}



/*
    Description: our function that gets called when Sleep gets
    called. We created our own so that we can pass in the parameter 
    void * that represents the syscall struct that we need. This
    function adds the process that needs to sleep to the priorty
    queue and then blocks the process.
    
    Param: void*args, the struct holding the data from the syscall
    Return: None

*/
int ourSleep(void *args) {
    int startTime = GLOBAL_CLOCK_TICKS * 100;
    USLOSS_Sysargs *sysargs = args; 
    int seconds = sysargs->arg1;
    int endTime = seconds*1000 + startTime;
    struct Process * p = getProcess(getpid(), endTime);
    addToPriorityQueue(getpid());
    MboxSend(p->mboxID, NULL, NULL);
    return 0;
}



/*
    Description: our function that gets called when termRead gets
    called. We created our own so that we can pass in the parameter 
    void * that represents the syscall struct that we need. This
    function adds the process that needs to sleep to the priorty
    queue and then blocks the process.

    Param: void*args, the struct holding the data from the syscall
    Return: None

*/
int ourTermRead(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int bufferSize = sysargs->arg2;
    char *buffer = sysargs->arg1;
    READ_BUFFER_SIZE = bufferSize;
    int unitID = sysargs->arg3;
    if(unitID < 0 || unitID > 3 || bufferSize <= 0 ) {
        sysargs->arg4 = -1;
        return -1;
    }
    memset(buffer, '\0', bufferSize);
   int size =  MboxRecv(TERM_READ_MBOX_ARRAY[unitID], buffer, bufferSize);

    // //USLOSS_Console("BUFFER: %s\n", buffer);
    sysargs->arg2 = size;
    sysargs->arg4 = 0;   
    return 0;
}


void diskDaemon(int unit){
    //USLOSS_Console("TEST in s\n");
    assert(unit == 0 || unit == 1);
    int status;
    while (1){
        waitDevice(USLOSS_DISK_DEV, unit, &status);
        //USLOSS_Console("After device input()\n");
        // assert(status == USLOSS_DEV_ERROR);
        if (status == USLOSS_DEV_READY) {
            USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);

            //USLOSS_Console("operation is done\n");
            disks[unit].status = status;
            disks[unit].isComplete = 1;
            MboxCondSend(DISK_MBOX_ARRAY[unit], NULL, 0);
        } else if (status == USLOSS_DEV_ERROR) {
            // USLOSS_Console("ERROR in daemon\n");
            disks[unit].status = USLOSS_DEV_ERROR;
            MboxCondSend(DISK_MBOX_ARRAY[unit], NULL, 0);

        
        } 
    }
}


// Seek disk head to track
int Seek(int unit, int track) {
    if (DISK_TRACK[unit] == track){
        return;
    }

    USLOSS_DeviceRequest req;
    req.opr = USLOSS_DISK_SEEK;
    req.reg1 = track; 
    req.reg2 = NULL;

    USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, &req);

    MboxRecv(DISK_MBOX_ARRAY[unit], NULL, 0);

    if (disks[unit].status == USLOSS_DEV_ERROR){
        return USLOSS_DEV_ERROR;
    }

    return 111;




    
}

void lockDisk(int unit){
    MboxSend(DISK_LOCK_ARRAY[unit], NULL, NULL);
}

void unlockDisk(int unit){
    MboxRecv(DISK_LOCK_ARRAY[unit], NULL, NULL);
}

int ourDiskRead(void *args){
    USLOSS_Sysargs *sysargs = args; 
    char * diskBuffer = (char*) sysargs->arg1;
    int numOfSectors = ( (long) sysargs->arg2);
    int startingTrackNumber = ( (long) sysargs->arg3);
    int firstSector = ( (long) sysargs->arg4);
    int unit =  ( (long) sysargs->arg5);
    //USLOSS_Console("TEST in read\n");

    if ((unit != 1 && unit != 0) || firstSector < 0 || firstSector > 15){
        sysargs->arg4 = -1;
        return -1;
    }

    assert(unit == 0 || unit ==1);
    lockDisk(unit);

    int curSector = firstSector;
    int curTrack = startingTrackNumber;

    int status;
    
    for (int i = 0; i < numOfSectors; i++){
        int resultSeek = Seek(unit, curTrack);
        if (resultSeek == USLOSS_DEV_ERROR){
            sysargs->arg1 = USLOSS_DEV_ERROR;
            sysargs->arg4 = 0;
            return;

        }


        struct USLOSS_DeviceRequest *request = &disks[unit].request;

        request->opr = USLOSS_DISK_READ;
        request->reg1 =  (void*)(long) curSector;
        request->reg2 = diskBuffer;

        USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);

        MboxRecv(DISK_MBOX_ARRAY[unit], NULL, 0);

        //USLOSS_Console("back to read\n");
        assert(disks[unit].isComplete == 1);
        disks[unit].isComplete = 0;

        status = disks[unit].status;
        
        // ASSUMUNG TRACK INDEXED AT 1
        if (curSector == 15){
            curSector = -1;
            curTrack ++;
        }
        curSector++;

        diskBuffer = diskBuffer + 512;
    }

    if (status == USLOSS_DEV_READY){sysargs->arg1 = 0;}
    else {sysargs->arg1 = status;}
    
    sysargs->arg4 = 0;
    unlockDisk(unit);


}

int ourDiskWrite(void *args){
    // USLOSS_Console("jere\n");
    USLOSS_Sysargs *sysargs = args; 
    char * diskBuffer = (char*) sysargs->arg1;
    int numOfSectors = ( (long) sysargs->arg2);
    int startingTrackNumber = ( (long) sysargs->arg3);
    int firstSector = ( (long) sysargs->arg4);
    int unit =  ( (long) sysargs->arg5);

    if ((unit != 1 && unit != 0) || firstSector < 0 || firstSector > 15){
        sysargs->arg4 = -1;
        sysargs->arg1 = USLOSS_DEV_ERROR;
        return -1;
    }
    assert(unit == 0 || unit ==1);

    lockDisk(unit);

    
    int curSector = firstSector;
    int curTrack = startingTrackNumber;

    int status;
    
    for (int i = 0; i < numOfSectors; i++){
        int resultSeek = Seek(unit, curTrack);
        if (resultSeek == USLOSS_DEV_ERROR){
            sysargs->arg1 = USLOSS_DEV_ERROR;
            sysargs->arg4 = 0;
            return;

        }

        struct USLOSS_DeviceRequest *request = &disks[unit].request;

        request->opr = USLOSS_DISK_WRITE;
        request->reg1 =  (void*)(long) curSector;
        request->reg2 = diskBuffer;

        USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, request);

        MboxRecv(DISK_MBOX_ARRAY[unit], NULL, 0);

        //USLOSS_Console("back to write\n");

        disks[unit].isComplete = 0;

        status = disks[unit].status;
        
        // ASSUMUNG TRACK INDEXED AT 1
        if (curSector == 15){
            curSector = -1;
            curTrack ++;
        }
        curSector++;

        diskBuffer = diskBuffer + 512;
    }

    if (status == USLOSS_DEV_READY){sysargs->arg1 = 0;}
    else {sysargs->arg1 = status;}
    
    sysargs->arg4 = 0;
    unlockDisk(unit);


}


int ourDiskSize(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int unit = sysargs->arg1;
    //USLOSS_Console("In disksize\n");
    //Seek(unit, 16);

    struct USLOSS_DeviceRequest request;
    request.opr = USLOSS_DISK_TRACKS;

    int numOfTracks;
    request.reg1 = &numOfTracks;


    //void * useReques = (void *) request;
    USLOSS_DeviceOutput(USLOSS_DISK_DEV,unit, &request);
    ////USLOSS_Console("about to block\n");
    MboxRecv(DISK_MBOX_ARRAY[unit],NULL,NULL);


    int status;
    // USLOSS_DeviceInput(USLOSS_DISK_DEV, unit, &status);

    // if (status == USLOSS_DEV_READY) {
    //     //USLOSS_Console("ready\n");
    // } else {
    //     //USLOSS_Console("not ready\n");
    // }
    
    ////USLOSS_Console("size: %d",numOfTracks);

    sysargs->arg1 = 512;
    sysargs->arg2 = 16;
    sysargs->arg3 = numOfTracks;
    sysargs->arg4 = 0;



    
    return 0;
}



//--------------------------------------------------------------------------
//functions that are not used but are required in the .h file


/*
Description: function for kernSleep, but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernSleep(int seconds) {
    return 0;
}

/*
Description: function for kernDiskRead, but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernDiskRead(void *diskBuffer, int unit, int track, int first,int sectors, int *status) {
    return 0; 
}

/*
Description: function for kernDiskWrite, but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernDiskWrite(void *diskBuffer, int unit, int track, int first, int sectors, int *status) {
    return 0;
}

/*
Description: function for kernDiskSize, but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernDiskSize(int unit, int *sector, int *track, int *disk) {
    return 0; 
}

/*
Description: function for kernTermRead, but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernTermRead(char *buffer, int bufferSize, int unitID, int *numCharsRead) {
    return 0; 
}

/*
Description: function for kernTermWrite , but is not used. Just included so the code
compiless because this function is in the phase4a.h file.
*/
int kernTermWrite(char *buffer, int bufferSize, int unitID, int *numCharsRead) {
    return 0; 
}