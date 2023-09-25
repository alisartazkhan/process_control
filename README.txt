TESTCASE 13:
Our output and gradescopes outputs are the same until line 16, where our output prints that testcase_main()
is blocked, but gradescope prints it is runnable. This is from the race between XXp3() and testcase_main().
We call testcase_main first, while gradescope calls XXp3() first. This is allowed in the spec because they
both have the same priority and both are runnable. This race causes us to have two incorrect print lines
in dumpProcesses(), both from us running testcase_main() instead of XXp3(), as well as line 40, where
you run testcase main prints a statement that we printed already, due to it being called before XXp3().
While the dumpProcesses show differences, the rest of the outputs are the same, just in a diffrerent order
because of the race conditions.






TESTCASE 14:
Our output and gradescopes outputs are the same until line 15, where our output prints that testcase_main()
continued to run after the dispatcher selected it, but gradescope printed that XXp3() had started running. 
This is from the race between XXp3() and testcase_main(). This is allowed in the spec because they
both have the same priority and both are runnable. This race causes us to have an incorrect print line 19
in dumpProcesses(), from us running testcase_main() and it blocking itself because of it called join, instead of calling 
XXp3(). While the dumpProcesses show differences, the rest of the outputs are the same, just in a diffrerent order
because of the race conditions.




TESTCASE 15:
Our output and gradescopes outputs are the same until line 17, where our output prints that testcase_main()
continued to run after the dispatcher selected it, but gradescope printed that XXp3() had started running. 
This is from the race between XXp3() and testcase_main(). This is allowed in the spec because they
both have the same priority and both are runnable. This race causes us to have an incorrect print line 21
in dumpProcesses(), from us running testcase_main() and it blocking itself because of it called join, instead of calling 
XXp3(). While the dumpProcesses show differences, the rest of the outputs are the same, just in a diffrerent order
because of the race conditions.




TESTCASE 16:
Our output and gradescopes outputs are the same until line 17, where our output prints that testcase_main()
continued to run after the dispatcher selected it, but gradescope printed that XXp3() had started running. 
This is from the race between XXp3() and testcase_main(). This is allowed in the spec because they
both have the same priority and both are runnable. This race causes us to have an incorrect print line 21
in dumpProcesses(), from us running testcase_main() and it blocking itself because of it called join, instead of calling 
XXp3(). While the dumpProcesses show differences, the rest of the outputs are the same, just in a diffrerent order
because of the race conditions.



TESTCASE 27: 
Our output and gradescopes outputs are the same until line 15, where our output prints that XXP1()
continued to run after the dispatcher selected it, but gradescope printed that XXp2() had started running. 
This is from the race between XXp1() and XXp2(). This is allowed in the spec because they
both have the same priority and both are runnable. This causes us to have different outputs, because gradescope
wants XXp2() to run before XXp1(), whereas we run XXp1() before XXp2(). Similarly, lines 28-29 in the gradescope
output doesn't match ours because based on our scheduler, testcase_main() later. And so we print the same lines but in a 
different order later on. This issue happens because we have many Processes with the same priority.

