#include "phase1.h"
#include <stdio.h>
#include <usloss.h> // Assuming USLOSS functions/types are declared here

// Initialize the Phase 1 kernel
void phase1_init(void) {
    // Your implementation here
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
