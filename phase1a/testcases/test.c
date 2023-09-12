
#include <stdio.h>
#include <usloss.h>
#include <phase1.h>



int ff(char *);

int   tm_pid = -1;

int testcase_main(void)
{


    int status;
    for (int i = 0; i < 47; i++){
        fork1("XXp1", ff, "XXP1", USLOSS_MIN_STACK, 2);



    }
         TEMP_switchTo(43);

        //fork1("extra", ff, "XXP1", USLOSS_MIN_STACK, 2);

     //fork1("XXp2", ff, "XXP2", USLOSS_MIN_STACK, 2);
     //fork1("XXp3", ff, "XXP3", USLOSS_MIN_STACK, 2);
    // tm_pid = getpid();



     join(&status);

              dumpProcesses();

        fork1("extra", ff, "XXP1", USLOSS_MIN_STACK, 2);


    dumpProcesses();

    // //TEMP_switchTo(5);
    // //join(NULL);
    // dumpProcesses();

    // TEMP_switchTo(6); // GIVES SEG FAULT SINCE IT CANT FREE THE STACK MEMORY IN QUIT()
    // join(NULL); 
    // dumpProcesses();





    USLOSS_Halt(0);

    return 6;
 }



int ff(char* arg)
{

    printf("%s process runs\n", arg);
    quit(0, 3);

    return 0;
}



//process will rerun if switched to again after finishing. I think this is because we reset the context to NULL