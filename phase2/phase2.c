#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>

#define MAX_SLOTS 2500
#define MAX_MESSAGE 150

void phase2_clockHandler();
int phase2_check_io();


/*
    Struct that represents an individual process. Each field represents an
    attribute for a individual process. When a process is created, it's
    state is switched to Ready, and all of the fields passed in by fork
    are initialized, as well as isInitialized being set to 1. When a process
    is called using the dispatcher and its PID, its state is switched to 
    Running, and its startFunc is called. When a process has run to it's 
    entirety, its state is changed to Terminated, and after it has been
    joined, it is removed from its parent tree and its stack is freed.

*/
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

    struct Process* queueHead;
    struct Process* nextQueueNode;
    struct Process* prevQueueNode;

    struct Process* zapHead;
    struct Process* nextZapNode;
    struct Process* prevZapNode;
    struct Process* iZap;

    char* state;
    void * stack;
    int status;
    bool isBlocked;
    int isZapped;
    bool isZombie;
    int (*startFunc)(char*);
    char* arg;
    int time;
    int startTime;
    int endTime;
    int totalTime;

    // phase2 fields


};


struct MB {
  int id;
  Message* messages;
  int slotSize;
  struct Process* senderQueueHead;
  struct Process* receiverQueueHead;
};

struct Message {
  char* payload;
}

static struct Process shadowProcessTable[MAXPROC];
static struct MB mbTable[MAXMBOX];
static struct Message MessageTable[MAX_SLOTS];

void phase2_clockHandler(){

}

int phase2_check_io(){
  return 0;
}

struct Message createMessage(int slotSize){


}

void fork(char *name, int(func)(char *), char *arg, int stacksize, int priority){
  int pid = fork1(name, func, arg, stacksize, priority);


}


// Initialize mailboxes and other data structures
void phase2_init() {

}

void phase2_start_service_processes(){

}

// Create a new mailbox
int MboxCreate(int slots, int slotSize) {

  return 0;
}

// Release a mailbox
int MboxRelease(int mboxId) {

  return 0;
}

// Send message to mailbox
int MboxSend(int mboxId, void *msgPtr, int msgSize) {
  

  return 0;
}

// Receive message from mailbox  
int MboxRecv(int mboxId, void *msgPtr, int msgMaxSize) {


  return 0;
}

// Conditional send
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
  // Implementation similar to MboxSend but 
  // returns -2 instead of blocking if full

  return 0;
} 

// Conditional receive  
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
  // Implementation similar to MboxRecv but
  // returns -2 instead of blocking if empty
    return 0;

}

// Wait for interrupt
void waitDevice(int type, int unit, int *status) {

  // Check for invalid args

  // Block calling process

  // When interrupt occurs, copy status and unblock
}

// Interrupt handlers 

// System call handler
void nullsys() {
  // Print error and halt 
}