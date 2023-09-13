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
void dumpProcesses();
void trampoline(void);
struct Process* getProcess(int);
void printProcess (struct Process*);

int createProcess(char *name, int (*startFunc)(char*), void *arg, int stackSize, int priority) {
    // Disable interrupts  
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    // USLOSS_Console("SLOT: %d\n", slot);
    if (slot == -1) { // IMPLEMENT 
        return -1; 
    }

    struct Process p = {.PID = pidCounter-1, .priority = priority, .stack = malloc(stackSize), .startFunc = startFunc, .arg = arg, .isInitialized = 1, .state = "Runnable", .nextSibling = NULL, .prevSibling = NULL };

    if (p.stack != NULL){
    //printf("Malloc in created process is correct\n");
    }

    processTable[slot] = p;
    strcpy(processTable[slot].name, name);
    processTable[slot].name[MAXNAME - 1] = '\0';


    // Initialize context
    USLOSS_ContextInit(&(processTable[slot].context), processTable[slot].stack, stackSize, NULL, trampoline);

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
    USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to testcase_main() after using fork1() to create it.\n");

    TEMP_switchTo(3);

    USLOSS_Halt(0);
    return 1;
}

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


void testcase_main_local(){
    // Save interrupt state
    int psr = USLOSS_PsrGet();
    
    // Enable interrupts
    USLOSS_PsrSet(psr | USLOSS_PSR_CURRENT_INT);

    int retVal = testcase_main();
    USLOSS_Console("Phase 1A TEMPORARY HACK: testcase_main() returned, simulation will now halt.\n");
    if(retVal != 0){
        printf("ERROR MESSAGE");
    }
    // Restore interrupt state
    USLOSS_PsrSet(psr);

    USLOSS_Halt(retVal);

}



// Initialize the Phase 1 kernel
void phase1_init(void) {

    createProcess("init", init_main, NULL, USLOSS_MIN_STACK, 6);

    

    fork1("sentinel", sentinel, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", testcase_main_local, NULL,USLOSS_MIN_STACK,3);
}

void trampoline(void){
    /**
     * Trampoline:
     * Gets return number from child main and pass that in for quit()
     * enable interrupts
     * if process returns to you call quit
    */
    struct Process* curProcess = getProcess(getpid());
    int status = curProcess->startFunc(curProcess->arg);
    printf("FINISHED RUNNING CHILD MAIN\n");

    //quit(status, curProcess->parent->PID);
}

struct Process* getProcess(int pid) {

    
    return &processTable[pid%MAXPROC];
}
// Create a new process
int fork1(char *name, int(func)(char *), char *arg, int stacksize, int priority) {
    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if (mode != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Someone attempted to call fork1 while in user mode!\n");
        USLOSS_Halt(1);
    }

    if (stacksize < USLOSS_MIN_STACK){
        return -2;
    }
    int pid = createProcess(name, func, arg, stacksize, priority);
    if (pid == -1 || (priority > 7 || priority < 1)){
        return -1;
    }

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
    
    struct Process* parent = getProcess(runningProcessID);
    struct Process* curChild = parent -> firstChild;
    
    //printf("pointers created");

    if (curChild == NULL){
        //printf("no child found");
        return -2;
    } 
    while (curChild != NULL){
        //printf("%s\n", curChild->state);
        if (strcmp(curChild -> state,"Terminated")==0){
            //printf("zombie found");
            if (curChild->prevSibling != NULL){
                //printf("Zombie is not first child\n");
            curChild->prevSibling->nextSibling = curChild->nextSibling;
            //printf("seg fault in here");
            }
            else{
                parent -> firstChild = curChild -> nextSibling;
            }
            if (curChild->nextSibling != NULL){
                //printf("Zombie is not =last child\n");
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
        //printf("checking next child\n");
        curChild = curChild->nextSibling;
        
    } 


    return -2; // Dummy return
}

void printProcess (struct Process* p){
     USLOSS_Console("PID: %d | NAME: %s | PRIORITY: %d | STATE: %s | ADDRESS: %p\n",p->PID,p->name,p->priority, p->state, &p);
}
// Terminate the current process
void quit(int status, int switchToPid) {

    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    if (mode != USLOSS_PSR_CURRENT_MODE) {
        USLOSS_Console("ERROR: Someone attempted to call quit while in user mode!\n");
        USLOSS_Halt(1);

    }
    int curPid = runningProcessID;

    // is quit being called on the right process? is running process the parent or the terminated process?

    struct Process* curProcess = getProcess(curPid);
    //("PID: %d | ADDRESS: %p\n", curProcess->PID, (void *)curProcess);

    if (curProcess -> firstChild != NULL){
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", curPid);
        USLOSS_Halt(1);
    }

    // printProcess(curProcess);
        //printf("GOT CUR PROC\n");

    //struct Process* switchToProcess = &processTable[switchToPid % MAXPROC];

    curProcess->status = status;
    curProcess->state = "Terminated";
    //printf("AFTER SETTING ZOMBIE\n");

    //printf("BEFORE FREEING STACK\n");

    // Assuming 'stack' is a pointer field in the struct, to be freed.


    //free(curProcess->stack); //CAUSES SEG FAULT!!!!!!
    //printf("BEFORE NEW PROC\n");

    struct Process* newProcess = getProcess(switchToPid);
                //printf("good here");

        //printf("GOT NEW PROC\n");


    newProcess -> state = "Running";

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
    phase2_start_service_processes();
    phase3_start_service_processes();
    phase4_start_service_processes();
    phase5_start_service_processes();

    struct Process* init = &(processTable[1]);
    init -> state =  "Running";
    runningProcessID = 1;
    //dumpProcesses();
    //phase1_init()
    USLOSS_ContextSwitch(NULL, &(processTable[1].context));

}

// TEMP_switchTo (Assuming this function is optional)
void TEMP_switchTo(int pid) {
    


    //printf("TEMPSWITCH PID: %d\n", pid);
    struct Process* oldProc = getProcess(runningProcessID); 

   


    oldProc -> state = "Runnable";
    struct Process* newProc = getProcess(pid); 

    newProc -> state = "Running";   
    int prevID = runningProcessID; 
    runningProcessID = pid;

   //printf("free in temp works\n");

    //dumpProcesses();
    USLOSS_ContextSwitch(&processTable[prevID % MAXPROC].context, &(processTable[pid % MAXPROC].context));

}


// int findOpenProcessTableSlot(){
//     // MAXPROC
//     int i = pidCounter;
//     int j=0;
//     while (j<MAXPROC){
//         int slot = i % MAXPROC;
//         if (processTable[slot].isInitialized == 0){
//             pidCounter++;
//             return slot;
//         }
//         i = (i+1) % MAXPROC;
//         j++;
//     }

//     return -1;
// }

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

void dumpProcessesForUs(void){
    for (int i = 0; i < MAXPROC; i++){
        // if (processTable[i].PID == 0){
        //     USLOSS_Console("%d: PID: -- | NAME: -- | PRIORITY: -- | \n", i);
        //     continue;
        // }
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
        //     USLOSS_Console("%d: PID: -- | NAME: -- | PRIORITY: -- | \n", i);
             //printf("0000000");
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
        //USLOSS_Console("Running Process ID: %d\n",runningProcessID);
    //USLOSS_Console("-------------------------------------\n \n");

}

