/**
 * Author: Samuel Richardson and Ali Sartaz Khan
 * Course: CSC 452
 * Phase: 1a
 * Description: This program uses USLOSS and Docker to simulate an operating
 * system. We create a Process struct with fields relating to individual 
 * process information such as siblings, parents, stack, etc, and then 
 * initialize an array with MAXPROC slots of type Process. We have functions
 * join, fork1, and quit that handle the creation and removal of processes,
 * and we use TEMP_switchto to context switch to the next process. We also 
 * have implemention for freeing the stack, printing using dumpProcesses(),
 * and helper functions to assist the main required functions.
 * */



#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
#include <usloss.h> 
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// global variables to keep track  which process is running and what ID the 
// next process may have
int runningProcessID = -1;
int pidCounter = 1;
int clockTicks = 0;
int ZAP_STATUS = 150000;
int JOIN_BLOCK_STATUS = 20000;

/*
    Struct that represents an individual process. Each field represents an
    attribute for a individual process. When a process is created, it's
    state is switched to Ready, and all of the fields passed in by fork
    are initialized, as well as isInitialized being set to 1. When a process
    is called using Tempswitchto() and its PID, its state is switched to 
    Running, and its startFunc is called. When a process has run to it's 
    entirety, its state is changed to Terminated, and after it has been
    joined, it is removed from its parent tree and its stack is freed.

*/
struct Process {
    int isInitialized;
    char name[MAXNAME];
    int PID;
    int priority;
    bool isAlive;
    USLOSS_Context context;

    struct Process* nextSibling;
    struct Process* prevSibling;
    struct Process* parent;
    struct Process* firstChild;

    struct Process* queueHead;
    struct Process* nextQueueNode;
    struct Process* prevQueueNode;

    struct Process* zapHead;
    struct Process* nextZapNode;
    struct Process* prevZapNode;
    struct Process* iZap;

    char* state;
    void * stack;
    int status;
    bool isBlocked;
    int isZapped;
    bool isZombie;
    int (*startFunc)(char*);
    char* arg;
    int time;
    int startTime;
    int endTime;
    int totalTime;


};


struct Process processTable[MAXPROC];
struct Process* runQueues[8];


// declaration for functions not already mane in phase.h
int findOpenProcessTableSlot();
void addChildToParent(struct Process* ,struct Process*);
void printChildren(struct Process*);
void dumpProcesses();
void dumpQueue();
void trampoline();
struct Process* getProcess(int);
int init_main(char*);
void removeNode(struct Process*);
void addToQueue(struct Process*);

void printChildren(struct Process* parent){
    struct Process* curChild = parent->firstChild;
    USLOSS_Console("child list for %d: ", parent->PID);
    
    while (curChild != NULL){
        USLOSS_Console("%s -> ",curChild->name);
        curChild = curChild->nextSibling;
    }
    USLOSS_Console("NULL\n");

}

void dumpProcessesUS(void){
    for (int i = 0; i < MAXPROC; i++){
        // if (processTable[i].PID == 0){
        //     USLOSS_Console("%d: PID: -- | NAME: -- | PRIORITY: -- | \n", i);
        //     continue;
        // }
        USLOSS_Console("%d: PID: %d | NAME: %s | PRIORITY: %d | STATE: %s | ",i,processTable[i].PID,processTable[i].name,processTable[i].priority, processTable[i].state);
        printChildren(&processTable[i]);
    }
        USLOSS_Console("Running Process ID: %d\n",runningProcessID);
    USLOSS_Console("-------------------------------------\n \n");

}


