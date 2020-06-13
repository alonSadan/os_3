#include "types.h"
#include "stat.h"
#include "user.h"

void testfork();
void testAlloc();
void testONpageFault1();
void testONpageFault2();
void testONpageFault3();
void sanitycow();
void testcow1();
void testcow2();
void testcow3();
void EladTestfork();
void EladTest1();

void fail(char *errorMassage);
void pass(char *message);

int strncmp();
void printStats();
int print = 0;

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

    if (argc > 1)
    {
        if (*argv[1] == 'p')
        {
            print = 1;
        }
    }
    printf(1, "starting automate tests\n");
    //testAlloc();
    //testfork();
    // testONpageFault1();
    //     testONpageFault2();
//    testONpageFault3();            //not passing - panic aquire
  // sanitycow();                   //not passing , but this could be OK
    // testcow1();
    // testcow2();
    // testcow3();
     EladTestfork();
    // EladTest1();
    // printf(1, "automate tests passed!!!!!!!!!!\n");
    // printf(1, "please check the satats:\n");
    // //pageReplacmentStats();

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
    if (print)
    {
        printStats();
    }

    printf(1, "TEST PASSED : %s\n", message);
}

void fail(char *message)
{
    if (print)
    {
        printStats();
    }

    printf(1, "TEST FAILED : %s\n", message);
    exit();
}
// test alloc : alloc(malloc/sbrk) more then 16 pages on check pageInMemory and pageInSwapFile values
void testAlloc()
{

    printf(1, "testAlloc:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;

    int numberOfPages = 20;
    sbrk(4096 * numberOfPages);
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    if (memoryPages < 16)
    {
        fail("should allocate pages on swap only when memory is full");
    }
    if (swapPages < 4)
    {
        fail("should have at least 4 pages in swap");
    }
    pass("allocation test passed");
}
// test fork : fork a process and check that the child has the same number of swap pages and physical pages
void testfork()
{
    printf(1, "testfork:\n");

    //child feilds
    int pid;
    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    //Parent feilds
    int parentMemoryPages = null;
    int parentSwapPgaes = null;
    int parentPageFaults = null;
    int parentPagedOut = null;

    int numberOfPages = 16;
    char *buf = malloc(4096 * numberOfPages);
    //Parent feilds
    getStats(&parentMemoryPages, &parentSwapPgaes, &parentPageFaults, &parentPagedOut);
    if ((pid = fork()) == 0)
    {
        sleep(5);
        if (print)
        {
            printf(1, "child stats:\n");
            printStats();
        }
    }
    else
    {
        //printf(1,"parent pid %d,i=%d, freePages=%d \n",getpid(),i,getNumberOfFreePages());
        pid = wait2(&memoryPages, &swapPages, &pageFaults, &pagedOut);
        if (parentMemoryPages != memoryPages)
        {
            printf(1, "parnet memory pages: %d\n child memory pages: %d\n", parentMemoryPages, memoryPages);
            fail("test fork failed: memory pages were not equal\n");
        }
        if (parentSwapPgaes != swapPages)
        {
            printf(1, "parnet swap Pages: %d\n child swap pages: %d\n", parentSwapPgaes, swapPages);
            fail("test fork failed: swap pages were not equal\n");
        }
        printf(1, "before freeing buf:\n");
        if (print)
        {
            printStats();
        }

        free(buf);
        printf(1, "after freeing buf:\n");
        pass("test fork passed\n");
    }
}

// OnPageFault1: when ram is not full write to addres that located on disk and check if the write works
void testONpageFault1()
{
    printf(1, "testONpageFault1:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    int numberOfPages = 5; //choos 5 to make sure pages are not allocated in ram.

    char *buf = sbrk(4096 * numberOfPages);

    for (int i = 0; i < numberOfPages; i++)
    {
        strcpy(&buf[i * 4096], "test");
    }
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    if (memoryPages < 5)
    {
        fail("did not allocate pages in memory");
    }

    if (swapPages > 0 || swapPages > 0)
    { //maybe check pagefaults?
        fail("should not allocate pages in swap");
    }
    for (int i = 0; i < numberOfPages; i++)
    {
        if (strcmp("test", &buf[i * 4096]) < 0)
        {
            fail("on page fault test 2 : failed to write to page on ram\n");
        }
    }
    printf(1, "before freeing buf:\n");
    if (print)
    {
        printStats();
    }

    free(buf);
    printf(1, "after freeing buf:\n");
    pass("onPageFault-1 passed\n");
}
// OnPageFault2: same as OnPagefault 1 on 'full ram'
void testONpageFault2()
{
    printf(1, "testONpageFault2:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    int numberOfPages = 16; //choos 16 to make sure pages are ram is full and swap have free space.

    char *buf = sbrk(4096 * numberOfPages);

    for (int i = 0; i < numberOfPages; i++)
    {
        strcpy(&buf[i * 4096], "test");
    }

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    if (memoryPages < 16)
    {
        fail("ram should be full\n");
    }

    if (swapPages <= 0)
    {
        fail("swap should have pages saved on it\n");
    }

    if (pagedOut < 0)
    {
        fail(" should have at least one paged out page\n");
    }
    int oldPagedOut = pagedOut;

    for (int i = 0; i < numberOfPages; i++)
    {
        if (strcmp("test", &buf[i * 4096]) < 0)
        {

            fail("failed to write to page on ram or on swap when memory is full and swap is not\n"); // I don't think we can tell if the [age we are reading from is in swap or in ram
        }
    }
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    if (oldPagedOut == pagedOut)
    {
        printf(1, "old paged out is %d, current paged out is %d\n", oldPagedOut, pagedOut);
        fail("should have paged out more pages after reading\n");
    }
    if (pageFaults <= 0)
    {
        fail(" should have at least one page fault\n");
    }
    printf(1, "before freeing buf:\n");
    if (print)
    {
        printStats();
    }

    free(buf);
    printf(1, "after freeing buf:\n");
    pass("onPageFault-2 passed\n");
}
// OnPageFault3: same as OnPagefault 1 on 'full ram full disk'
void testONpageFault3()
{
    printf(1, "testONpageFault3:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    int numberOfPages; //choos 16 to make sure pages are ram is full and swap have free space.

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    numberOfPages = (32 - memoryPages); //fill memory completely
    printf(1, "number of pages to allocate %d\n", numberOfPages);
    char *buf = sbrk(4096 * numberOfPages);

    for (int i = 0; i < numberOfPages; i++)
    {

        strcpy(&buf[i * 4096], "test");
    }

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    if (memoryPages < 16)
    {
        fail("ram should be full\n");
    }

    if (swapPages < 16)
    {
        fail("swap should be full\n");
    }

    if (pagedOut < 0)
    {
        fail(" should have at least one paged out page\n");
    }

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    int oldPagedOut = pagedOut;

    for (int i = 0; i < numberOfPages; i++)
    {
        if (strcmp("test", &buf[i * 4096]) < 0)
        {

            fail("failed to write to page on ram or on swap when ram and swap are full\n"); // I don't think we can tell if the [age we are reading from is in swap or in ram
        }
    }

    if (oldPagedOut == pagedOut)
    {
        fail("should have paged out more pages after reading\n");
    }
    if (pageFaults <= 0)
    {
        fail(" should have at least one page fault\n");
    }

    printf(1, "before freeing buf:\n");
    if (print)
    {
        printStats();
    }

    free(buf);
    printf(1, "after freeing buf:\n");
    pass("onPageFault-3 passed\n");
}

void sanitycow()
{
    printf(1, "sanitycow:\n");

    int pid;
    int oldNumberOfFreePages = getNumberOfFreePages();
    printf(1, "before test numebr of free pages %d\n", getNumberOfFreePages());
    if ((pid = fork()) == 0)
    {
       // printf(1, "child numebr of free pages %d\n", getNumberOfFreePages());
        while (1)
        {
        };
    }

    sleep(10);
    printf(1, "father numebr of free pages %d\n", getNumberOfFreePages());
    if (oldNumberOfFreePages != getNumberOfFreePages())
    {
        kill(pid);
        wait();
        fail("did not allocate new pages for child in time");
    }
    kill(pid);
    wait();
    pass("sanity cow test passed");
}
void testcow1()
{
    printf(1, "testcow2:\n");

    char *test = "test";
    int pid;
    int oldNumberOfFreePages = getNumberOfFreePages();
    if ((pid = fork()) == 0)
    {

        strcpy(test, "pass");
        printf(1,"child number of free pages: %d\n", getNumberOfFreePages());
        while (1)
        {
        };
    }

    sleep(10);
    printf(1,"parent number of free pages: %d\n", getNumberOfFreePages());
    if (oldNumberOfFreePages <= getNumberOfFreePages())
    {
        kill(pid); //I use kill so that I can fail the test properly. in testcow2 I made it the regular way
        wait();
        fail("testcow1: did not allocate new pages for child in time\n");
    }
    kill(pid);
    wait();
    pass("test cow 1 passed");
}

// COW2: like COW1 with more then 1 child (N childs) , and check that addresses are different
void testcow2()
{
    printf(1, "testcow2:\n");
    char *test = "test";
    int pid;
    int numberOfchilds = 10;
    int oldNumberOfFreePages;

    for (int i = 0; i < numberOfchilds; ++i)
        if ((pid = fork()) == 0)
        {
            sleep(10);
            oldNumberOfFreePages = getNumberOfFreePages();
            strcpy(test, "pass");
            if (oldNumberOfFreePages <= getNumberOfFreePages())
            {
                fail("test cow 2 failed : one of the childs did not get new pages when cow'ing\n");
            }
            exit();
        }

    sleep(10);
    for (int i = 0; i < numberOfchilds; ++i)
    {
        wait();
    }
    pass("test cow 2 passed\n");
}

// COW3: decrease the number of MAX_PYSHC_PAGES and check the behaviour
void testcow3()
{
    printf(1, "testcow3:\n");
    int numberOfPages = 16;
    char *buf = sbrk(4096 * numberOfPages);
    if (print)
    {
        printStats();
    }

    testcow1();
    testcow2();

    printf(1, "before freeing buf:\n");
    if (print)
    {
        printStats();
    }

    free(buf);
    printf(1, "after freeing buf:\n");
    pass("testcow-3 passed\n");
}

void EladTestfork()
{
    printf(1, "EladTestfork:\n");

    int pages = 25;
    char *buf = sbrk(4096 * pages);

    for (int i = 0; i < pages; i++)
    {
        buf[i * 4096] = 'a';
    }

    if (fork() == 0)
    {
        printf(1,"hasdjkannnnnnnnnnnnnnnnnnnnnn\n");
        for (int i = 0; i < pages; i++)
        {
            printf(1, "child data: %c\n", buf[i * 4096]);
        }

        for (int i = 0; i < pages; i++)
        {
            buf[i * 4096] = 'b';
        }

        for (int i = 0; i < pages; i++)
        {
            printf(1, "child data after change: %c\n", buf[i * 4096]);
        }
        printf(1, "child ended  in test\n");
    }
    else
    {
        for (int i = 0; i < pages; i++)
        {
            printf(1, "father data: %c\n", buf[i * 4096]);
        }
        wait();
        free(buf);
    }
}

void EladTest1()
{
    int pages = 18;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = malloc(4096 * pages);
    for (int i = 0; i < pages; i++)
    {
        buf[i * 4096] = 'a';
    }
    printf(1, "good\n");
    for (int i = 0; i < pages; i++)
    {
        printf(1, "data: %c\n", buf[i * 4096]);
    }

    printf(1, "calling free\n");
    sleep(10);
    free(buf);
}

int strncmp(const char *p, const char *q, int n)
{
    printf(1, "compare %d chars. now %c  and  %c", n, p, q);
    while (n && *p == *q)
    {
        printf(1, "compare %d chars. now %c  and  %c", n, p, q);
        p++, q++, n--;
    }
    return (uchar)*p - (uchar)*q;
}

void printStats()
{

    int memoryPages = 0;
    int swapPages = 0;
    int pageFaults = 0;
    int pagedOut = 0;

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    printf(1, "PROC_NUM\tPAGES_IN_MEMORY\t\tPAGES_IN_SWAP\tTOTAL_PAGE_FAULTS\tTOTAL_PAGED_OUT\t\tFREE_PAGES_IN_MEMORY\n");
    printf(1, "%d\t\t%d\t\t\t%d\t\t\t%d\t\t%d\t\t\t%d\n", getpid(), memoryPages, swapPages, pageFaults, pagedOut, getNumberOfFreePages());
}