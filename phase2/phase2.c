

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

struct MB * getMb(int);

void termHandler(int, void*);
void syscallHandler(int, void*);


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
    int blocked;
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
  char* name;
  int isInitialized;
  int id;
  int mailSlotLimit;
  int messageSizeLimit;
  int mailSlotsAvailable;
  struct Message* mailSlotQueueHead;
  struct Process* consumerQueueHead;
  struct Process* producerQueueHead;
  int released;
};


void addToConsumerQueue(struct MB *);
void addToProducerQueue(struct MB *);
void addToMailQueue(struct MB *, struct Message *);
void nullsys(int);


// index to insert into mbTable. Increments after every mb insertion
int messageIDCounter =0;
int pidCounter = 1;


// arrays
static struct Process shadowProcessTable[MAXPROC];
static struct MB mbTable[MAXMBOX];
static struct Message* messageSlots[MAXSLOTS];
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs*);

int countSlotsAvailable();


int countSlotsAvailable(){
  int count = 0;
  for (int i=0; i < MAXSLOTS; i++){
    if (messageSlots[i]->isInitialized != 0){
      count++;
    }
  }
  return count;
}

static int lastMessage = 0; 

void phase2_clockHandler() {


  int now = currentTime();

  if (now - lastMessage >= 100000) {
    //USLOSS_Console("now:    %d\n",now);
    MboxSend(0, now, sizeof(int)); // send to clock mailbox

    lastMessage = now; 
  }

}



int phase2_check_io(){
  for (int i = 0; i < 7; i++){
    struct MB * mb = getMb(i);
    if (mb->consumerQueueHead != NULL || getMb(i)->producerQueueHead != NULL){
      return 1;
    }
  
  }
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
        if (i < 7){
          MboxCreate(1,100);
          //struct MB * mb = getMb(i);
          

        }
        else{
              MboxCreate(0,0);
        }

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
  USLOSS_IntVec[USLOSS_CLOCK_INT] = phase2_clockHandler; 

  //USLOSS_IntVec[USLOSS_DISK_INT] = diskHandler;
  USLOSS_IntVec[USLOSS_TERM_INT] = termHandler;
  USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;
  for (int i = 0; i < MAXSYSCALLS; i++){
      systemCallVec[i] = nullsys;
  }


  
}
void syscallHandler(int blank, void *unit){

  
  USLOSS_Sysargs* arg = (USLOSS_Sysargs*) unit;

  if(arg->number < 0 || arg->number >= MAXSYSCALLS) {
    USLOSS_Console("syscallHandler(): Invalid syscall number %d\n", arg->number);
    USLOSS_Halt(1);
  }

  nullsys(arg->number);


}
void termHandler(int blank, void *unit){

  long newUnit = (long) unit;
  // USLOSS_Console("unit %d newUnit %d\n",unit,newUnit);


  int status = 0;
  //USLOSS_Console("retunr status: %d\n",USLOSS_DeviceInput(USLOSS_TERM_DEV,newUnit,&status));
  //USLOSS_Console("ok %d",USLOSS_DEV_OK);

  USLOSS_DeviceInput(USLOSS_TERM_DEV,newUnit,&status);


  // int mboxId = newUnit+3;
  // long *msgPtr = &status;
  // int msgSize = sizeof(long);


  //MboxSend(mboxId, msgPtr, msgSize); // send to clock mailbox



  MboxSend(unit+3, status, sizeof(int)); // send to clock mailbox
    // making sizeof(long) seems to help 
    //1105579920
    //1229995308
    //1457782060



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
        int slot = i;
        i++;
        if (mbTable[slot].isInitialized == 0){
            // USLOSS_Console("%d\n",slot);
            return slot;}
    }

    return -1;
}

//see why dounle -- seg faults
//check if mail slot is added before ti shoudl be

