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

// global variables to keep track  which process is running and what ID the 
// next process may have
int runningProcessID = -1;
int pidCounter = 1;


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

    char* state;
    void * stack;
    int status;
    bool isBlocked;
    bool isZombie;
    int (*startFunc)(char*);
    char* arg;

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
void removeNode(int);


void runProcess(int pid){
    USLOSS_Console("name of process running : %s\n",getProcess(pid)->name);


    if (getpid() == -1){
            //USLOSS_Console("inside if sttement \n");
        struct Process* newProc = getProcess(pid);
                //USLOSS_Console("1\n");

        newProc -> state = "Running";
        runningProcessID = pid;
        //USLOSS_Console("name of process: %s\n",&(processTable[pid % MAXPROC].name));

        USLOSS_ContextSwitch(NULL, &(processTable[pid % MAXPROC].context));
        //USLOSS_Console("1\n");
        return;
    }


    struct Process* oldProc = getProcess(runningProcessID); 

    if (strcmp(oldProc->state,"Terminated") ==1){
        oldProc -> state = "Runnable";
    }

    
    struct Process* newProc = getProcess(pid); 

    newProc -> state = "Running";   
    int prevID = runningProcessID; 
    runningProcessID = pid;

    USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, 
    &(processTable[pid % MAXPROC].context));

}

void printProcess(int pid){
    USLOSS_Console("Process Name: %s\n",&processTable[pid % MAXPROC].name);
}

void dispatcher(){
    int found = 0;
    int retPID;
    dumpQueue();
    dumpProcesses();
    for (int i = 1; i < 8; i++){
        struct Process * curProcess = runQueues[i];
        while (curProcess != NULL){

            USLOSS_Console("Process in question: %s\n",curProcess->name);

            if (strcmp(curProcess->state,"Runnable") ==0){
                runProcess(curProcess->PID);
                return;

            }
            
            curProcess = curProcess->nextQueueNode;

        }
        USLOSS_Console("priority %d has no runnable process\n",i);

        //USLOSS_Console("iteration: %d\n",i);
    }


}




void zap(int pid){

}

int isZapped(){
    return 0;
}

void blockMe(int block_status){
    (processTable[runningProcessID % MAXPROC]).state = "Blocked";
    dispatcher();
}

int unblockProc(int pid){
    return 0;
}

int readCurStartTime(){
    return 0;
}

void timeSlice(){

}

int readtime(){

}

int currentTime(){

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
    .prevSibling = NULL };

    processTable[slot] = p;
    strcpy(processTable[slot].name, name);
    processTable[slot].name[MAXNAME - 1] = '\0';

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
            USLOSS_Console("DEADLOCK DETECTED\n");
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
    USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
    if(retVal != 0){
        USLOSS_Console("ERROR MESSAGE");
    }

    // sets interrupts state to old state, and halts program, signifying 
    // the completion of the test case.
    USLOSS_PsrSet(psr);
    USLOSS_Halt(retVal);
}

/*
    Description: Boostrap process that simplt creates the init process
    using our createProcess function.

    Arg: none

    return: none
*/
void phase1_init(void) {
    // Initialize run queues


    for (int i = 0; i < 8; i++) {
        runQueues[i] = NULL;
    }
    
    createProcess("init", init_main, NULL, USLOSS_MIN_STACK, 6);


}

/**
     * Description: trampoline function that runs the childs main function.
     * 
     * Arg: none
     * 
     * return: none
    */
void trampoline(void){
    struct Process* curProcess = getProcess(getpid());
    int status = curProcess->startFunc(curProcess->arg);
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
    parent -> state = "Runnable";
    // USLOSS_ContextSwitch(&processTable[parentID%MAXPROC].context, NULL);

    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
    } else {
        parent -> firstChild -> prevSibling = newChild;
        newChild->nextSibling = parent->firstChild;
        parent->firstChild = newChild;
    }

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
    

    if (curChild == NULL){
        return -2;
    } 
    while (curChild != NULL){
        if (strcmp(curChild -> state,"Terminated")==0){
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


    return -2; 
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

    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if (mode != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Someone attempted to call quit while in user mode!\n");
        USLOSS_Halt(1);

    }
    int curPid = runningProcessID;


    struct Process* curProcess = getProcess(curPid);

    if (curProcess -> firstChild != NULL){
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", curPid);
        USLOSS_Halt(1);
    }
    
   
    curProcess->status = status;
    curProcess->state = "Terminated";
    removeNode(curProcess->PID);

    dispatcher();
}

void removeNode(int pid){
    struct Process* p = getProcess(pid);
    int priority = p->priority;

    struct Process* curProcess = runQueues[priority];
    if (curProcess != NULL && curProcess->PID == pid){
        runQueues[priority] = curProcess -> nextQueueNode;
        curProcess -> nextQueueNode -> prevQueueNode = NULL;
    }
    else{
        while (curProcess != NULL && curProcess->PID != pid){curProcess = curProcess -> nextQueueNode;}

        struct Process* prevNode = curProcess -> prevQueueNode;
        struct Process* nextNode = curProcess -> nextQueueNode;
        prevNode -> nextQueueNode = nextNode;
        nextNode -> prevQueueNode = prevNode;
    }
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

    dispatcher();

}



/*
    Description: sets the current process to runnable, next process to 
    running, and the context switches to the next process given by the pid.

    Arg: int pid: pid of process to switch to

    return; none
*/
// void TEMP_switchTo(int pid) {

//     struct Process* oldProc = getProcess(runningProcessID); 
//     oldProc -> state = "Runnable";
//     struct Process* newProc = getProcess(pid); 

//     newProc -> state = "Running";   
//     int prevID = runningProcessID; 
//     runningProcessID = pid;


//     USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, 
//     &(processTable[pid % MAXPROC].context));

// }



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
    
    USLOSS_Console("init main started\n");
    
    
    fork1("sentinel", sentinel, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
    //dumpQueue();
    //fork1("testcase_main2", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
    dumpProcesses();
    dumpQueue();
    //USLOSS_Console("end of initmain");

    dispatcher();
    // USLOSS_ContextSwitch(NULL, &(processTable[3].context));
    // TEMP_switchTo(3);
    USLOSS_Halt(0);
    return 0;
}