#include "types.h"
#include "stat.h"
#include "user.h"

void testfork();
void testalloc();
void testONpageFault1();
void testONpageFault2();
void testONpageFault3();
void testcow1();
void testcow2();
void testcow3();
void fail(char *errorMassage);
void pass(char *message);
void pageReplacmentStats();

// test cases:
// test fork : fork a process and check that the child has the same number of swap pages and physical pages
// test alloc : alloc(malloc/sbrk) more then 16 pages on check pageInMemory and pageInSwapFile values

// OnPageFault1: when ram is not full write to addres that located on disk and check if the write works
// OnPageFault2: same as OnPagefault 1 on 'full ram'
// OnPageFault3: same as OnPagefault 1 on 'full ram full disk'

// COW1: like partical session 1: check number of free pages before and after write to the copied page
// COW2: like COW1 with more then 1 child (N childs) , and check that addresses are different
// COW3: decrease the number of MAX_PYSHC_PAGES and check the behaviour

// pageReplacement1: check each algo sepratley: change the value of PTE_A for different pages and check

int main(int argc, char **argv)
{

    printf(1, "starting automate tests\n");
    testfork();
    //testalloc();
    // testONpageFault1();
    // testONpageFault2();
    // testONpageFault3();
    // testcow1();
    // testcow2();
    // testcow3();
    printf(1, "automate tests passed!!!!!!!!!!\n");
    printf(1, "please check the satats:\n");
    //pageReplacmentStats();

    // malloc(4096*16);

    // int pid;//,processNum=1;
    // int i = 1234;
    // printf(1,"start: pid=%d i=%d free pages=%d\n",getpid(),i, getNumberOfFreePages());
    // sleep(10);
    // printf(1,"start2: pid=%d i=%d free pages=%d\n",getpid(),i, getNumberOfFreePages());
    // if ((pid = fork()) == 0){
    //     printf(1,"child pid %d i=%d, free pages=%d \n",getpid(),i,getNumberOfFreePages());
    //     i = 6666;
    //     printf(1,"child pid %d i=%d, free pages=%d \n",getpid(),i,getNumberOfFreePages());
    //     exit();
    // }else{
    //     //printf(1,"parent pid %d,i=%d, freePages=%d \n",getpid(),i,getNumberOfFreePages());
    //     wait();
    // }

    // printf(1,"end: i=%d free pages: %d\n",i, getNumberOfFreePages());
    // for (int kid = 0; kid < processNum; kid++) {
    //     //Child process
    //     if ((pid = fork()) == 0){
    //         printf(1,"child %d before write - NumberOf pages: %d\n",kid,getNumberOfFreePages());
    //         sleep(10);
    //         printf(2,"child %d before write - NumberOf pages: %d\n",kid,getNumberOfFreePages());
    //         exit();
    //     }
    // }
    // for (int kid = 0; kid < processNum; ++kid) {
    //     wait(); // kids could be ready in any order
    // }

    exit();
}

void pass(char *message)
{
    printf(1, "TEST PASSED : %s\n", message);
}

// test fork : fork a process and check that the child has the same number of swap pages and physical pages
void testfork()
{
    malloc(4096 * 16);

    //child feilds
    int pid;
    int *memoryPages = null;
    int *swapPages = null;
    int *pageFaults = null;
    int *pagedOut = null;
    int *parentMemoryPages = null;
    int *parentSwapPgaes = null;
    int *parentPageFaults = null;
    int *parentPagedOut = null;

    //Parent feilds
    getStats(parentMemoryPages,parentSwapPgaes,parentPageFaults,parentPagedOut);
    if ((pid = fork()) == 0)
    {
        sleep(5);
    }
    else
    {
        //printf(1,"parent pid %d,i=%d, freePages=%d \n",getpid(),i,getNumberOfFreePages());
        pid = wait2(memoryPages, swapPages, pageFaults, pagedOut);
        if (parentMemoryPages != memoryPages)
        { 
            printf(1,"parnet memory cages: %d\n child memory pages: %d\n",parentMemoryPages,memoryPages);
            fail("test fork failed: memory pages were not equal\n");
        }
        if (parentSwapPgaes != swapPages)
        {
            printf(1, "parnet swap Pages: %d\n child swap pages: %d\n", parentSwapPgaes, swapPages);
            fail("test fork failed: swap pages were not equal\n");
        }

        pass("test fork passed");
    }
}

void testalloc(){

}