int findOpenMessageSlot(){
    
    int i = 0;
    while (i<MAXSLOTS){

        int slot = messageIDCounter % MAXSLOTS;
        messageIDCounter++;

        if (messageSlots[slot]->isInitialized == 0){
              messageIDCounter--;
            return slot;}
        i++;
    }
    return -1;
}


void printMbTable(){
  USLOSS_Console("PRINTING MailBox table: \n");

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
  if (numSlots < 0 || slotSize < 0 ){return -1;}
  if (numSlots > MAXSLOTS || slotSize < 0 ){return -1;}

int slot = findOpenMbTableSlot();
  if (slot == -1) {return -1;}

  struct MB m = {
    .isInitialized = 1,
    .id = slot,
    .mailSlotLimit = numSlots,
    .messageSizeLimit = slotSize,
    .mailSlotsAvailable = numSlots,
    .mailSlotQueueHead = NULL,
    .producerQueueHead = NULL,
    .consumerQueueHead = NULL,
    .released = 0
  };

  // USLOSS_Console("%d\n", sizeof(mbTable)/ sizeof(mbTable[0]));
  

  mbTable[slot] = m;

  return slot;
}

// Release a mailbox
int MboxRelease(int mboxId) {
  struct MB * mb = getMb(mboxId);
  mb->isInitialized = 0;

  mb->released = 1;

  while (mb->mailSlotQueueHead != NULL){
    mb->mailSlotQueueHead->isInitialized = 0;
    mb->mailSlotQueueHead = mb->mailSlotQueueHead -> nextMessage;
  }

 

   while (mb->consumerQueueHead != NULL){
    unblockProc(mb->consumerQueueHead->PID);
    mb->consumerQueueHead = mb->consumerQueueHead -> nextConsumerNode;
  }

  
   while (mb->producerQueueHead != NULL){
    unblockProc(mb->producerQueueHead->PID);
    mb->producerQueueHead = mb->producerQueueHead -> nextProducerNode;
  }

  return 0;
}


// Send message to mailbox
int MboxSend(int mboxId, void *msgPtr, int msgSize) {

  struct MB* mb = getMb(mboxId);
  if (mb->released == 1){
    return -1;
  }

  if (mb->isInitialized==0){return -3;}

  if (msgSize != 0 && msgPtr == NULL){
    return -1;
  }

  if (msgSize > mb->messageSizeLimit){
    return -1;
  }

  int slot = findOpenMessageSlot();
  if (slot == -1) {
    return -2;
  }

  

  struct Message* m = messageSlots[slot];

  m->isInitialized = 1;
  m->message = malloc(msgSize);  // Allocate memory and copy message
  if (m->message == NULL) {
    // Handle memory allocation failure
    return -1;
  }

  if (mb->id < 7){
    memcpy(m->message, &msgPtr, msgSize);  // Copy message contents
  }else{
    memcpy(m->message, msgPtr, msgSize);  // Copy message contents
  }

  m->messageSizeLimit = msgSize;
  m->nextMessage = NULL;
  m->prevMessage = NULL;

  if (mb->mailSlotLimit == 0){ // CHECKING FOR ZERO SLOT MB
    addToProducerQueue(mb);

    struct Process* producer = getProcess(getpid());
    if (mb->consumerQueueHead == NULL){

      blockMe(8000);
            if (mb->isInitialized==0){return -3;}

    }


    mb->producerQueueHead = mb->producerQueueHead->nextProducerNode;
    producer->message = NULL;
    unblockProc(mb->consumerQueueHead->PID);
    return 0;
  }

    // ADD TO PRODUCER QUEUE

  if (mb->mailSlotsAvailable<=0){
    addToProducerQueue(mb);

    blockMe(8000);

    if (mb->isInitialized==0){return -3;}
  }
  // ADD TO MAIL QUEUE
  addToMailQueue(mb,m);
  // printMB(7);
  // dumpProcesses();
  if (mb->consumerQueueHead != NULL){
    
    unblockProc(mb->consumerQueueHead->PID);
  }

  return 0;
}
  