void runProcess(int pid){
    //USLOSS_Console("name of process running : %s\n",getProcess(pid)->name);


    if (getpid() == -1){
            //USLOSS_Console("inside if sttement \n");
        struct Process* newProc = getProcess(pid);
                //USLOSS_Console("1\n");

        newProc -> state = "Running";
        runningProcessID = pid;
        //USLOSS_Console("name of process: %s\n",&(processTable[pid % MAXPROC].name));

        newProc->startTime = currentTime();
        USLOSS_ContextSwitch(NULL, &(processTable[pid % MAXPROC].context));
        //USLOSS_Console("1\n");
        return;
    }


    struct Process* oldProc = getProcess(runningProcessID); 
    // USLOSS_Console("%s STATE IN RUN PROC(): %s\n", oldProc->name, oldProc-> state);


    // if (strcmp(oldProc->state,"Terminated") != 0 && strcmp(oldProc->state,"Blocked(waiting for child to quit)") != 0 && strcmp(oldProc->state,"Blocked(waiting for zap target to quit)") != 0 && strcmp(oldProc->state,"Blocked") != 0){
    //     // USLOSS_Console("Process hasnot been terminated\n");
    //     oldProc -> state = "Runnable";
    //     //setZappedVarVar(oldProc->PID);
    // }
    if (strcmp(oldProc->state,"Running") == 0 ){
        // USLOSS_Console("Process hasnot been terminated\n");
        oldProc -> state = "Runnable";
        //setZappedVarVar(oldProc->PID);
    }
    
    struct Process* newProc = getProcess(pid); 

    newProc -> state = "Running";   

    int prevID = runningProcessID; 
    runningProcessID = pid;
    newProc->startTime = currentTime();
    //USLOSS_Console("%s is running.\n", newProc -> name);



    // USLOSS_Console("BEFORE REMOVE NODE\n");
    // dumpQueue();
    // removeNode(oldProc);


    // int priority = oldProc->priority;
    // int slot = oldProc->PID % MAXPROC;

    // if (runQueues[priority] == NULL){
    //     struct Process* new = &processTable[slot];
    //     runQueues[priority] = new;
    // } else{
    //     struct Process* curProc = runQueues[priority];
    //     while (curProc->nextQueueNode!= NULL){curProc = curProc->nextQueueNode;}
    //     curProc -> nextQueueNode = &processTable[slot];
    //     (&processTable[slot])->prevQueueNode = curProc;
    //     curProc->nextQueueNode = NULL;
    // }

    // USLOSS_Console("After REMOVE NODE\n");
    // dumpQueue();



    USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, 
    &(processTable[pid % MAXPROC].context));

}

void printProcess(int pid){
    USLOSS_Console("Process Name: %s\n",&processTable[pid % MAXPROC].name);
}

void dispatcher(){
    // DISABLE INTERRUPTS
    // ENABLE INTERRUPTS IN TRAMPOLINE
    // CALL THIS IN QUIT(), BLOCKME(), UNBLOCKPROC(), ZAP(), JOIN()
    // dumpProcesses();
    int ps = USLOSS_PsrGet();
    USLOSS_PsrSet(ps & ~USLOSS_PSR_CURRENT_INT);

    struct Process * prevProc = getProcess(runningProcessID);
    int timeRunning = readtime();
    prevProc->totalTime += timeRunning;
    // USLOSS_Console("Process \"%s\" ran for %d ms\n",prevProc->name,timeRunning);
    // USLOSS_Console("Process total: %dms\n",prevProc->totalTime);
    int found = 0;
    int retPID;
    //dumpQueue();
    //dumpProcesses();


    for (int i = 1; i < 8; i++){
        struct Process * curProcess = runQueues[i];
        
        while (curProcess != NULL){
            //USLOSS_Console("Process in question: %s\n",curProcess->name);
            
            if (strcmp(curProcess->state,"Runnable") == 0){
                // USLOSS_Console("Process: \"%s\" is Runnable.\n", curProcess->name);
                // USLOSS_Console("TARGET_ZAP_PID: %d\n", TARGET_ZAP_PID);
                runProcess(curProcess->PID);
                return;
            }
            curProcess = curProcess->nextQueueNode;
        }
        //USLOSS_Console("priority %d has no runnable process\n",i);
    }

    USLOSS_PsrSet(ps);
}


