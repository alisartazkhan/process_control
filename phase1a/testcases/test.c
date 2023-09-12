
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>



int ff(char *);

int testcase_main(void)
{
    fork1("XXp1", ff, "XXP1", USLOSS_MIN_STACK, 2);
    fork1("XXp2", ff, "XXP2", USLOSS_MIN_STACK, 2);
    fork1("XXp3", ff, "XXP3", USLOSS_MIN_STACK, 2);


    dumpProcesses();


    TEMP_switchTo(4);
    join(NULL);
    dumpProcesses();

    TEMP_switchTo(6);
    join(NULL);
    dumpProcesses();

    TEMP_switchTo(5); // GIVES SEG FAULT SINCE IT CANT FREE THE STACK MEMORY IN QUIT()
    join(NULL); 
    dumpProcesses();





    USLOSS_Halt(0);

    return 6;
 }



int ff(char* arg)
{

    printf("%s process runs\n", arg);
    return 0;
}



//process will rerun if switched to again after finishing. I think this is because we reset the context to NULL