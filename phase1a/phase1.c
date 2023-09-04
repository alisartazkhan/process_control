#include "phase1.h"

// Initialize the Phase 1 kernel
void phase1_init(void) {
    // Your code here
}

// Create a new process
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    // Your code here
    return 0; // Dummy return
}

// Wait for a child process to terminate
int join(int *status) {
    // Your code here
    return 0; // Dummy return
}

// Terminate the current process
void quit(int status, int switchToPid) {
    // Your code here
    // No return as the function has __attribute__((__noreturn__))
}

// Get the ID of the current process
int getpid(void) {
    // Your code here
    return 0; // Dummy return
}

// Dump the process information
void dumpProcesses(void) {
    // Your code here
}

// Start the processes (never returns)
void startProcesses(void) {
    // Your code here
    // This function never returns
}

#if 1  // ADDED. This is *NOT* part of Phase 1b, but I'm adding it to 1a
void TEMP_switchTo(int pid) {
    // Your code here
}
#endif

// Functions that will be defined in later phases (Phase 5)
USLOSS_PTE *phase5_mmu_pageTable_alloc(int pid) {
    // Your code here (or NOP)
    return NULL; // Dummy return
}

void phase5_mmu_pageTable_free(int pid, USLOSS_PTE* pageTable) {
    // Your code here (or NOP)
}

// And so on for other functions...