void zap(int pid){
    int old_psr = USLOSS_PsrGet();
     USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);
    struct Process* target = getProcess(pid); // Process getting zapped


    if(pid == 1){
        USLOSS_Console("ERROR: Attempt to zap() init.\n");
        USLOSS_Halt(1);
    }
    if(pid <= 0){
        USLOSS_Console("ERROR: Attempt to zap() a PID which is <=0.  other_pid = %d\n",pid);
        USLOSS_Halt(1);
    }
    if(pid == runningProcessID){
        USLOSS_Console("ERROR: Attempt to zap() itself.\n");
        USLOSS_Halt(1);
    }
    if (target->isInitialized == 0 || pid != target->PID){
        USLOSS_Console("ERROR: Attempt to zap() a non-existent process.\n");
        USLOSS_Halt(1);
    }
    if (strcmp(target->state,"Terminated") == 0){
        USLOSS_Console("ERROR: Attempt to zap() a process that is already in the process of dying.\n");
        USLOSS_Halt(1);
    }

    // adding zappers to target's zap list
     if (target->zapHead != NULL){
         struct Process* oldHead = target -> zapHead;
         target -> zapHead = getProcess(getpid());
         target -> zapHead -> nextZapNode = oldHead;
     } else {
         target -> zapHead = getProcess(getpid());
     }

    // setting which process that current process zaps
    struct Process* runningProc = getProcess(getpid());
    runningProc -> iZap = target;

    
    blockMe(ZAP_STATUS);
    USLOSS_PsrSet(old_psr);


}


int isZapped(){
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);
    
    USLOSS_PsrSet(old_psr);
    if (getProcess(getpid()) -> zapHead == NULL){return 0;}
    return 1;
}


void blockMe(int block_status){
    if (block_status == ZAP_STATUS){
        (processTable[runningProcessID % MAXPROC]).state = "Blocked(waiting for zap target to quit)";
        removeNode(getProcess(getpid()));
    }
    else if (block_status == JOIN_BLOCK_STATUS){
        (processTable[runningProcessID % MAXPROC]).state = "Blocked(waiting for child to quit)";
                removeNode(getProcess(getpid()));

    }
    else if (block_status <= 10){

        USLOSS_Console("ERROR: BlockME has incorrect block status!");
        USLOSS_Halt(1);
}
    else {
        (processTable[runningProcessID % MAXPROC]).state = "Blocked";
        (processTable[runningProcessID % MAXPROC]).status = block_status;
                removeNode(getProcess(getpid()));



    }
    // USLOSS_Console("Blocked PID: %d in blockMe()\n", getpid());
    // dumpProcesses();
    dispatcher();
}

void addToQueue1(int pid){
    int priority = getProcess(pid)->priority;
    int slot = pid % MAXPROC;

// adding to the Queue
    if (runQueues[priority] == NULL){
        struct Process* new = &processTable[slot];
        runQueues[priority] = new;
    } else{
        struct Process* curProc = runQueues[priority];
        while (curProc->nextQueueNode!= NULL){curProc = curProc->nextQueueNode;}
        curProc -> nextQueueNode = &processTable[slot];
        (&processTable[slot])->prevQueueNode = curProc;
    }
}

int unblockProc(int pid){
    // USLOSS_Console("Unblocked PID: %d\n", pid);
    getProcess(pid) -> state =  "Runnable";

    addToQueue(getProcess(pid));
    
    dispatcher();
    return 0;
}


// Get start time of current time slice
int readCurStartTime() {
  return getProcess(getpid())->startTime; 
}

