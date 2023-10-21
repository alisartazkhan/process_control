#include <stdio.h>
#include <stdlib.h>
#include <usloss.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <usyscall.h>
typedef void (*FunctionPointer)();

int pidCounter = 5;
int semCounter = 0;

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
    int (*startFunc)(char*);
    char* arg;
};

// struct Semaphore {
//     int mboxID;
//     int slotsLeft;
// };

// arrays
static struct Process shadowProcessTable[MAXPROC];
static int semaphoreTable[MAXSEMS];

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
int createShadowProcess(int (*func)(char*), void *arg) {
    struct Process p = {
      .PID = pidCounter, 
      .startFunc = func,
      .arg = arg
    };


    shadowProcessTable[pidCounter%MAXPROC] = p;
    pidCounter ++;
    return pidCounter-1;
}

/*
    Description: prints information about the processTable in the same formatting
    given by the test cases.

    Arg: none

    return: none
*/
void dumpSP(void){
    USLOSS_Console("\n DUMPING PROC\n");
    for (int i = 0; i < MAXPROC; i++){
         if ( shadowProcessTable[i].PID == 0){
             continue;
         }
        USLOSS_Console("PID: %d \n",shadowProcessTable[i].PID);

    }
    USLOSS_Console(" END OF DUMP PROC\n\n");

}

/* 
  Description: getter function that return process
   
  Arg: pid of process to retrieve
   
  return: Process that you are looking for
*/
struct Process* getProcess(int pid) {
  //if (pid != shadowProcessTable[pid%MAXPROC].PID){createShadowProcess(pid); }  
  return &shadowProcessTable[pid%MAXPROC];
}



void printCurrentMode() {

  int psr = USLOSS_PsrGet();

  if (psr == USLOSS_PSR_CURRENT_INT) {
    USLOSS_Console("Currently in USER mode\n");
  }
  else {
    USLOSS_Console("Currently in KERNEL mode\n"); 
  }

}

void switchToUserMode(){

    // Get current PSR
int psr = USLOSS_PsrGet();  

// Set user mode bit
psr = USLOSS_PSR_CURRENT_INT;



// Update PSR
USLOSS_PsrSet(psr);
}

void tramp(){
    switchToUserMode();
    struct Process* curProcess = getProcess(getpid()); 
        // USLOSS_Console("HERE1 PID: %d  C: %d\n", getpid(), pidCounter);

    // dumpSP();
    int status = curProcess->startFunc(curProcess->arg);
        // USLOSS_Console("HERE2\n");

    Terminate(status);

}

void phase3_start_service_processes(){
    return;
}

int kernSpawn(void *args){
    int (*func)(char*);
    USLOSS_Sysargs *sysargs = args; // Cast to pointer to USLOSS_Sysargs
    char* name = (char*) sysargs->arg5;
    func=  sysargs->arg1;
    char* arg = (char*) sysargs->arg2;
    int stack_size = (int)(long)sysargs->arg3; // Dereference pointer to access arg3
    int priority = (int)(long)sysargs->arg4; // Dereference pointer to access arg3
        

    createShadowProcess(func,arg);
    
    sysargs->arg1 = (void*)(long) fork1(name, tramp, arg, stack_size, priority);

    return 0;
}

int kernWait(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int status;

    int retVal = join(&status);
    if (retVal == -2){
        sysargs->arg4 = retVal;
        return;
    }

    sysargs->arg1 = retVal;
    sysargs->arg2 = status;
    sysargs->arg4 = 0;


    return 0;
}

int kernTerminate(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int s1 = sysargs->arg1;
    int status = sysargs->arg1;
    //USLOSS_Console("status: %d\n",status);

    while (join(&status) != -2){}
    //USLOSS_Console("status: %d\n",status);
    sysargs->arg1 = status;
    quit(s1);
    return 0;
}

int semCreate(int id){
    for (int i = 0; i < MAXSEMS; i++){
        if (semaphoreTable[i] == 0){
            semaphoreTable[i] = id;
            semCounter++;
            return;
        }
    }
    return -1;
}

int kernSemCreate(void* args) {
    USLOSS_Sysargs *sysargs = args; 
    int value = sysargs->arg1;
    sysargs->arg4 = 0;
    // Allocate new mailbox to use as semaphore
    int mailboxID = MboxCreate(value, 0);
    if (semCreate(mailboxID) == -1 || value < 0){
        sysargs->arg4 = -1;
        sysargs->arg1 =0;
        return -1;
    }
    sysargs->arg1 = semCounter-1;
    return 0;
}

int kernSemP(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int id = sysargs->arg1;
    sysargs->arg4 = 0;
    if (MboxRecv(id, NULL, NULL) == -1){
        sysargs->arg4 = -1;
    }

    return 0;
}

int kernSemV(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int id = sysargs->arg1;
    sysargs->arg4 = 0;
    if (MboxSend(id, NULL, NULL) == -1){
        sysargs->arg4 = -1;
    }
    return 0;
}

int kernGetTimeofDay(void *args){
    return 0;
}

int kernCPUTime(void *args){
    return 0;
}

int kernGetPID(void *args){
    USLOSS_Sysargs *sysargs = args; 
    sysargs->arg1 = getpid();
}

void phase3_init(){
    systemCallVec[SYS_SPAWN] = kernSpawn;
    systemCallVec[SYS_WAIT] = kernWait;
    systemCallVec[SYS_TERMINATE] = kernTerminate;
    systemCallVec[SYS_SEMCREATE] = kernSemCreate;
    systemCallVec[SYS_SEMP] = kernSemP;
    systemCallVec[SYS_SEMV] = kernSemV;
    systemCallVec[SYS_GETTIMEOFDAY] = kernGetTimeofDay;
    systemCallVec[SYS_GETPROCINFO] = kernCPUTime;
    systemCallVec[SYS_GETPID] = kernGetPID;


    return;
}