#include <stdio.h>
#include <stdlib.h>
#include "usloss.h"
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>

#define MAX_SLOTS 2500
#define MAX_MESSAGE 150

typedef struct {
  int inUse;
  int numSlots;
  int slotSize;
  int numMessages;
} Mbox;

typedef struct {
  int inUse;
  void *msgPtr;
  int msgSize;  
} Slot;

static int numMboxes;
static Mbox mboxes[MAXMBOX];

static int numSlots;
static Slot slots[MAX_SLOTS];

static PCB shadowProcessTable[MAXPROC]; 

void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs*);

// Initialize mailboxes and other data structures
void phase2_init() {

  // Initialize mailboxes
  numMboxes = 0;
  for (int i = 0; i < MAXMBOX; i++) {
    mboxes[i].inUse = 0;
  }

  // Initialize mail slots
  numSlots = 0; 
  for (int i = 0; i < MAX_SLOTS; i++) {
    slots[i].inUse = 0;
  }

  // Initialize system call handler vector
  for (int i = 0; i < MAXSYSCALLS; i++) {
    systemCallVec[i] = nullsys;
  }

}

// Create a new mailbox
int MboxCreate(int slots, int slotSize) {

  // Check for invalid arguments
  if (slots < 0 || slots > MAXMBOX ||  
      slotSize < 0 || slotSize > MAX_MESSAGE) {
    return -1;
  }

  // Check if any mailboxes left
  if (numMboxes == MAXMBOX) {
    return -1;
  }

  // Find empty mailbox
  int mboxId = -1;
  for (int i = 0; i < MAXMBOX; i++) {
    if (!mboxs[i].inUse) {
      mboxId = i;
      break; 
    }
  }

  // Initialize mailbox
  mboxes[mboxId].inUse = 1;
  mboxes[mboxId].numSlots = slots;
  mboxes[mboxId].slotSize = slotSize;
  mboxes[mboxId].numMessages = 0;

  // Increment mailbox count
  numMboxes++;

  return mboxId;
}

// Release a mailbox
int MboxRelease(int mboxId) {

  // Check for invalid mailbox
  if (mboxId < 0 || mboxId >= MAXMBOX || !mboxs[mboxId].inUse) {
    return -1;
  }

  // Mark mailbox as free
  mboxes[mboxId].inUse = 0;

  // Decrement mailbox count
  numMboxes--;

  return 0;
}

// Send message to mailbox
int MboxSend(int mboxId, void *msgPtr, int msgSize) {
  
  // Check for invalid args
  if (mboxId < 0 || mboxId >= MAXMBOX ||  
      !mboxs[mboxId].inUse ||
      msgSize < 0 || msgSize > mboxes[mboxId].slotSize) {
    return -1;
  }

  // Check if mailbox full
  if (mboxs[mboxId].numMessages == mboxes[mboxId].numSlots) {
    return -2; // mail box full
  }

  // Enqueue message in slot
  int slotId = enqueueSlot(msgPtr, msgSize);

  // Increment message count
  mboxes[mboxId].numMessages++;

  return 0;
}

// Receive message from mailbox  
int MboxRecv(int mboxId, void *msgPtr, int msgMaxSize) {

  // Check for invalid args
  if (mboxId < 0 || mboxId >= MAXMBOX ||
      !mboxs[mboxId].inUse ||  
      msgMaxSize < 0 || msgMaxSize > mboxes[mboxId].slotSize) {
    return -1;
  }

  // Check if mailbox empty
  if (mboxs[mboxId].numMessages == 0) {
    return -2; // mailbox empty
  }

  // Dequeue message from slot
  int msgSize = dequeueSlot(msgPtr, msgMaxSize);

  // Decrement message count
  mboxes[mboxId].numMessages--;

  return msgSize;
}

// Conditional send
int MboxCondSend(...) {
  // Implementation similar to MboxSend but 
  // returns -2 instead of blocking if full
} 

// Conditional receive  
int MboxCondRecv(...) {
  // Implementation similar to MboxRecv but
  // returns -2 instead of blocking if empty
}

// Wait for interrupt
void waitDevice(int type, int unit, int *status) {

  // Check for invalid args

  // Block calling process

  // When interrupt occurs, copy status and unblock
}

// Interrupt handlers 

// System call handler
void nullsys(...) {
  // Print error and halt 
}