/* this is the interrupt handler for the CLOCK device */
static void clockHandler(int dev,void *arg)
{
        // if (debug) {
        //     USLOSS_Console("clockHandler(): PSR = %d\n", USLOSS_PsrGet());
        //     USLOSS_Console("clockHandler(): currentTime = %d\n", currentTime());
        // }
        /* make sure to call this first, before timeSlice(), since we want to do
        * the Phase 2 related work even if process(es) are chewing up lots of
        * CPU.
        */
        phase2_clockHandler();
        // call the dispatcher if the time slice has expired
        timeSlice();
        /* when we return from the handler, USLOSS automatically re-enables
        * interrupts and disables kernel mode (unless we were previously in
        * kernel code).  Or I think so.  I haven’t double-checked yet.  TODO
        */
}
int currentTime()
{
/* I don’t care about interrupts for this process.  If it gets interrupted,
    * then it will either give the correct value for *before* the interrupt,
    * or after.  But if the caller hasn’t disabled interrupts, then either one
    * is valid
    */
    int retval;
    int usloss_rc = USLOSS_DeviceInput(USLOSS_CLOCK_DEV, 0, &retval);
    assert(usloss_rc == USLOSS_DEV_OK);
    return retval;
}

// Get total time for current process
int readtime() {
  return currentTime() - readCurStartTime(); 
}

// Check if time slice expired
void timeSlice() {
    // USLOSS_Console("INSIDE THE TIMESLICE()\n");



  if (readtime() >= 80 && getProcess(getpid())->nextQueueNode != NULL) {
    // USLOSS_Console("INSIDE THE TIMESLICE() if statement\n");
    // dumpQueue();
    struct Process* p = getProcess(getpid());
    struct Process* head = runQueues[p->priority];

    // USLOSS_Console("DOES HAVE A SECOND NODE!\n");
    struct Process* curProc = head;
    
    while (curProc->nextQueueNode != NULL){curProc = curProc->nextQueueNode;}
    // USLOSS_Console("END\n");

    curProc -> nextQueueNode = head;
    runQueues[p->priority] = head -> nextQueueNode;
    head->nextQueueNode = NULL;

    //USLOSS_Console("Outside the if\n");
    // dumpQueue();
    dispatcher();
  }

}

/*
    Description: Creates a process and sets its fields to the
    arguments passed in. Allocates the stack memory and adds
    it to the processTable array.

    Arg: char *name: name of the process
    Arg: int (*startFunc)(char*): function to be run
    Arg: void *arg: argument to pass into startFunc
    Arg: int stackSize: size of stack to allocate
    Arg: int priority: priority of the process

    return int: the PID of the process
*/
int createProcess(char *name, int (*startFunc)(char*), void *arg,
int stackSize, int priority) {

    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    if (slot == -1) {  
        return -1; 
    }

    struct Process p = {.PID = pidCounter-1, .priority = priority,
    .stack = malloc(stackSize), .startFunc = startFunc, .arg = arg, 
    .isInitialized = 1, .state = "Runnable", .nextSibling = NULL, 
    .prevSibling = NULL, .totalTime = 0 };

    processTable[slot] = p;
    strcpy(processTable[slot].name, name);
    processTable[slot].name[MAXNAME - 1] = '\0';

    addToQueue(&processTable[slot]);

    USLOSS_ContextInit(&(processTable[slot].context),
    processTable[slot].stack, stackSize, NULL, trampoline);
    USLOSS_PsrSet(old_psr);

    return pidCounter-1;
}






/*
    Description: Bootstrap function created. Detects deadlocks in
    the processes, but since there are no blocking in phase1a,
    I don't believe it has much use in this program yet.

    Arg: char *arg: main function to be ran

    return int: error code should be zero if it reaches return
*/
int sentinel(char *arg){
    while (1) {
        if (phase2_check_io() == 0){
            USLOSS_Console("DEADLOCK DETECTED!  All of the processes have blocked, but I/O is not ongoing.\n");
            USLOSS_Halt(1);
        }
        USLOSS_WaitInt();
    }

    return 0;
}


/*
    Description: Main process of the testcase_main process. Enables 
    interupts, calls the testcase_main() function from the testcase,
    and the disables interrupts and ends the program. 

    Arg: none

    return: none
*/
void testcase_main_local(){

    // saves interrupts state and enables interrupts
    int psr = USLOSS_PsrGet();
    USLOSS_PsrSet(psr | USLOSS_PSR_CURRENT_INT);

    int retVal = testcase_main();
    //USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
    if(retVal != 0){
        USLOSS_Console("ERROR MESSAGE");
    }

    // sets interrupts state to old state, and halts program, signifying 
    // the completion of the test case.
    USLOSS_PsrSet(psr);
    USLOSS_Halt(retVal);
}

