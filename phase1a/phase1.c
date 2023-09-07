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

};

struct Process processTable[MAXPROC];

int findOpenProcessTableSlot();



// Initialize the Phase 1 kernel
void phase1_init(void) {

    int slot = findOpenProcessTableSlot();
    struct Process init = {.PID = slot, .name = "init", .priority = 6};
    processTable[slot] = init;

    //processTable[slot].startFunc = init;
    //fork1("hello\0",NULL,NULL,10,5);
    //fork1("ihi\0",NULL,NULL,100,3);

    fork1("sentinal",NULL,NULL,USLOSS_MIN_STACK,7);
    //fork1("testcasemain",NULL,NULL,USLOSS_MIN_STACK,3);
    printf("Fork1 called and done running");
    
    
    for (int i = 0; i < MAXPROC; i++){
        printf("%dth index ID: %d   %s   %d   parentid: %d\n",i,processTable[i].PID,processTable[i].name,processTable[i].priority, (processTable[i].parent)->PID);
    }


    // Your implementation here


    //USLOSS_Console("helllooooooooo");



    //while (true) {

//        join()
//   }
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {

    int pid = currentPID;
    struct Process p = {.PID = pid, .priority = priority, .stack = malloc(stacksize)};

    strncpy(p.name, name, MAXNAME); // Copy the name into the name field and ensure it's null-terminated
    p.name[MAXNAME - 1] = '\0';


    int slot = findOpenProcessTableSlot();
    
    processTable[slot] = p;

    //int parentID = getpid();
    //printf("Parent ID: %d\n", parentID);
    //p.parent = &processTable[parentID%MAXPROC];
    



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