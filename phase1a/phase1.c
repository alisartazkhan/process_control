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
    struct Process* prevSibiling;
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


struct Process processTable[MAXPROC];

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

int createProcess(char *name, void *startFunc, void *arg, int priority, int stackSize) {
  
    // Disable interrupts  
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    if (slot == -1) { // IMPLEMENT 
        return -1; 
    }

    struct Process p = {.PID = pidCounter, .priority = priority, .stack = malloc(stackSize), .startFunc = startFunc, .arg = arg};
    processTable[slot] = p;
    strcpy(processTable[slot].name, name);

    // Initialize context
    USLOSS_ContextInit(&processTable[slot].context, processTable[slot].stack, stackSize, NULL, startFunc);

    // Restore interrupts
    USLOSS_PsrSet(old_psr);

    return pidCounter;
}


// Initialize the Phase 1 kernel
void phase1_init(void) {

    // int slot = findOpenProcessTableSlot();
    // struct Process init = {.PID = slot, .name = "init", .priority = 6, .isInitialized = 1};
    // processTable[slot] = init;

    createProcess("init", NULL, NULL, 6, USLOSS_MIN_STACK);

    fork1("1st child",NULL,NULL,USLOSS_MIN_STACK,7);
    fork1("2nd child",NULL,NULL,USLOSS_MIN_STACK,3);
    fork1("3nd child",NULL,NULL,USLOSS_MIN_STACK,3);

    runningProcessID = 3;
    fork1("4th child",NULL,NULL,USLOSS_MIN_STACK,3);
    fork1("5th child",NULL,NULL,USLOSS_MIN_STACK,3);

    runningProcessID = 4;
    fork1("6th child",NULL,NULL,USLOSS_MIN_STACK,3);
    fork1("7th child",NULL,NULL,USLOSS_MIN_STACK,3);
    
    printTable();

}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = pidCounter;
    struct Process child = {.PID = pid, .priority = priority, .stack = malloc(USLOSS_MIN_STACK), .startFunc = func, .arg = arg, .isInitialized = 1};
    strncpy(child.name, name, MAXNAME); // Copy the name into the name field and ensure it's null-terminated
    child.name[MAXNAME - 1] = '\0';
    

    int newChildSlot = findOpenProcessTableSlot();
    processTable[newChildSlot] = child;
    int parentID = getpid();
    child.parent = &processTable[parentID%MAXPROC];

    struct Process* parent = &processTable[parentID%MAXPROC];
    struct Process* newChild = &processTable[newChildSlot%MAXPROC];
    printf("----------------%d\n", newChild->PID);

    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
        // printf("%s [first child] inside if statement\n ",parent->firstChild->name);

    } else {
        // printf("%s [first child] before\n",parent->firstChild->name);
        newChild->nextSibling = parent->firstChild;
        // printf("%s [old first child] after\n",newChild->nextSibling->name);
        parent->firstChild = newChild;

        // printf("%s (P) -> ",parent->name);
        // printf("%s -> ",parent->firstChild->name);
        // printf("%s\n",parent->firstChild->nextSibling->name);
        // printf("In C2P: parent: %p, child: %p, sibling: %p\n", &parent, &(parent->firstChild), &(parent->firstChild->nextSibling));
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
    printf("child list for %d:    ", parent->PID);
    
    while (curChild != NULL){
        printf("%s ",curChild->name);
        curChild = curChild->nextSibling;
    }
    printf("\n");

}