// void clockHandler(int type, void *arg) {

//     // Print a message on each clock tick
//     //   USLOSS_Console("Clock tick %d\n", clockTicks);

//     // Increment counter
//     clockTicks++;

//     // Call time slice check
//     timeSlice(); 

// }

/*
    Description: Boostrap process that simplt creates the init process
    using our createProcess function.

    Arg: none

    return: none
*/
void phase1_init(void) {
    for (int i = 0; i < 8; i++) {
        runQueues[i] = NULL;
    }
    
    createProcess("init", init_main, NULL, USLOSS_MIN_STACK, 6);
    USLOSS_IntVec[USLOSS_CLOCK_INT] = &clockHandler;
}

/**
     * Description: trampoline function that runs the childs main function.
     * 
     * Arg: none
     * 
     * return: none
    */
void trampoline(void){
    int ps = USLOSS_PsrGet();
    USLOSS_PsrSet(ps | USLOSS_PSR_CURRENT_INT);

    struct Process* curProcess = getProcess(getpid());
    int status = curProcess->startFunc(curProcess->arg);
    // USLOSS_Console("Trampoline Status: %d\n", status);
    if (status != -1) {
        quit(status); 
    }
    USLOSS_PsrSet(ps);
}


/** 
     * Description: getter function that return process
     *  
     * Arg: pid of process to retrieve
     * 
     * return: Process that you are looking for
    */
struct Process* getProcess(int pid) {    
    return &processTable[pid%MAXPROC];
}



/*
    Description: Creates the process using createProcess(), and then adds
    the process to the parent-sibling tree.

    Arg: char *name: name of the process
    Arg: int (*startFunc)(char*): function to be run
    Arg: void *arg: argument to pass into startFunc
    Arg: int stackSize: size of stack to allocate
    Arg: int priority: priority of the process

    return int: the PID of the process
*/
int fork1(char *name, int(func)(char *), char *arg, int stacksize, 
int priority) {

    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if (mode != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Someone attempted to call fork1 while in user mode!\n");
        USLOSS_Halt(1);
    }

    if (stacksize < USLOSS_MIN_STACK){
        return -2;
    }
    int pid = createProcess(name, func, arg, stacksize, priority);
    if (pid == -1 || (priority > 7 || priority < 1) || func == NULL || 
    name == NULL || strlen(name) > MAXNAME){
        return -1;
    }

    int newChildSlot = pid % MAXPROC;
    int parentID = getpid();

    struct Process* newChild = &processTable[newChildSlot]; 
    newChild->parent = &processTable[parentID%MAXPROC]; 
    struct Process* parent = &processTable[parentID%MAXPROC]; 
    // USLOSS_ContextSwitch(&processTable[parentID%MAXPROC].context, NULL);

    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
    } else {
        parent -> firstChild -> prevSibling = newChild;
        newChild->nextSibling = parent->firstChild;
        parent->firstChild = newChild;
    }

    // printProcess(parent->PID);
    parent -> state = "Runnable";
    //setZappedVarVar(parent->PID);

    

    dispatcher();

    return pid; 
}

