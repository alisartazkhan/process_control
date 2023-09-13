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
int runningProcessID = 1;
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
    char* state;
    void * stack;
    int status;
    bool isBlocked;
    bool isZombie;
    int (*startFunc)(char*);
    char* arg;

};

struct Process processTable[MAXPROC];


// declaration for functions not already mane in phase.h
int findOpenProcessTableSlot();
void addChildToParent(struct Process* ,struct Process*);
void printChildren(struct Process*);
void dumpProcesses();
void trampoline(void);
struct Process* getProcess(int);
int init_main(char*);





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
    USLOSS_Console("FINISHED RUNNING CHILD MAIN\n");
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


    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
    } else {
        parent -> firstChild -> prevSibling = newChild;
        newChild->nextSibling = parent->firstChild;
        parent->firstChild = newChild;
    }

    
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
void quit(int status, int switchToPid) {

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

    struct Process* newProcess = getProcess(switchToPid);

    newProcess -> state = "Running";

    runningProcessID = switchToPid;


    USLOSS_ContextSwitch(NULL, &(processTable[switchToPid % MAXPROC].context));
}


/*
    Description: returns the PID of current process

    Arg: none

    return int: none
*/
int getpid(void) {
    return runningProcessID; 
}

void startProcesses(void) {
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes();

    struct Process* init = &(processTable[1]);
    init -> state =  "Running";
    runningProcessID = 1;
    USLOSS_ContextSwitch(NULL, &(processTable[1].context));

}


void TEMP_switchTo(int pid) {
    


    struct Process* oldProc = getProcess(runningProcessID); 


    oldProc -> state = "Runnable";
    struct Process* newProc = getProcess(pid); 

    newProc -> state = "Running";   
    int prevID = runningProcessID; 
    runningProcessID = pid;


    USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, &(processTable[pid % MAXPROC].context));

}




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


void printChildren(struct Process* parent){
    struct Process* curChild = parent->firstChild;
    USLOSS_Console("child list for %d: ", parent->PID);
    
    while (curChild != NULL){
        USLOSS_Console("%s -> ",curChild->name);
        curChild = curChild->nextSibling;
    }
    USLOSS_Console("NULL\n");

}

void dumpProcessesForUs(void){
    for (int i = 0; i < MAXPROC; i++){

        USLOSS_Console("%d: PID: %d | NAME: %s | PRIORITY: %d | STATE: %s | ADDRESS: %p | ",i,processTable[i].PID,processTable[i].name,processTable[i].priority, processTable[i].state, &processTable[i]);
        printChildren(&processTable[i]);
    }
        USLOSS_Console("Running Process ID: %d\n",runningProcessID);
    USLOSS_Console("-------------------------------------\n \n");

}

void dumpProcesses(void){
    USLOSS_Console(" PID  PPID  NAME              PRIORITY  STATE\n");
    for (int i = 0; i < MAXPROC; i++){
         if (processTable[i].isInitialized != 1){
             continue;;
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



/*
    Description: Called in boot strap process. Creates the sentinel
    and testcase_main processes, and context switched to testcase_main

    Arg: char *arg: main function to be ran

    return int: error code, but should be halted before it returns anyway
*/
int init_main(char *arg){
    fork1("sentinel", sentinel, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
    USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using fork1() to create it.\n");

    TEMP_switchTo(3);
    USLOSS_Halt(0);
    return 0;
}