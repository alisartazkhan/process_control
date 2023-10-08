#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>

// #define MAX_SLOTS 20
// #define MAX_MESSAGE 150

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
    int PID;
    struct Message* message;
    // phase2 fields
    struct Process* nextConsumerNode;
    struct Process* nextProducerNode;
};

struct Message {
  int id;
  int isInitialized;
  int messageSizeLimit;
  void* message;
  struct Message* nextMessage;
  struct Message* prevMessage;
};

struct MB {
  int isInitialized;
  int id;
  int mailSlotLimit;
  int messageSizeLimit;
  int mailSlotsAvailable;
  struct Message* mailSlotQueueHead;
  struct Process* consumerQueueHead;
  struct Process* producerQueueHead;
};

// index to insert into mbTable. Increments after every mb insertion
int mbIdCounter = 0;   
int messageIDCounter =0;
int pidCounter = 1;


// arrays
static struct Process shadowProcessTable[MAXPROC];
static struct MB mbTable[MAXMBOX];
static struct Message* messageSlots[MAXSLOTS];
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs*);



void phase2_clockHandler(){

}

int phase2_check_io(){
  return 0;
}

struct Message createMessage(int slotSize){


}


void dumpProcesses1(void){
    USLOSS_Console(" PID\n");
    for (int i = 0; i < MAXPROC; i++){
         if (shadowProcessTable[i].PID == 0){
             continue;
         }
        else {
          USLOSS_Console("%d\n",shadowProcessTable[i].PID);
        }
    } 
    USLOSS_Console("vsssssssssss\n");
    dumpProcesses();
}


// Initialize mailboxes and other data structures
void phase2_init() {
  int mbNumber = 0;
  for (int i = 0; i< 7; i++){
    MboxCreate(0,0);
  }
  for (int i = 0; i< MAXSLOTS; i++){
    struct Message* m = (struct Message*)malloc(sizeof(struct Message));
    if (m != NULL) {
        m->id = i;
        m->isInitialized = 0;
        m->message = NULL;
        m->messageSizeLimit = 0;
        m->nextMessage = NULL;
        m->prevMessage = NULL;
    }
    messageSlots[i] = m;
  }
}

void phase2_start_service_processes(){
  // phase2_init();
}

/** 
     * Description: getter function that return process
     *  
     * Arg: pid of process to retrieve
     * 
     * return: Process that you are looking for
    */
struct Process* getProcess(int pid) {
  if (pid != shadowProcessTable[pid%MAXPROC].PID){createShadowProcess(pid); }  
  return &shadowProcessTable[pid%MAXPROC];
}

struct MB* getMb(int mbId){
  return &mbTable[mbId%MAXMBOX];
}

/*
    Description: Creates a process and sets its fields to the
    arguments passed in. Allocates the stack memory and adds
    it to the processTable array.

    Arg: char *name: name of the process
    Arg: int (*startFunc)(char*): function to be run
    Arg: void *arg: argument to pass into startFunc
    Arg: int stackSize: size of stack to allocate
    Arg: int priority: priority of the process

    return int: the PID of the process
*/
int createShadowProcess(int pid) {
    struct Process p = {
      .PID = pid, 
      .nextConsumerNode = NULL,
      .nextProducerNode = NULL
    };

    shadowProcessTable[pid%MAXPROC] = p;

    return pid;
}

/*
    Description: iterates through the process table to find an open slot
    for the created process using pidCounter % MAXPROC.

    Arg: none

    return int: the found slot for the process or -1 if table is full
*/
int findOpenMbTableSlot(){
    int i = 0;
    while (i<MAXMBOX){
        int slot = mbIdCounter % MAXMBOX;
        mbIdCounter++;
        if (mbTable[slot].isInitialized == 0){
            return slot;}
        i++;
    }

    return -1;
}

int findOpenMessageSlot(){
    int i = 0;
    while (i<MAXSLOTS){
        int slot = messageIDCounter % MAXSLOTS;
        messageIDCounter++;
        if (messageSlots[slot]->isInitialized == 0){
            return slot;}
        i++;
    }

    return -1;
}


void printMbTable(){
  USLOSS_Console("PRINTING MailBox table: ");

  for (int i=0;i<MAXMBOX; i++){
    if (mbTable[i].isInitialized == 1){
      USLOSS_Console("ID: %d | MESSAGESIZELIMIT: %d | MAILSLOTLIMIT: %d | MAILSLOTSAVAILABLE: %d\n", 
      mbTable[i].id, mbTable[i].messageSizeLimit, mbTable[i].mailSlotLimit, mbTable[i].mailSlotsAvailable);
    }
  }
}

void printMessage(struct Process* p){
  USLOSS_Console("PRINTING MESSAGE FROM PROCESS: ");
  USLOSS_Console("PID: %d | MESSAGE: %s\n", p->PID, (char*)p->message);
}

