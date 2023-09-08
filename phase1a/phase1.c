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
    struct Process* nextSibling;
    struct Process* prevSibiling;
    struct Process* parent;
    struct Process* firstChild;
    void * stack;

    int (*startFunc)(char*);
    char* arg;

};

struct Process processTable[MAXPROC];

int findOpenProcessTableSlot();
void addChildToParent(struct Process* ,struct Process*);
void printChildren(struct Process*);




// Initialize the Phase 1 kernel
void phase1_init(void) {

    int slot = findOpenProcessTableSlot();
    struct Process init = {.PID = slot, .name = "init", .priority = 6, .isInitialized = 1};
    processTable[slot] = init;


    fork1("1st child",NULL,NULL,USLOSS_MIN_STACK,7);
    fork1("2nd child",NULL,NULL,USLOSS_MIN_STACK,3);
    fork1("3nd child",NULL,NULL,USLOSS_MIN_STACK,3);

    
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


    // Your implementation here


    //USLOSS_Console("helllooooooooo");



    //while (true) {

//        join()
//   }
}

void addChildToParent(struct Process* newChild ,struct Process* parent){
    //printf("add child to parent");

    printf("In C2P: parent: %p, child: %p\n", &parent, &newChild);

    if (parent->firstChild == NULL){
        parent->firstChild = newChild;
        printf("%s [first child]\n ",parent->firstChild->name);

    }
    else {
        printf("%s [first child] before\n",parent->firstChild->name);
        newChild->nextSibling = parent->firstChild;
        printf("%s [old first child] after\n",newChild->nextSibling->name);

        parent->firstChild = newChild;

        printf("%d (P) -> ",parent->PID);
        printf("%d -> ",parent->firstChild->PID);
        printf("%d\n",parent->firstChild->nextSibling->PID);
        printf("In C2P: parent: %p, child: %p, sibling: %p\n", &parent, &(parent->firstChild), &(parent->firstChild->nextSibling));
    }
}
// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = currentPID;
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
        printf("%s [first child] inside if statement\n ",parent->firstChild->name);

    } else {
        printf("%s [first child] before\n",parent->firstChild->name);
        newChild->nextSibling = parent->firstChild;
        printf("%s [old first child] after\n",newChild->nextSibling->name);
        parent->firstChild = newChild;

        printf("%s (P) -> ",parent->name);
        printf("%s -> ",parent->firstChild->name);
        printf("%s\n",parent->firstChild->nextSibling->name);
        printf("In C2P: parent: %p, child: %p, sibling: %p\n", &parent, &(parent->firstChild), &(parent->firstChild->nextSibling));
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
        int slot = currentPID % MAXPROC;
        currentPID++;

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