void addToProducerQueue(struct MB * mb){
  if (mb->producerQueueHead == NULL){
      mb->producerQueueHead = getProcess(getpid());
    }
    else {
      struct Process * curProdProc = mb->producerQueueHead;
      while (curProdProc -> nextProducerNode != NULL){ // there is a blocked consumer so get to the end of the consumer queue
        curProdProc = curProdProc -> nextProducerNode;
      }
        curProdProc -> nextProducerNode = getProcess(getpid());
    }
}

void addToConsumerQueue(struct MB * mb){

  if (mb->consumerQueueHead == NULL){
      mb->consumerQueueHead = getProcess(getpid());
    }
    else {
      struct Process * curProdProc = mb->consumerQueueHead;
      while (curProdProc -> nextConsumerNode != NULL){ // there is a blocked consumer so get to the end of the consumer queue
        curProdProc = curProdProc -> nextConsumerNode;
      }
        curProdProc -> nextConsumerNode = getProcess(getpid());
    }
}

void addToMailQueue(struct MB * mb, struct Message * mes){
  mb->mailSlotsAvailable--;
  if (mb->mailSlotQueueHead == NULL){

      mb->mailSlotQueueHead = mes;
    }
    else {
      struct Message * curMessage = mb->mailSlotQueueHead;
      while (curMessage -> nextMessage != NULL){ // there is a blocked consumer so get to the end of the consumer queue

        curMessage = curMessage -> nextMessage;
      }
        curMessage -> nextMessage = mes;

    }
    mes->nextMessage=NULL;

}


// Receive message from mailbox  
int MboxRecv(int mboxId, void *msgPtr, int msgMaxSize) {

struct MB * mb = getMb(mboxId);

if (mb->released == 1){
    return -1;
  }
  if (mb->isInitialized==0){return -3;}

  struct Process* curProc = getProcess(getpid());
  addToConsumerQueue(mb);

  if (mb->mailSlotLimit == 0){ // CHECKING FOR ZERO SLOT MB
    if (mb->producerQueueHead == NULL){
      blockMe(8000);
      if (mb->isInitialized==0){return -3;}
    }

    mb -> consumerQueueHead = mb->consumerQueueHead->nextConsumerNode;
    curProc->message = NULL;
    return 0;
  }

  if (mb->mailSlotQueueHead == NULL){
    blockMe(8000);
    if (mb->isInitialized==0){return -3;}
  }

  struct Message* mesRec = mb->mailSlotQueueHead;
  if(mesRec->messageSizeLimit > msgMaxSize){
    return -1;
  }

  curProc->message = (struct Message*)malloc(sizeof(struct Message));
  memcpy(curProc->message, mesRec, sizeof(struct Message));
  memcpy(msgPtr, curProc->message->message,mesRec->messageSizeLimit);
  mesRec->isInitialized = 0;

  mb->consumerQueueHead = mb->consumerQueueHead->nextConsumerNode;
  mb->mailSlotQueueHead = mb->mailSlotQueueHead->nextMessage;
  mb->mailSlotsAvailable++;

  if (mb->producerQueueHead!=NULL){
      struct Process* aboutToUB = mb->producerQueueHead;
      mb->producerQueueHead = mb->producerQueueHead->nextProducerNode;
      unblockProc(aboutToUB->PID);

  }

  return mesRec -> messageSizeLimit;
}

