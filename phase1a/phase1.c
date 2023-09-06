#include "phase1.h"
#include <stdio.h>
#include <stdlib.h>
#include <usloss.h> // Assuming USLOSS functions/types are declared here

void phase1_init(void) {

  // Disable interrupts 
  int old_psr = USLOSS_PsrGet();
  USLOSS_PsrSet(old_psr & ~USLOSS_PSR_CURRENT_INT);

  // Initialize process table
  memset(proctable, 0, sizeof(proctable));

  // Create entry for init process
  proctable[1].pid = 1;
  proctable[1].status = CREATED;
  proctable[1].priority = 6;
  strcpy(proctable[1].name, "init");

  // Initialize run queues
  for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
    runqueues[i] = NULL;
  }

  // Restore interrupts
  USLOSS_PsrSet(old_psr);
  
  USLOSS_Console("Inside phase1_init\n");
}

// Create a new process  
int fork1(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
  int pid;
  
  // Error checking
  if (stacksize < USLOSS_MIN_STACK) 
    return -2;

  // Allocate process table entry    
  pid = allocate_proc_table_entry(); 
  if (pid == -1)
    return -1;

  // Initialize process table fields
  proctable[pid].name = name;
  proctable[pid].parent_pid = getpid();
  proctable[pid].priority = priority;
  proctable[pid].status = READY;
  
  // Allocate stack
  proctable[pid].stack = malloc(stacksize);

  // Initialize context
  USLOSS_ContextInit(&proctable[pid].context, proctable[pid].stack, stacksize, NULL, wrapper);

  // Add to run queue
  add_to_run_queue(pid);

  return pid;
}

// Wait for a child process to terminate
int join(int *status) {
  int pid;
  
  // Wait for child 
  pid = wait_for_child();
  if (pid == -1)
    return -2;
  
  // Get status
  *status = proctable[pid].exit_status;
  
  // Cleanup    
  free(proctable[pid].stack);
  free_proc_table_entry(pid);

  return pid;  
}

// Terminate the current process
void quit(int status) {
  int pid = getpid();

  // Set exit status
  proctable[pid].exit_status = status;  

  // Cleanup
  free(proctable[pid].stack);
  free_proc_table_entry(pid);

  // Switch contexts
  USLOSS_ContextSwitch(NULL, &proctable[1].context);
}

// Get the ID of the current process
int getpid(void) {
  // Get current running process
  return running_pid; 
}

// Dump process information 
void dumpProcesses(void) {
  int i;

  for (i = 0; i < MAXPROC; i++) {
    if (proctable[i].status != UNUSED) {
      print_process(&proctable[i]); 
    }
  }
}

// Start processes (never returns)
void startProcesses(void) {
  // Create and start init process
  int pid = fork1("init", init, NULL, USLOSS_MIN_STACK, 6);
  
  // Switch to init
  USLOSS_ContextSwitch(NULL, &proctable[pid].context);
}

// TEMP_switchTo (Assuming this function is optional)
void TEMP_switchTo(int pid) {
    // Your implementation here
}