/*
    Description: iterates through the current process until a Terminated
    process is found. Then removes it from the parent-sibling tree, removes
    it from the processTable, and frees the processes stack memory.

    Arg: int *status: address to be changed to the process end status

    return int: the PID of the process, or -2 if no terminated children
*/
int join(int *status) {
    
    struct Process* parent = getProcess(runningProcessID);
    struct Process* curChild = parent -> firstChild;

    // USLOSS_Console("firstCHILD = %s\n",curChild->name);
    

    if (curChild == NULL){
        return -2;
    } 
    while (curChild != NULL){
        if (strcmp(curChild -> state,"Terminated") == 0){
            if (curChild->prevSibling != NULL){
            curChild->prevSibling->nextSibling = curChild->nextSibling;
            }
            else{
                parent -> firstChild = curChild -> nextSibling;
            }
            if (curChild->nextSibling != NULL){
            curChild->nextSibling->prevSibling = curChild->prevSibling;
            }
        *status = curChild -> status;
        //USLOSS_Console("STatus = %d\n",*status);

        curChild -> state = "REMOVED";
        int prevPID = curChild->PID;

        if (curChild->stack != NULL){
            free(curChild->stack);
        }

        struct Process blank = {};
        processTable[curChild->PID % MAXPROC] = blank;
        return prevPID;

        } 
        curChild = curChild->nextSibling;
        
    } 

    //USLOSS_Console("BLOCKED IN THE PARENT JOIN\n");
    // dumpProcesses();

    blockMe(JOIN_BLOCK_STATUS);
    //USLOSS_Console("UNBLOCKED IN THE PARENT JOIN\n");
    // dumpProcesses();
    //USLOSS_Console("RETURNING -2 HERE\n");
    int temp;
    int retVal = join(&temp);
    *status = temp;
    return retVal; 
}



/*
    Description: Called at when a process has been fully run, and not
    when it has only been context switched away from. Turns the current
    process

    Arg: int status: passed in from join
    Arg: int switchToPid: id of process to be manually switched to

    return int: none
*/
void quit(int status) {

    //USLOSS_Console("quit called\n");

    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if (mode != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Someone attempted to call quit while in user mode!\n");
        USLOSS_Halt(1);

    }
    int curPid = runningProcessID;

    // dumpProcessesUS();
    struct Process* curProcess = getProcess(curPid);
    if (curProcess -> firstChild != NULL){
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", curPid);
        USLOSS_Halt(1);
    }
    
   
    curProcess->status = status;
    curProcess->state = "Terminated";

    //USLOSS_Console("node about to be renmvoed\n");
    removeNode(curProcess);
    //USLOSS_Console("node removed\n");

    int parentID = curProcess->parent->PID;

    // Setting list of zappers from blocked to runnable
    struct Process* curProc = curProcess -> zapHead;
    while (curProc != NULL){
        if (strcmp(curProc->state, "Terminated") != 0){
            curProc -> state = "Runnable";
            addToQueue(curProc);
        }
        curProc = curProc -> nextZapNode;
        
    }

    //USLOSS_Console("parentID = %d\n",parentID);
    // USLOSS_Console("PROC: %s ENDED IN QUIT()\n", getProcess(getpid()) -> name);

    unblockProc(parentID);
    // dispatcher();
    // when i quit, i will wake up parent if they are bklocked in join, not blocked in zapped.
    // wake up: remove from blocked to runnable
}



// Function to add a node to the end of the doubly linked list
void addToQueue(struct Process* nodeToAppend) {
    // USLOSS_Console("\n\nInside ADD Node\n");
    //     printProcess(nodeToAppend->PID);
    //         dumpQueue();



    struct Process* head = runQueues[nodeToAppend->priority];
    if (head == NULL) {
        runQueues[nodeToAppend->priority] = nodeToAppend;
    } else {
        struct Process* current = head;
        while (current->nextQueueNode != NULL) {
            current = current->nextQueueNode;
        }
        current->nextQueueNode = nodeToAppend;
        nodeToAppend->prevQueueNode = current;
        nodeToAppend -> nextQueueNode = NULL;
    }
    // dumpQueue();
        // USLOSS_Console("\n END OF ADD Node\n");

}

// Function to remove a specific node from the doubly linked list
void removeNode(struct Process* nodeToRemove) {
    if (runQueues[nodeToRemove->priority] == NULL || nodeToRemove == NULL) {
        return; 
    }
    // USLOSS_Console("\n\nInside remove Node\n");
    // printProcess(nodeToRemove->PID);
    // dumpQueue();
    if (nodeToRemove->prevQueueNode != NULL) {
        nodeToRemove->prevQueueNode->nextQueueNode = nodeToRemove->nextQueueNode;
    }
    if (nodeToRemove->nextQueueNode != NULL) {
        nodeToRemove->nextQueueNode->prevQueueNode = nodeToRemove->prevQueueNode;
    }
    if (runQueues[nodeToRemove->priority] == nodeToRemove) {
        runQueues[nodeToRemove->priority] = nodeToRemove->nextQueueNode;
    }
    // dumpQueue();

    // USLOSS_Console("END OF remove Node\n\n");

}