// Conditional send
int MboxCondSend(int mboxId, void *msgPtr, int msgSize) {

  struct MB* mb = getMb(mboxId);
  if (mb->released == 1){
    return -1;
  }

  if (mb->isInitialized==0){return -3;}

  if (msgSize != 0 && msgPtr == NULL){
    return -1;
  }

  if (msgSize > mb->messageSizeLimit){
    return -1;
  }



  int slot = findOpenMessageSlot();
  if (slot == -1) {
    return -2;
  }
if (mb->mailSlotsAvailable<=0){
      //USLOSS_Console("returned -2");
      
      return -2;

      if (mb->isInitialized==0){


        return -3;
      }
    }
  struct Message* m = messageSlots[slot];

  m->isInitialized = 1;
  m->message = malloc(msgSize);  // Allocate memory and copy message
  if (m->message == NULL) {
    // Handle memory allocation failure
    return -1;
  }

  if (mb->id < 7){
    memcpy(m->message, &msgPtr, msgSize);  // Copy message contents
  }else{
    memcpy(m->message, msgPtr, msgSize);  // Copy message contents
  }

  m->messageSizeLimit = msgSize;
  m->nextMessage = NULL;
  m->prevMessage = NULL;

  if (mb->mailSlotLimit == 0){ // CHECKING FOR ZERO SLOT MB
    addToProducerQueue(mb);

    struct Process* producer = getProcess(getpid());
    if (mb->consumerQueueHead == NULL){

      //blockMe(8000);
      return -2;
            if (mb->isInitialized==0){return -3;}

    }


    mb->producerQueueHead = mb->producerQueueHead->nextProducerNode;
    producer->message = NULL;
    unblockProc(mb->consumerQueueHead->PID);
    return 0;
  }

    // ADD TO PRODUCER QUEUE

  if (mb->mailSlotsAvailable<=0){
    addToProducerQueue(mb);

    //blockMe(8000);
    return -2;

    if (mb->isInitialized==0){return -3;}
  }
  // ADD TO MAIL QUEUE
  // printMB(7);
  // dumpProcesses();
  if (mb->consumerQueueHead != NULL){
    
    unblockProc(mb->consumerQueueHead->PID);
  }
    addToMailQueue(mb,m);


  return 0;

} 

// Conditional receive  
int MboxCondRecv(int mboxId, void *msgPtr, int msgMaxSize) {
  
struct MB * mb = getMb(mboxId);

if (mb->released == 1){
    return -1;
  }
  if (mb->isInitialized==0){return -3;}

  struct Process* curProc = getProcess(getpid());
  addToConsumerQueue(mb);

  if (mb->mailSlotLimit == 0){ // CHECKING FOR ZERO SLOT MB
    if (mb->producerQueueHead == NULL){
      //blockMe(8000);
      return -2;
      if (mb->isInitialized==0){return -3;}
    }

    mb -> consumerQueueHead = mb->consumerQueueHead->nextConsumerNode;
    curProc->message = NULL;
    return 0;
  }

  if (mb->mailSlotQueueHead == NULL){
    //blockMe(8000);
    return -2;
    if (mb->isInitialized==0){return -3;}
  }

  struct Message* mesRec = mb->mailSlotQueueHead;
  if(mesRec->messageSizeLimit > msgMaxSize){
    return -1;
  }

  curProc->message = (struct Message*)malloc(sizeof(struct Message));
  memcpy(curProc->message, mesRec, sizeof(struct Message));
  memcpy(msgPtr, curProc->message->message,mesRec->messageSizeLimit);
  mesRec->isInitialized = 0;

  mb->consumerQueueHead = mb->consumerQueueHead->nextConsumerNode;
  mb->mailSlotQueueHead = mb->mailSlotQueueHead->nextMessage;
  mb->mailSlotsAvailable++;

  if (mb->producerQueueHead!=NULL){
      struct Process* aboutToUB = mb->producerQueueHead;
      mb->producerQueueHead = mb->producerQueueHead->nextProducerNode;
      unblockProc(aboutToUB->PID);

  }

  return mesRec -> messageSizeLimit;
}



