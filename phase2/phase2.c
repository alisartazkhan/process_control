/**
 * Author: Samuel Richardson and Ali Sartaz Khan
 * Course: CSC 452
 * Phase: 2
 * Description: This program uses phase1 library and USLOSS to create a mailbox
 * that is used to send and receive messages from one process to another. We also make 
 * sure we have clock handlers that work with different use cases. Since we don't have 
 * direct access to phase 1, we also used a shadow process table that emulates the proc 
 * table in phase1. Using that table, we created processes that are used in the Mailboxes.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>

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

/*
  Struct represents a single message that is stored in processes and the mailboxes.
*/
struct Message {
  int id;
  int isInitialized;
  int messageSizeLimit;
  void* message;
  struct Message* nextMessage;
  struct Message* prevMessage;
};

/*
Struct represents a mailbox that includes fields that keeps track of the producer, 
consumer, and mailslot queue.
*/
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

void phase2_clockHandler();
int phase2_check_io();
struct MB * getMb(int);
void termHandler(int, void*);
void syscallHandler(int, void*);
void addToConsumerQueue(struct MB *);
void addToProducerQueue(struct MB *);
void addToMailQueue(struct MB *, struct Message *);
void nullsys(int);

// index to insert into mbTable. Increments after every mb insertion
int messageIDCounter =0;
int pidCounter = 1;
static int lastMessage = 0; 


// arrays
static struct Process shadowProcessTable[MAXPROC];
static struct MB mbTable[MAXMBOX];
static struct Message* messageSlots[MAXSLOTS];
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs*);


/*
  This method is called by the code in phase1 before timeslicing that sends a message
  to the clockhandler mailbox. The time it takes for it to send is 100ms.
*/
void phase2_clockHandler() {
  int now = currentTime();

  if (now - lastMessage >= 100000) {
    //USLOSS_Console("now:    %d\n",now);
    MboxSend(0, now, sizeof(int)); // send to clock mailbox
    lastMessage = now; 
  }

}


/*
  This method checks to see if there are any blocked processes available in the mailboxes.
*/
int phase2_check_io(){
  for (int i = 0; i < 7; i++){
    struct MB * mb = getMb(i);
    if (mb->consumerQueueHead != NULL || getMb(i)->producerQueueHead != NULL){
      return 1;
    }
  }
  return 0;
}


/*
  This method initializes mailboxes, messages, initializes interrupt handlers, and 
  populates syscall vector.
*/
void phase2_init() {
  int mbNumber = 0;
  // initializes mailboxes for interrupts
  for (int i = 0; i< 7; i++){
        if (i < 7){
          MboxCreate(1,100);
        }
        else{
              MboxCreate(0,0);
        }
  }
  // initializes messages
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
  // USLOSS_IntVec[USLOSS_DISK_INT] = diskHandler;
  USLOSS_IntVec[USLOSS_TERM_INT] = termHandler;
  USLOSS_IntVec[USLOSS_SYSCALL_INT] = syscallHandler;

  // populate syscall array
  for (int i = 0; i < MAXSYSCALLS; i++){
      systemCallVec[i] = nullsys;
  }
}

/*
  This is a syscall handler that takes in two arguments and once this is called, it outputs
  an error message and stops the program.

  Args:
    blank: int argument that isn't used
    unit: void* argument that is cast to a USLOSS_Sysargs*

  Return: void
*/
void syscallHandler(int blank, void *unit){
  USLOSS_Sysargs* arg = (USLOSS_Sysargs*) unit;

  if(arg->number < 0 || arg->number >= MAXSYSCALLS) {
    USLOSS_Console("syscallHandler(): Invalid syscall number %d\n", arg->number);
    USLOSS_Halt(1);
  }
  nullsys(arg->number);
}


/*
  This is a term handler that takes in two arguments and once this is called, it waits 
  for device input using USLOSS and then sends a message to the mailbox of that associated
  unit.

  Args:
    blank: int argument that isn't used
    unit: void* argument that is cast to a USLOSS_Sysargs*

  Return: void
*/
void termHandler(int blank, void *unit){
  long newUnit = (long) unit;
  int status = 0;
  USLOSS_DeviceInput(USLOSS_TERM_DEV,newUnit,&status);
  MboxSend(unit+3, status, sizeof(int)); // send to clock mailbox
}


/*
  This method starts the process in phase2. This is blank because it isn't necessary for our implementation.
*/
void phase2_start_service_processes(){}


/* 
  Description: getter function that return process
   
  Arg: pid of process to retrieve
   
  return: Process that you are looking for
*/
struct Process* getProcess(int pid) {
  if (pid != shadowProcessTable[pid%MAXPROC].PID){createShadowProcess(pid); }  
  return &shadowProcessTable[pid%MAXPROC];
}


/*
  Getter function to get a mailbox from the mb table 
  
  Args:
    mbId: id of the mailbox

  Return: returns a pointer to the MB struct
*/
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


/*
  Description: iterates through the messageSlots to find an open slot
  for the created process using messageIDCounter % MAXSLOTS.

  Arg: none

  return int: the found slot for the process or -1 if table is full
*/
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



/*
  Method creates a new mailbox and inserts a pointer to that MB inside the 
  mailbox table.

  Args:
    numslots: int value that represents the total number of slots in a MB
    slotSize: int the total size of each mail slot
  
  Returns: the Mailbox ID. -1 if args are invalid or there are no more available slots for
            mailbox.
*/
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

  mbTable[slot] = m;

  return slot;
}

/*
  Iterates through the different queues in the Mailbox and unblocks each process in the
  queue and also frees every message in the mailslot queue.

  Args:
    mboxId: int id of the mailbox

  Returns: returns 0 once MB is released.
*/
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


