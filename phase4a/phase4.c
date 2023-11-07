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
#include <phase4.h>


void ourSleep();


void phase4_init(void) {
    //USLOSS_IntVec[USLOSS_CLOCK_INT] = daemonProcess;
    systemCallVec[SYS_SLEEP] = ourSleep;
    
    //systemCallVec[SYS_TERMREAD] = ourTermRead;
    //systemCallVec[SYS_TERMWRITE] = ourTermWrite;
    //systemCallVec[SYS_DISKREAD] = ourDiskRead;
    //systemCallVec[SYS_DISKWRITE] = ourDiskWrite;
    //systemCallVec[SYS_DISKSIZE] = ourDiskSize;
}


void daemonProcess() {
    int status = -1;
    while(1) {
        USLOSS_Console("hello");
    waitDevice(USLOSS_CLOCK_DEV, 0, &status);
    
    } return 0;
}


void phase4_start_service_processes(){

    int status;
    fork1("daemon", daemonProcess, NULL, USLOSS_MIN_STACK, 1);
    dumpProcesses();


}

int kernSleep(int seconds) {
    return 0;
}

void ourSleep() {

    USLOSS_Console("inside sleep\n");
    
    return 0;
}

int kernDiskRead(void *diskBuffer, int unit, int track, int first,int sectors, int *status) {

    // Kernel mode disk read implementation
    return 0; // Placeholder return value
}

int kernDiskWrite(void *diskBuffer, int unit, int track, int first, int sectors, int *status) {

    // Kernel mode disk write implementation
    return 0; // Placeholder return value
}

int kernDiskSize(int unit, int *sector, int *track, int *disk) {

    // Kernel mode disk size implementation
    return 0; // Placeholder return value
}

int kernTermRead(char *buffer, int bufferSize, int unitID, int *numCharsRead) {
    // USLOSS_Sysargs *sysargs = args; 

    // Kernel mode terminal read implementation
    return 0; // Placeholder return value
}

int kernTermWrite(char *buffer, int bufferSize, int unitID, int *numCharsRead) {

    // Kernel mode terminal write implementation
    return 0; // Placeholder return value
}