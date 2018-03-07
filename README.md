Objectives
1. To learn the basic programming technique in Linux kernel.
2. To learn the software structures of kernel thread and synchronization.
3. To practice the mechanisms and data structures of system call, wait queue, and other Linux


Barrier synchronization has been applied widely in parallel computing to synchronize the execution of parallel loops. The basic idea is to allow parallel loop computation to start next iteration if all loops have completed the previous iteration. In NPTL pthread library, it is realized by 3 functions for threads of the same process, i.e., pthread_barrier_init, pthread_barrier_wait, and pthread_barrier_destroy. 

A description of the functions can be found in https://docs.oracle.com/cd/E19253-01/816-5137/gfwek/index.html, and their source code is available in glibc NPTL cross reference page http://code.metager.de/source/xref/gnu/glibc/nptl/.


To demonstrate your barrier implementation, a testing program should be developed. The testing program forks two child processes. In each child process, there are 5 threads to exercise the 1 st barrier and additional 20 threads use the 2 nd barrier. Each thread sleeps a random amount of time.