/*
  Method is called by a producer process that attempts to send a message to the mailbox.

  Args:
    mboxId: int value of the ID of the mailbox
    msgPtr: void* containing the message that is being sent
    msgSize: int value of the size of the message
  
  Returns: status of the send process or size of the message
*/
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
  

/*
  Method adds process to the end of the producer queue.

  Args:
    mb: struct MB pointer that is being added

  Returns: void
*/
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


/*
  Method adds process to the end of the consumer queue.

  Args:
    mb: struct MB pointer that is being added

  Returns: void
*/
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


/*
  Method adds process to the end of the mail queue.

  Args:
    mb: struct MB pointer that is being added
    mes: struct Message* pointer of the message that is being added

  Returns: void
*/
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


/*
  Method is called by a consumer process that attempts to receive a message from the mailbox.

  Args:
    mboxId: int value of the ID of the mailbox
    msgPtr: void* containing the message will be received
    msgSize: int value of the size of the message
  
  Returns: status of the receive process or size of the message
*/
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


/*
  Method is called by a producer process that attempts to receive a message from the mailbox.
  If there is a scenario where you cant send the message immediately, it doesn't block the
  process.

  Args:
    mboxId: int value of the ID of the mailbox
    msg_ptr: void* containing the message will be received
    msg_size: int value of the size of the message
  
  Returns: status of the send process or size of the message
*/
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size) {
  struct MB* mb = getMb(mbox_id);
  if (msg_size != 0 && msg_ptr == NULL){
    return -1;
  }

  if (msg_size > mb->messageSizeLimit){
    // ERROR MESSAGE
    //USLOSS_Console("ERROR: Message Size it too big for mb\n");
    return -1;
  }

  if (mb->isInitialized == 0){
    return -1;
  }
  int slot = findOpenMessageSlot();
    if (slot == -1) {
        return -2;
    }

  if (mb->mailSlotsAvailable<=0){
    return -2;
    if (mb->isInitialized==0){return -3;}
    }
    
  struct Message* m = messageSlots[slot];

  m->isInitialized = 1;
  m->message = (char*)malloc(msg_size);  // Allocate memory and copy message
  if (m->message == NULL) {return -1;}
  memcpy(m->message, msg_ptr, msg_size);  // Copy message contents
  m->messageSizeLimit = msg_size;
  m->nextMessage = NULL;
  m->prevMessage = NULL;

  // ADD TO MAIL QUEUE
  if (mb->consumerQueueHead != NULL){
    unblockProc(mb->consumerQueueHead->PID);
  }
  addToMailQueue(mb,m);
  return 0;
} 


/*
  Method is called by a consumer process that attempts to receive a message from the mailbox.
  If there is a scenario where you cant receive the message immediately, it doesn't block the
  process.

  Args:
    mboxId: int value of the ID of the mailbox
    msg_ptr: void* containing the message will be received
    msg_size: int value of the size of the message
  
  Returns: status of the send process or size of the message
*/ 
int MboxCondRecv(int mbox_id, void *msg_ptr, int msg_max_size) {
  struct MB * mb = getMb(mbox_id);
  struct Process* curProc = getProcess(getpid());
  if (mb->isInitialized == 0){return -1;}

  if (mb->mailSlotQueueHead == NULL){
    return -2;
    if (mb->isInitialized==0){return -3;}
  }
  addToConsumerQueue(mb);

  struct Message* mesRec = mb->mailSlotQueueHead;

  curProc->message = (struct Message*)malloc(sizeof(struct Message));
  memcpy(curProc->message, mesRec, sizeof(struct Message));
  memcpy(msg_ptr, curProc->message->message,mesRec->messageSizeLimit);
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



/*
  Method called MBoxRecv() on the right mailbox depending on the
  unit and device type. The message that it received from the function
  call it then assigns it to the status pointer in the argument.

  Args:
    type: int value of the device type
    unit: unit of the device
    status: int* of the status value 
  
  Returns: void
*/
void waitDevice(int type, int unit, int *status) {
     int mboxId = -1;
     if (type == USLOSS_CLOCK_DEV){ // 0
        if (unit != 0){
          USLOSS_Console("wront unit\n");
          return;
        }
        mboxId = 0;
     }
     if (type == USLOSS_DISK_DEV){ // 2
      if (unit != 1 && unit != 0){
            USLOSS_Console("wront unit\n");
            return;
          }
      mboxId = unit+1;
     }
     if (type == USLOSS_TERM_DEV){ // 3
      if (unit < 0 || unit > 3){
            USLOSS_Console("wront unit\n");
            return;
          }
      mboxId = unit+3;
     }
 
    int message;
    MboxRecv(mboxId,&message,sizeof(int));

    *status = message;
}


/*
  Method that outputs an error message and halts the program.

  Args:
    num: int value representing the syscall number
  
  Returns: void
*/
void nullsys(int num) {
  
  USLOSS_Console("nullsys(): Program called an unimplemented syscall. syscall no: %d   PSR: 0x%02x \n",num,USLOSS_PsrGet());
  USLOSS_Halt(1);
}


/*
  Method enables interrupts using the psr.
  
  Args: NONE
  Returns: void
*/
void enableInterrupts() {

  unsigned int psr = USLOSS_PsrGet();
  psr |= USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);

}

/*
  Method disables interrupts using the psr.
  
  Args: NONE
  Returns: void
*/
void disableInterrupts() {
  unsigned int psr = USLOSS_PsrGet();
  psr &= ~USLOSS_PSR_CURRENT_INT;
  USLOSS_PsrSet(psr);
}