// Wait for interrupt
void waitDevice(int type, int unit, int *status) {
     int mboxId = -1;


     if (type == USLOSS_CLOCK_DEV){ // 0
        if (unit != 0){
          USLOSS_Console("wront unit\n");
          return;
        }
        mboxId = 0;

      //unit =0, mailbox0

     }
     if (type == USLOSS_DISK_DEV){ // 2
      if (unit != 1 && unit != 0){
            USLOSS_Console("wront unit\n");
            return;
          }

      mboxId = unit+1;
      //unit = 0, mailbox1
      //unit = 1, mailbox2

     }
     if (type == USLOSS_TERM_DEV){ // 3
      if (unit < 0 || unit > 3){
            USLOSS_Console("wront unit\n");
            return;
          }
      mboxId = unit+3;
      //unit = 0, mailbox3
      //unit = 1, mailbox4
      //unit = 2, mailbox5
      //unit = 3, mailbox6

     }


     
    int message;

    MboxRecv(mboxId,&message,sizeof(int));





    *status = message;
    //USLOSS_Console("message %d\n",message);

    // _validateArgs(type, unit, &mboxId);
    // debug("passed type %d, unit %d", type, unit);

    // assert(mboxId != -1);
    // int pid = getpid();
    // PQueue *thisproc = &shadowProcs[pid % MAXPROC];
    // thisproc->pid = pid;
    // thisproc->blockedInWaitDevice = 1;

    // int message;
    // int retVal = MboxRecv(mboxId, &message, sizeof(int));
    ///debug("mbox recv args: id %d message: %d", mboxId, message);
    //assert(retVal != -1);
    //thisproc->blockedInWaitDevice = 1;

    //*status = message;
}
  // Check for invalid args

  // Block calling process

  // When interrupt occurs, copy status and unblock

// Interrupt handlers 

// System call handler
void nullsys(int num) {
  
  USLOSS_Console("nullsys(): Program called an unimplemented syscall. syscall no: %d   PSR: 0x%02x \n",num,USLOSS_PsrGet());
  USLOSS_Halt(1);
}

void printMB(int mbid){
    struct MB* mb = getMb(mbid);

  USLOSS_Console("\nPRINTING MAILBOX: %d -----------: \n",mb->id);


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


  USLOSS_Console("Mail Slot Queue (%d/%d): ", mb->mailSlotsAvailable,mb->mailSlotLimit);
  struct Message* curMsg = mb -> mailSlotQueueHead;
  while (curMsg != NULL){
    USLOSS_Console("MESSAGE: %s -> ", (char*)curMsg->message);
    curMsg = curMsg -> nextMessage;
  }
  USLOSS_Console("NULL\n");

  USLOSS_Console("END OF PRINTING MAILBOX -----------: \n\n");

}




// method #1 we may switch to

//send
// always add to the end of producer queue
// if there is a mailbox slot/buffer space, add it to the space
// if not, block, 
// when unblocked, add to mailbox slot

//receive
// always add to end of rec queue
// if no mail, block, 
// when unblocked, check if mail slots are empty
// if mail slot not empy, recieve mail from from of mailslotqueue
// if mail slot empty, recieve mail from producer queue










/*The mailbox keeps track of two queues:

Consumer queue - contains PIDs of processes blocked in MboxRecv() waiting to receive messages
Mail slot queue - contains messages that have been sent but not yet delivered
When a message arrives:

Dequeue the first consumer from the consumer queue. This is the process that blocked first.
Remove the first message from the mail slot queue.
Deliver the message to the consumer by copying it into their buffer.
Mark the consumer as ready to run.
Before letting the consumer run, check if there are more messages and consumers. If so, queue up the next consumer to receive the next message.
Context switch to the consumer process to let it run.
The key thing is we extract consumers and messages in pairs, one at a time, matching the order they arrived.

We don't wake up all blocked processes at once, to avoid race conditions.

We prep the next consumer/message before context switching, to avoid races.

This ensures each process receives the correct message in the proper order.*/


// Enable interrupts
void enableInterrupts() {

  unsigned int psr = USLOSS_PsrGet();
  psr |= USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);
  USLOSS_Console("Interupts Enabled\n");

}

// Disable interrupts 
void disableInterrupts() {

  unsigned int psr = USLOSS_PsrGet();
  psr &= ~USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);
  USLOSS_Console("Interupts Disabled\n");

}