// Create a new mailbox
int MboxCreate(int numSlots, int slotSize) {
  if (numSlots < 0 || slotSize < 0){return -1;}


  struct MB m = {
    .isInitialized = 1,
    .id = mbIdCounter,
    .mailSlotLimit = numSlots,
    .messageSizeLimit = slotSize,
    .mailSlotsAvailable = numSlots,
    .mailSlotQueueHead = NULL,
    .producerQueueHead = NULL,
    .consumerQueueHead = NULL
  };

  // USLOSS_Console("%d\n", sizeof(mbTable)/ sizeof(mbTable[0]));
  int slot = findOpenMbTableSlot();
  if (slot == -1) {return -1;}

  mbTable[slot] = m;

  return mbIdCounter - 1;
}

// Release a mailbox
int MboxRelease(int mboxId) {

  return 0;
}


// Send message to mailbox
int MboxSend(int mboxId, void *msgPtr, int msgSize) {
  struct MB* mb = getMb(mboxId);
  if (msgSize != 0 && msgPtr == NULL){
    return -1;
  }

  if (msgSize > mb->messageSizeLimit){
    // ERROR MESSAGE
    USLOSS_Console("ERROR: Message Size it too big for mb\n");
    return -1;
  }

  // struct Message* m = (struct Message*)malloc(sizeof(struct Message));

  // if (m != NULL) {
  //     m->isInitialized = 1;
  //     m->message = msgPtr;
  //     m->messageSizeLimit = msgSize;
  //     m->nextMessage = NULL;
  //     m->prevMessage = NULL;
  // }

  int slot = findOpenMessageSlot();

  if (slot == -1){return -1;}
  struct Message* m = messageSlots[slot];

  m->isInitialized = 1;
  m->message = msgPtr;
  m->messageSizeLimit = msgSize;
  m->nextMessage = NULL;
  m->prevMessage = NULL;
  
  if (mb->consumerQueueHead != NULL){ // theres exists a process waiting to consume sent message
    struct Process* curProc = mb->consumerQueueHead;
    curProc->message = (struct Message*)malloc(sizeof(struct Message));
    memcpy(curProc->message, m, sizeof(struct Message));
    // printMessage(curProc);
    mb->consumerQueueHead = curProc -> nextConsumerNode;
    
  } 
  else if (mb->mailSlotsAvailable > 0){ // if mail slots is not full, add it to the buffer
    mb->mailSlotsAvailable--;

    if (mb->mailSlotQueueHead == NULL){
      mb->mailSlotQueueHead = m;
    }
    //struct Message* curMes = mb->mailSlotQueueHead;
    //while (curMes)
    
  }
  else if (mb->mailSlotsAvailable <= 0){ // if mail slots is full, add cur proc to the end of the producer queue and block cur proc
    return -2;
  }
  


  




  return 0;
}

// Receive message from mailbox  
int MboxRecv(int mboxId, void *msgPtr, int msgMaxSize) {
  // Once message is received, remove it from message queue and messageslot array
  if (getMb(mboxId)->mailSlotQueueHead!= NULL){
    //USLOSS_Console("1\n");
    struct MB * curMB = getMb(mboxId);
    struct Process* curProc = getProcess(getpid());
    struct Message* mesRec = curMB->mailSlotQueueHead;
    curProc->message = (struct Message*)malloc(sizeof(struct Message));
    memcpy(curProc->message, mesRec, sizeof(struct Message));

    strcpy(curProc -> message, msgPtr);
    curMB->mailSlotsAvailable++;
    curMB->mailSlotQueueHead = curMB->mailSlotQueueHead->nextMessage;
    // // USLOSS_Console("%s\n", (char*)mesRec->message);
    // // USLOSS_Console("MESSAGE: %s\n", (char*)mesRec->message);
    strcpy(msgPtr, mesRec->message);
    // USLOSS_Console("Print ptr: %s\n", (char*)msgPtr);
    return mesRec->messageSizeLimit;
  }
  else {
    //USLOSS_Console("process is at end of consumer queue\n");
    // getMb(mboxId)->consumerQueueHead = getProcess(getpid());
    //dumpProcesses1();
  }

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

void printMB(int mbid){
  USLOSS_Console("PRINTING MAILBOX -----------: \n");

  struct MB* mb = getMb(mbid);

  USLOSS_Console("Consumer Queue: ");
  struct Process* curConsumer = mb -> consumerQueueHead;
  while (curConsumer != NULL){
    USLOSS_Console("PID: %d -> ", curConsumer->PID);
    curConsumer = curConsumer -> nextConsumerNode;
  }
  USLOSS_Console("NULL\n");

  USLOSS_Console("Producer Queue: ");
  struct Process* curProducer = mb -> producerQueueHead;
  while (curProducer != NULL){
    USLOSS_Console("PID: %d -> ", curProducer->PID);
    curProducer = curProducer -> nextProducerNode;
  }
  USLOSS_Console("NULL\n");

  USLOSS_Console("Mail Slot Queue (%d): ", mb->mailSlotLimit);
  struct Message* curMsg = mb -> mailSlotQueueHead;
  while (curMsg != NULL){
    USLOSS_Console("MESSAGE: %s -> ", (char*)curMsg->message);
    curMsg = curMsg -> nextMessage;
  }
  USLOSS_Console("NULL\n");

  USLOSS_Console("END OF PRINTING MAILBOX -----------: \n");

}