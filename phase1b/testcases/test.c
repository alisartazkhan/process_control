
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);


int testcase_main()
{
    
    int status;

    USLOSS_Console("testcase_main(): started\n");

    fork1("other", XXp1, NULL,USLOSS_MIN_STACK,1);

    join(&status);

    USLOSS_Console("JOIN has been ran");

    dumpProcesses();


    // /blockMe(5);

    USLOSS_Console("EXPECTATION: Init Process should be in the Queue.\n");

    dumpQueue();

    quit(5);
    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started, pid = %d\n", getpid());
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
    USLOSS_Console("XXp1(): this process will terminate immediately.\n");
    quit(getpid());
}


