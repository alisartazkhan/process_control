#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
#include <usloss.h> // Assuming USLOSS functions/types are declared here

#include <stdbool.h>




int currentPID = 1;

struct Process {
    bool isInitialized = false;
    char name[MAXNAME];
    int PID;
    int priority;
    bool isAlive;
    struct Process* nextSibling;
    struct Process* prevSibiling;
    struct Process* parent;
    struct Process* firstChild;

};

struct Process processTable[MAXPROC];

int findOpenProcessTableSlot();


// Initialize the Phase 1 kernel
void phase1_init(void) {

    
    int slot = findOpenProcessTableSlot();

    struct Process init = {.PID = slot, .name = "init"};
    processTable[slot] = init;


    for (int i = 0; i < MAXPROC; i++){
        printf("%dth index ID: %d\n",i,processTable[i].PID);
        printf("%dth index name: %s\n",i,processTable[i].name);
    }


    // Your implementation here


    //USLOSS_Console("helllooooooooo");

    //fork1(sentinal)
    //fork1(testcasemain)

    //while (true) {

//        join()
//   }
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {


    // Your implementation here
    return 0; // Dummy return
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
    return 0; // Dummy return
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
    //while (true){
        int slot = currentPID % MAXPROC;
        currentPID++;
        return slot; 

        //if (processTable[slot] == "empty"){

        //}

    //}


}