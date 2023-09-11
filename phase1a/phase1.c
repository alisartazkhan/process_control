#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
#include <usloss.h> // Assuming USLOSS functions/types are declared here
#include <stdbool.h>
#include <string.h>


int runningProcessID = 1;
int pidCounter = 1;

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

int findOpenProcessTableSlot();
void addChildToParent(struct Process* ,struct Process*);
void printChildren(struct Process*);
// void initQueue(Queue*);
// void addToRunQueue(int, Queue*);
void dumpProcesses();


int createProcess(char *name, void *startFunc, void *arg, int stackSize, int priority) {
    // Disable interrupts  
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    // USLOSS_Console("SLOT: %d\n", slot);
    if (slot == -1) { // IMPLEMENT 
        USLOSS_Console("No slot available");
        USLOSS_Halt(1);
        return -1; 
    }

    struct Process p = {.PID = pidCounter-1, .priority = priority, .stack = malloc(stackSize), .startFunc = startFunc, .arg = arg, .isInitialized = 1, .state = "READY", .nextSibling = NULL, .prevSibling = NULL };
    processTable[slot] = p;
    strcpy(processTable[slot].name, name);
    processTable[slot].name[MAXNAME - 1] = '\0';


    // Initialize context
    USLOSS_ContextInit(&(processTable[slot].context), processTable[slot].stack, stackSize, NULL, startFunc);

    // Restore interrupts
    USLOSS_PsrSet(old_psr);

    return pidCounter-1;
}


int init_main(char *arg){
    // int i = 0;
    // while (i<1){
    //     USLOSS_Console("init_main() running...\n");
    //     i++;}
    // USLOSS_Console("init_main() DONE.\n");
    //USLOSS_ContextSwitch(&processTable[1].context, &(processTable[2].context));

    TEMP_switchTo(3);

    USLOSS_Halt(0);
    return 1;
}

int sentinel(char *arg){
    // int i = 0;
    // while (i<10){
    //     USLOSS_Console("sentinel() running: %d\n", i);
    //     if (i==5){
    //         TEMP_switchTo(3);  
    //         ///USLOSS_ContextSwitch(&processTable[2].context, &(processTable[3].context));
    //   }
    //     i++;}
    // USLOSS_Console("sentinel Done.\n");
    // quit(0, 1);
    return 1;
}






// Initialize the Phase 1 kernel
void phase1_init(void) {

    createProcess("init", init_main, NULL, USLOSS_MIN_STACK, 6);

    fork1("sentinel", sentinel, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", testcase_main, NULL,USLOSS_MIN_STACK,3);



    // // Initialize run queues
    // for (int i = 0; i < NUM_PRIORITIES; i++) {
        
    //     //initQueue(runQueues[i]); 
    // }


    // fork1("1st child",NULL,NULL,USLOSS_MIN_STACK,7);
    // fork1("2nd child",NULL,NULL,USLOSS_MIN_STACK,3);
    // fork1("3rd child",NULL,NULL,USLOSS_MIN_STACK,3);

    // runningProcessID = 3;
    // fork1("4th child",NULL,NULL,USLOSS_MIN_STACK,3);
    // fork1("5th child",NULL,NULL,USLOSS_MIN_STACK,3);

    // runningProcessID = 4;
    // fork1("6th child",NULL,NULL,USLOSS_MIN_STACK,3);
    // fork1("7th child",NULL,NULL,USLOSS_MIN_STACK,3);
    
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = createProcess(name, func, arg, stacksize, priority);

    int newChildSlot = pid % MAXPROC;
    int parentID = getpid();

    struct Process* newChild = &processTable[newChildSlot]; // accessing new child from the array
    newChild->parent = &processTable[parentID%MAXPROC]; // setting the parent in the child struct
    struct Process* parent = &processTable[parentID%MAXPROC]; // accessing parent struct from the array
    // USLOSS_Console("parentID: %d,newChildID: %d\n",parent->PID, newChild->PID);
    //dumpProcesses();

    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
    } else {
        parent -> firstChild -> prevSibling = newChild;
        newChild->nextSibling = parent->firstChild;
        parent->firstChild = newChild;
    }
    return pid; // Dummy return
}

// Wait for a child process to terminate
int join(int *status) {
    // Your implementation here

    //printf("join started");

    struct Process* parent = &processTable[runningProcessID % MAXPROC];
    struct Process *curChild = parent -> firstChild;
    
    //printf("pointers created");

    if (curChild == NULL){
        //printf("no child found");
        return -2;
    } 
    while (curChild != NULL){
        //printf("%s\n", curChild->state);
        if (strcmp(curChild -> state,"ZOMBIE")==0){
            //printf("zombie found");
            if (curChild->prevSibling != NULL){
                printf("Zombie is not first child\n");
            curChild->prevSibling->nextSibling = curChild->nextSibling;
            //printf("seg fault in here");
            }
            else{
                parent -> firstChild = curChild -> nextSibling;
            }
            if (curChild->nextSibling != NULL){
                printf("Zombie is not =last child\n");
            curChild->nextSibling->prevSibling = curChild->prevSibling;
            }
        curChild -> state = "REMOVED";
        struct Process blank = {};
        processTable[curChild->PID % MAXPROC] = blank;
        return 0;

        } 
        //printf("checking next child\n");
        curChild = curChild->nextSibling;
        
    } 


    return 0; // Dummy return
}


// Terminate the current process
void quit(int status, int switchToPid) {
    int curPid = runningProcessID;

    struct Process* curProcess = &processTable[curPid % MAXPROC];
    //struct Process* switchToProcess = &processTable[switchToPid % MAXPROC];

    curProcess->status = status;
    curProcess->state = "ZOMBIE";


    // Assuming 'stack' is a pointer field in the struct, to be freed.
    free(curProcess->stack);

    struct Process* newProcess = &(processTable[switchToPid % MAXPROC]);
    newProcess -> state = "RUNNING";

    runningProcessID = switchToPid;

    USLOSS_ContextSwitch(NULL, &(processTable[switchToPid % MAXPROC].context));
}

// Get the ID of the current process
int getpid(void) {
    // Your implementation here
    return runningProcessID; // Dummy return
}


// Start the processes (never returns)
void startProcesses(void) {
    struct Process* init = &(processTable[1]);
    init -> state =  "RUNNING";
    runningProcessID = 1;
    dumpProcesses();
    USLOSS_ContextSwitch(NULL, &(processTable[1].context));

}

// TEMP_switchTo (Assuming this function is optional)
void TEMP_switchTo(int pid) {
    struct Process* oldProc = &processTable[runningProcessID % MAXPROC]; 
    oldProc -> state = "READY";
    struct Process* newProc = &processTable[pid % MAXPROC]; 
    newProc -> state = "RUNNING";   
    int prevID = runningProcessID; 
    runningProcessID = pid;
    //dumpProcesses();
    USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, &(processTable[pid % MAXPROC].context));

}


int findOpenProcessTableSlot(){
    // MAXPROC
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

void dumpProcesses(void){
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