
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>

int XXp1(char *);

int testcase_main()
{

    USLOSS_Console("testcase_main(): started\n");
    USLOSS_Console("EXPECTATION: Init Process should be in the Queue.\n");

    dumpQueue();
    return 0;
}

int XXp1(char *arg)
{
    USLOSS_Console("XXp1(): started\n");
    USLOSS_Console("XXp1(): arg = '%s'\n", arg);
}

