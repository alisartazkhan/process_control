/**
 * Author: Samuel Richardson and Ali Sartaz Khan
 * Course: CSC 452
 * Phase: 3
 * Description: This program uses phase1 and 2 library and USLOSS to
 * implement the syscalls for our operating system. We have all of the
 * usermode code in phase3_usermode.c, which is what will be called by
 * the user. Each of the functions in that file will call its 
 * corresponding handler function in this file, which calls each
 * function using its kernel mode functions. 
**/

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
  This version of process struct is just a shadow version that only
  keeps track of PID, func, and arg.
*/
struct Process {
    int PID;
    int (*startFunc)(char*);
    char* arg;
};


/*
  Struct that represents an a sempahore. It keeps track of value
  to be incremented and decremented, and has a mailBoxID that is
  the id of a mailbox of size 0 that is used as the blocking and 
  unblocking mechanism for the sempaphor.
*/
struct Semaphore {
    int semID;
    int mboxID;
    int value;
};

// arrays
static struct Process shadowProcessTable[MAXPROC];
static struct Semaphore semaphoreTable[MAXSEMS];

/*
    Description: Creates a process and sets its fields to the
    arguments passed in. Allocates the stack memory and adds
    it to the processTable array.

    Arg: int (*startFunc)(char*): function to be run
    Arg: void *arg: argument to pass into startFunc

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
  Description: getter function that return process
   
  Arg: pid of process to retrieve
   
  return: Process that you are looking for
*/
struct Process* getProcess(int pid) {
  return &shadowProcessTable[pid%MAXPROC];
}




/* 
  Description: sets the PSR of USLOSS to user mode
   
  Arg: None
   
  return: None
*/
void switchToUserMode(){
    int psr = USLOSS_PsrGet();  
    psr = USLOSS_PSR_CURRENT_INT;
    USLOSS_PsrSet(psr);
}


/* 
  Description: is called by each fork function passed in, and in turn
  called the actualy function passed in from the shadow process table,
  where the variables are stored.
   
  Arg: None
   
  return: None
*/
void tramp(){

    switchToUserMode();
    struct Process* curProcess = getProcess(getpid()); 
    int status = curProcess->startFunc(curProcess->arg);
    Terminate(status);
}



/* 
  Description: we have no service processes in this project
   
  Arg: None
   
  return: None
*/
void phase3_start_service_processes(){
    return;
}



/* 
  Description: the Syscall handler function for Spawn, which is kernel
  version of Fork. The parameter's fields are cast to their specific
  types, and after the corresponding higher level function is called,
  the return field in arg is populated.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernSpawn(void *args){
    int (*func)(char*);
    USLOSS_Sysargs *sysargs = args;
    char* name = (char*) sysargs->arg5;
    func=  sysargs->arg1;
    char* arg = (char*) sysargs->arg2;
    int stack_size = (int)(long)sysargs->arg3; 
    int priority = (int)(long)sysargs->arg4; 
        
    createShadowProcess(func,arg);
    sysargs->arg1 = (void*)(long) fork1(name, tramp, arg, stack_size, priority);

    return 0;
}


/* 
  Description: the Syscall handler function for Wait, which is kernel
  version of Join. The parameter's fields are cast to their specific
  types, and after the corresponding higher level function is called,
  the return field in arg is populated.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
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


/* 
  Description: the Syscall handler function for Terminate, which is kernel
  version of Quit. The parameter's fields are cast to their specific
  types, and after the corresponding higher level function is called,
  the return field in arg is populated. This functino also difffers
  from quit by looping the quit call until it is ready to be terminated.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernTerminate(void *args){
    USLOSS_Sysargs *sysargs = args; 
    int s1 = sysargs->arg1;
    int status = sysargs->arg1;

    while (join(&status) != -2){}
    sysargs->arg1 = status;
    quit(s1);
    return 0;
}


/* 
  Description: the local  function for kernSemCreate. The
  function creates the semaphore and adds it to the semTable.
   
  Arg: int id, the id of the mailbox to be used
  Arg: int val, the val of the sem to be intialized to
   
  return: int -1 if errored, 0 otherwise
*/
int semCreate(int id, int val){
    for (int i = 0; i < MAXSEMS; i++){
        if (semaphoreTable[i].mboxID == 0){
            struct Semaphore s = {
                .mboxID = id,
                .semID = i,
                .value = val
            };
            semaphoreTable[i] = s;
            semCounter++;
            return;
        }
    }
    return -1;
}


/* 
  Description: the Syscall handler function for semCreate. The
  function creates the semaphore and adds it to the semTable by
  calling semCreate, and the returns the proper fields in the
  arg function.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernSemCreate(void* args) {
    USLOSS_Sysargs *sysargs = args; 
    int value = sysargs->arg1;
    sysargs->arg4 = 0;
    int mailboxID = MboxCreate(0, 0);
    if (semCreate(mailboxID, value) == -1 || value < 0){
        sysargs->arg4 = -1;
        sysargs->arg1 =0;
        return -1;
    }

    sysargs->arg1 = semCounter-1;
    return 0;
}



/* 
  Description: the Syscall handler function for semP. The
  function decremens and locks the sempahor if needed,
  using mBoxRecv and the corresponding mbox in its
  field as the blocking mechanishm
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernSemP(void *args){
    // lock decrement
    USLOSS_Sysargs *sysargs = args; 
    int id = sysargs->arg1;
    struct Semaphore* sem = &semaphoreTable[id];
    int mboxID = sem->mboxID;

    sysargs->arg4 = 0;
    sem->value--;
    if (sem->value < 0){
        int retVal = MboxRecv(mboxID, NULL, NULL);
        if (retVal == -1){
            sysargs->arg4 = -1;
            sem->value ++;
            return;
        }
    }
    
    return 0;
}


/* 
  Description: the Syscall handler function for semV. The
  function increments and unlocks the sempahor if needed,
  using mBoxSend and the corresponding mbox in its
  field as the unblocking mechanishm
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernSemV(void *args){
    // unlock increment
    USLOSS_Sysargs *sysargs = args; 
    int id = sysargs->arg1;
    struct Semaphore* sem = &semaphoreTable[id];
    int mboxID = sem->mboxID;
    sysargs->arg4 = 0;

    
    sem->value++;
    if (sem->value <= 0){

        int retVal = MboxSend(mboxID, NULL, NULL);
        if (retVal == -1){
            sysargs->arg4 = -1;
            sem->value--;
            return;
        }
    }
    

    return 0;
}


/* 
  Description: the Syscall handler function for getTimeOfDay.
  The function simply calls the curretTime function.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernGetTimeofDay(void *args){
    USLOSS_Sysargs *sysargs = args; 
    sysargs->arg1 = currentTime();
    return 0;
}

/* 
  Description: the Syscall handler function for CPUTime.
  The function simply calls the readTime function.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernCPUTime(void *args){
    USLOSS_Sysargs *sysargs = args; 
    sysargs->arg1 = readtime();
    return 0;
}


/* 
  Description: the Syscall handler function for GetPID.
  The function simply calls the getpid function.
   
  Arg: args, void pointer that holds the parameters
   
  return: int 0 (can be anything)
*/
int kernGetPID(void *args){
    USLOSS_Sysargs *sysargs = args; 
    sysargs->arg1 = getpid();
    return 0;
}



/* 
  Description: init function that populates the systemCallVec with
  the corresponding handler functions for each call.
  Arg: None
   
  return: None
*/
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