/*
    Description: returns the PID of current process from the global variable

    Arg: none

    return int: ID of the current process
*/
int getpid(void) {
    return runningProcessID; 
}


/*
    Description: Boostrap function that calls the phase service starts
    process, sets the init function to running, adds it to the table,
    and the context switches to init.

    Arg: none

    return none
*/
void startProcesses(void) {
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes();


    //technically suposed to call dispatcher
    dispatcher();

    //runProcess(1);

}



/*
    Description: iterates through the process table to find an open slot
    for the created process using pidCounter % MAXPROC.

    Arg: none

    return int: the found slot for the process or -1 if table is full
*/
int findOpenProcessTableSlot(){
    int i = 0;
    while (i<MAXPROC){
        int slot = pidCounter % MAXPROC;
        pidCounter++;
        if (processTable[slot].isInitialized == 0){
            return slot;}
        i++;
    }

    return -1;
}



/*
    Description: prints information about the processTable in the same formatting
    given by the test cases.

    Arg: none

    return: none
*/
void dumpProcesses(void){
    USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");
    for (int i = 0; i < MAXPROC; i++){
         if (processTable[i].isInitialized != 1){
             continue;
         }
        else if(processTable[i].PID == 1){
            USLOSS_Console(" %3d     0  %-13s     %d         %s",processTable[i].PID,processTable[i].name,processTable[i].priority,processTable[i].state);
        }
        else {
            USLOSS_Console(" %3d  %4d  %-13s     %d         %s",processTable[i].PID,processTable[i].parent->PID,processTable[i].name,processTable[i].priority,processTable[i].state);
            
        }
        if(strcmp(processTable[i].state,"Terminated")==0){
                USLOSS_Console("(%d)",processTable[i].status);
            }
        if(strcmp(processTable[i].state,"Blocked")==0){
                USLOSS_Console("(%d)",processTable[i].status);
            }
        USLOSS_Console("\n");
    }
    
}

void dumpQueue(){
    USLOSS_Console("-------------- Dump Queue ----------------\n");
    for (int i = 0; i < 8; i++){
        USLOSS_Console("%d: [", i);
        if (runQueues[i] == NULL){
            USLOSS_Console("]\n");
            continue;
        }

        struct Process* curProc = runQueues[i];
        USLOSS_Console(" %s (H) -> ", curProc->name);
        curProc = curProc -> nextQueueNode;
        while (curProc != NULL){
            USLOSS_Console("%s -> ", curProc->name);
            curProc = curProc -> nextQueueNode;
        }
        USLOSS_Console("NULL ]\n", curProc->name);
    }
    USLOSS_Console("-------------- END ------------------------\n\n\n");

}

/*
    Description: Called in boot strap process. Creates the sentinel
    and testcase_main processes, and context switched to testcase_main

    Arg: char *arg: main function to be ran

    return int: error code, but should be halted before it returns anyway
*/
int init_main(char *arg){
    
    //USLOSS_Console("init main started\n");
    
    
    fork1("sentinel", sentinel, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
    //dumpQueue();
    //fork1("testcase_main2", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
    //dumpProcesses();
    //dumpQueue();
    //USLOSS_Console("end of initmain");





    int retVal = 0;
    int errorCode = 0;
    while (retVal != -2){
        //USLOSS_Console("errorCode: %d\n",errorCode);
        retVal = join(&errorCode);
        getProcess(1) -> state = "Runnable";
        dispatcher();
    }

    // USLOSS_ContextSwitch(NULL, &(processTable[3].context));
    // TEMP_switchTo(3);
    USLOSS_Halt(0);
    return 0;
}

