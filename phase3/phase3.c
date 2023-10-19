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
void start3_trampoline(void* func, char* arg) {

  // Print PSR to show we're in kernel mode  
  USLOSS_Console("start3_trampoline: PSR before = 0x%x\n", USLOSS_PsrGet());

  // Set PSR to user mode
  int mode = USLOSS_PsrGet() | USLOSS_PSR_CURRENT_MODE;
  USLOSS_PsrSet(mode);

  // Print PSR to confirm now in user mode
  USLOSS_Console("start3_trampoline: PSR after = 0x%x\n", USLOSS_PsrGet());

  // Call user process main function
  //start3(NULL);
    FunctionPointer function = (FunctionPointer)func; // Cast void* to the function pointer type
    function(arg);

  // We should never get here!
  USLOSS_Halt(1);

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
    
    sysargs->arg1 = (void*)(long) fork1(name, func, arg, stack_size, priority);
    USLOSS_Console("PSR: %d\n",USLOSS_PsrGet());
    start3_trampoline(func,arg);
    // USLOSS_Console("stack size: %d\n", stack_size);
    // USLOSS_Console("name : %s\n", n);
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
    int status;

    while (join(&status) != -2){}
    sysargs->arg1 = status;
    quit(status);
    return 0;
}

int kernSemCreate(void *args){
    return 0;
}

int kernSemP(void *args){
    return 0;
}

int kernSemV(void *args){
    return 0;
}

int kernGetTimeofDay(void *args){
    return 0;
}

int kernCPUTime(void *args){
    return 0;
}

int kernGetPID(void *args){
    return 0;
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