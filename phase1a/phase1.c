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
    void * stack;
    int returnStatus;
    int (*startFunc)(char*);
    char* arg;

};

// Run queue struct 
typedef struct Queue {
  int head;
  int tail;
  int count;
} Queue;


struct Process processTable[10];
struct Process* runQueues[5]; // NUM_PRIORITIES = 5

int findOpenProcessTableSlot();
void addChildToParent(struct Process* ,struct Process*);
void printChildren(struct Process*);
void initQueue(Queue*);
void addToRunQueue(int, Queue*);
void printTable();

// Initialize queue 
void initQueue(Queue* q) {
  q->head = 0;
  q->tail = 0;
  q->count = 0;
}

// Add process to run queue
void addToRunQueue(int pid, Queue* q) {
  
//   // Set links
//   proctable[pid].next = NULL;
//   proctable[pid].prev = NULL;

//   if (q->count == 0) {
//     // Queue empty
//     q->head = pid;
//     q->tail = pid;
//   } else {
//     // Add to tail
//     proctable[q->tail].next = pid;
//     proctable[pid].prev = q->tail;
//     q->tail = pid;
//   }

//   q->count++;

}

int createProcess(char *name, void *startFunc, void *arg, int stackSize, int priority) {
  
    // Disable interrupts  
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    printf("SLOT: %d\n", slot);
    if (slot == -1) { // IMPLEMENT 
        return -1; 
    }

    struct Process p = {.PID = pidCounter-1, .priority = priority, .stack = malloc(stackSize), .startFunc = startFunc, .arg = arg};
    processTable[slot] = p;
    strcpy(processTable[slot].name, name);
    processTable[slot].name[MAXNAME - 1] = '\0';


    // Initialize context
    USLOSS_ContextInit(&processTable[slot].context, processTable[slot].stack, stackSize, NULL, startFunc);

    // Restore interrupts
    USLOSS_PsrSet(old_psr);

    return pidCounter-1;
}

// void testcase_main(){
//     int i = 0;
//     while (i<100){i++;}
//     printf("testcase_main() running...\n");
// }

// void sentinel(){
//     int i = 0;
//     while (i<100){i++;}
//     printf("sentinel running...\n");
// }

// Initialize the Phase 1 kernel
void phase1_init(void) {

    // int slot = findOpenProcessTableSlot();
    // struct Process init = {.PID = slot, .name = "init", .priority = 6, .isInitialized = 1};
    // processTable[slot] = init;

    createProcess("init", NULL, NULL, USLOSS_MIN_STACK, 6);
    

    fork1("sentinel", NULL, NULL,USLOSS_MIN_STACK,7);

    //fork1("sentinel", NULL, NULL,USLOSS_MIN_STACK,7);
    fork1("testcase_main", NULL, NULL,USLOSS_MIN_STACK,3);

    // USLOSS_ContextSwitch(NULL, &processTable[1].context);

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

    printTable();
    
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = createProcess(name, func, arg, stacksize, priority);

    int newChildSlot = pid % MAXPROC;
    int parentID = getpid();

    struct Process* newChild = &processTable[newChildSlot]; // accessing new child from the array
    newChild->parent = &processTable[parentID%MAXPROC]; // setting the parent in the child struct
    struct Process* parent = &processTable[parentID%MAXPROC]; // accessing parent struct from the array
    printf("parentID: %d,newChildID: %d\n",parent->PID, newChild->PID);
    //printTable();

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
    return 0; // Dummy return
}

// Terminate the current process
void quit(int status, int switchToPid) {
    // Your implementation here
    // No return as the function has __attribute__((__noreturn__))
    abort();
}

// Get the ID of the current process
int getpid(void) {
    // Your implementation here
    return runningProcessID; // Dummy return
}

// Dump the process information
void dumpProcesses(void) {
    // Your implementation here
}

// Start the processes (never returns)
void startProcesses(void) {
    // Your implementation here
    // This function never returns
}

// TEMP_switchTo (Assuming this function is optional)
void TEMP_switchTo(int pid) {
    // Your implementation here
}


int findOpenProcessTableSlot(){
    while (true){
        int slot = pidCounter % MAXPROC;
        pidCounter++;

        if (processTable[slot].isInitialized == 0){
            return slot;
        }

    }


}



void printChildren(struct Process* parent){
    struct Process* curChild = parent->firstChild;
    printf("child list for %d: ", parent->PID);
    
    while (curChild != NULL){
        printf("%s -> ",curChild->name);
        curChild = curChild->nextSibling;
    }
    printf("NULL\n");

}

void printTable(){
    for (int i = 0; i < 10; i++){
        if (processTable[i].PID == 0){
            printf("%d: PID: -- | NAME: -- | PRIORITY: -- | \n", i);
            continue;
        }
        printf("%d: PID: %d | NAME: %s | PRIORITY: %d | ",i,processTable[i].PID,processTable[i].name,processTable[i].priority);
        printChildren(&processTable[i]);
    }
    printf("-------------------------------------\n \n");

}