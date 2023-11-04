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

void phase4_init(void) {
    systemCallVec[SYS_SLEEP] = kernSleep;
    systemCallVec[SYS_TERMREAD] = kernTermRead;
    systemCallVec[SYS_TERMWRITE] = kernTermWrite;
    systemCallVec[SYS_DISKREAD] = kernDiskRead;
    systemCallVec[SYS_DISKWRITE] = kernDiskWrite;
    systemCallVec[SYS_DISKSIZE] = kernDiskSize;
}

void phase4_start_service_processes(){}


int kernSleep(USLOSS_Sysargs * args) {
    // Kernel mode sleep implementation
    USLOSS_Sysargs *sysargs = args; 
    int seconds = (int) sysargs->arg1;
    USLOSS_Console("Inside %d\n", seconds);


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