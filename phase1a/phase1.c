#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
#include <usloss.h> // Assuming USLOSS functions/types are declared here

#include <stdbool.h>
#include <string.h>


int runningProcessID = 1;

int currentPID = 1;

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


// Initialize queue 
void initQueue(Queue* q) {
  q->head = 0;
  q->tail = 0;
  q->count = 0;
}


// Add process to run queue
void addToRunQueue(int pid, Queue* q) {
  
  // Set links
  proctable[pid].next = NULL;
  proctable[pid].prev = NULL;

  if (q->count == 0) {
    // Queue empty
    q->head = pid;
    q->tail = pid;
  } else {
    // Add to tail
    proctable[q->tail].next = pid;
    proctable[pid].prev = q->tail;
    q->tail = pid;
  }

  // Increment count
  q->count++;

}

int createProcess(char *name, void *startFunc, void *arg, int priority, int stackSize) {
  
    // Disable interrupts  
    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    int slot = findOpenProcessTableSlot();
    struct Process init = {.PID = slot, .name = "init", .priority = 6, .stack = malloc(USLOSS_MIN_STACK)};
    processTable[slot] = init;

    if (slot == -1) { // IMPLEMENT 
        return -1; 
    }

  // Initialize process attributes
  strcpy(proctable[pid].name, name);
  proctable[pid].priority = priority;
  proctable[pid].status = CREATED;

  // Allocate stack
  proctable[pid].stack = malloc(stackSize);

  // Initialize context
  USLOSS_ContextInit(&proctable[pid].context, proctable[pid].stack, 
                     stackSize, startFunc);

  // Restore interrupts
  USLOSS_PsrSet(old_psr);

  return pid;
}

// Initialize the Phase 1 kernel
void phase1_init(void) {

    int old_psr = USLOSS_PsrGet();
    USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

    for (int i = 0; i < NUM_PRIORITIES; i++) {
    initQueue(&runQueues[i]);
    }

    int slot = findOpenProcessTableSlot();
    struct Process init = {.PID = slot, .name = "init", .priority = 6, .stack = malloc(USLOSS_MIN_STACK)};
    processTable[slot] = init;
    USLOSS_ContextInit(&init->context, init->stack, USLOSS_MIN_STACK, NULL);





    //processTable[slot].startFunc = init;
    //fork1("hello\0",NULL,NULL,10,5);
    //fork1("ihi\0",NULL,NULL,100,3);




    fork1("sentinal",NULL,NULL,USLOSS_MIN_STACK,7);
    fork1("testcasemain",NULL,NULL,USLOSS_MIN_STACK,3);
    //printf("Fork1 called and done running\n");

    // //runningProcessID = 3;
    // //fork1("hello",NULL,NULL,USLOSS_MIN_STACK,7);


    
    
    for (int i = 0; i < MAXPROC; i++){
        
        if (i == 1){
            //printChildren(&processTable[i]);
            printf("%dth index ID: %d   %s   %d   \n",i,processTable[i].PID,processTable[i].name,processTable[i].priority);
        }

        if (processTable[i].parent == NULL){
            continue;
        }


        //printChildren(&processTable[i]);
        //fflush(stdout);
        printf("%dth index ID: %d   %s   %d   parentid:%d \n",i,processTable[i].PID,processTable[i].name,processTable[i].priority, processTable[i].parent->PID);
        //fflush(stdout);
    }
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = currentPID;
    struct Process child = {.PID = pid, .priority = priority, .stack = malloc(stacksize), .startFunc = func, .arg = arg};

    strncpy(child.name, name, MAXNAME); // Copy the name into the name field and ensure it's null-terminated
    child.name[MAXNAME - 1] = '\0';


    int slot = findOpenProcessTableSlot();
    
    

    int parentID = getpid();
    //printf("Parent ID: %d\n", parentID);
    child.parent = &processTable[parentID%MAXPROC];
    addChildToParent(&child, child.parent);
    
    processTable[slot] = child;
    



    //func(arg);


    // Your implementation here
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
        int slot = currentPID % MAXPROC;
        currentPID++;

        if (processTable[slot].isInitialized == 0){
            return slot;
        }

    }


}

void addChildToParent(struct Process* newChild, struct Process* parent) {
    if (parent->firstChild == NULL) {
        parent->firstChild = newChild;
        newChild->nextSibling = NULL; // no next sibling
    } else {
        newChild->nextSibling = &(parent->firstChild);
        parent->firstChild = newChild;
        
        // Debug prints (modify accordingly)
        printf("%d (P) -> ", parent->PID);
        printf("%d -> ", parent->firstChild->PID);
        printf("%d\n", newChild->nextSibling->PID);
        
    }
}


// void printChildren(struct Process* parent){
//     struct Process* curChild = parent->firstChild;
//     printf("child list for %d:    ", parent->PID);
    
//     while (curChild != NULL){
//         printf("%s ",curChild->name);
//         curChild = curChild->nextSibling;
//     }
//     printf("\n");

// }