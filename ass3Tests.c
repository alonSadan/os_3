#include "types.h"
#include "stat.h"
#include "user.h"

//ToDo: add all the passed test to here
//ToDo: verify that each test start with no pages (in worse case run each test alone)

void printStats()
{

    int memoryPages = 0;
    int swapPages = 0;
    int pageFaults = 0;
    int pagedOut = 0;

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    printf(1, "PROC_NUM\tPAGES_IN_MEMORY\t\tPAGES_IN_SWAP\tTOTAL_PAGE_FAULTS\tTOTAL_PAGED_OUT\t\tFREE_PAGES_IN_MEMORY\n");
    printf(1, "%d\t\t%d\t\t\t%d\t\t\t%d\t\t%d\t\t\t%d\n\n", getpid(), memoryPages, swapPages, pageFaults, pagedOut, getNumberOfFreePages());
}

void pass(char *message)
{
    printStats();

    printf(1, "TEST PASSED : %s\n", message);
}

void fork_cow_with_swap()
{
    //bug: exit garbage
    printf(1, "fork_cow_with_swap starting.........\n");
    int pages = 21;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = sbrk(4096 * pages);
    for (int i = 0; i < pages; i++)
    {
        buf[i * 4096] = 'a';
    }
    if (fork() == 0)
    {
        for (int i = 0; i < pages; i++)
        {
            printf(1, "child data: %c\n", buf[i * 4096]);
        }

        printf(1, "heeeeeeeeeeeer\n");
        for (int i = 0; i < pages; i++)
        {
            buf[i * 4096] = 'b';
        }

        for (int i = 0; i < pages; i++)
        {
            printf(1, "child data after change: %c\n", buf[i * 4096]);
        }
        exit();
    }
    else
    {
        for (int i = 0; i < pages; i++)
        {
            printf(1, "father data: %c\n", buf[i * 4096]);
        }
        wait();
        sbrk(-4096 * pages);
        printf(1, "check print!!\n");
        pass("test fork cow with swap passed\n");
    }
}

void fork_cow_no_swap()
{
    printf(1, "fork_cow_no_swap starting.........\n");

    int pages = 10;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = sbrk(4096 * pages);
    for (int i = 0; i < pages; i++)
    {
        sleep(10);
        buf[i * 4096] = 'a';
    }
    if (fork() == 0)
    {
        for (int i = 0; i < pages; i++)
        {
            sleep(1);
            printf(1, "child data: %c\n", buf[i * 4096]);
        }
        for (int i = 0; i < pages; i++)
        {
            sleep(1);
            buf[i * 4096] = 'b';
        }
        for (int i = 0; i < pages; i++)
        {
            sleep(1);
            printf(1, "child data after change: %c\n", buf[i * 4096]);
        }
        exit();
    }
    else
    {
        for (int i = 0; i < pages; i++)
        {
            sleep(1);
            printf(1, "father data: %c\n", buf[i * 4096]);
        }
        wait();
        sbrk(-4096 * pages);
        pass("test cow no fork passed\n");
    }
}

void fail(char *message)
{
    printStats();

    printf(1, "TEST FAILED : %s\n", message);
    exit();
}

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
        sbrk(-(4096 * numberOfPages));
        fail("should allocate pages on swap only when memory is full");
    }
    if (swapPages < 4)
    {
        sbrk(-(4096 * numberOfPages));
        fail("should have at least 4 pages in swap");
    }
    sbrk(-(4096 * numberOfPages));
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
    sbrk(4096 * numberOfPages);
    //Parent felids
    getStats(&parentMemoryPages, &parentSwapPgaes, &parentPageFaults, &parentPagedOut);

    pid = fork();
    if (pid == 0)
    {
        sleep(5);
        printf(1, "child stats:\n");
        printStats();
        sbrk(-(4096 * numberOfPages));
        exit();
    }
    else
    {
        //printf(1,"parent pid %d,i=%d, freePages=%d \n",getpid(),i,getNumberOfFreePages());
        pid = wait2(&memoryPages, &swapPages, &pageFaults, &pagedOut);

        if (parentMemoryPages != memoryPages)
        {
            sbrk(-(4096 * numberOfPages));
            printf(1, "parnet memory pages: %d\n child memory pages: %d\n", parentMemoryPages, memoryPages);
            fail("test fork failed: memory pages were not equal\n");
        }
        if (parentSwapPgaes != swapPages)
        {
            sbrk(-(4096 * numberOfPages));
            printf(1, "parnet swap Pages: %d\n child swap pages: %d\n", parentSwapPgaes, swapPages);
            fail("test fork failed: swap pages were not equal\n");
        }
        printf(1, "before freeing buf:\n");
        printStats();
        //free(buf);
        sbrk(-(4096 * (numberOfPages)));
        printf(1, "after freeing buf:\n");
        pass("test fork passed\n");
    }
}

void testONpageFault3()
{
    printf(1, "testONpageFault3:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    int numberOfPages; //choos 16 to make sure pages are ram is full and swap have free space.

    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);
    numberOfPages = (32 - (memoryPages + swapPages)); //fill memory completely
    printf(1, "number of pages to allocate %d\n", numberOfPages);
    char *buf = sbrk(4096 * numberOfPages);
    printStats();
    for (int i = 1; i < numberOfPages; i++)
    {
        strcpy(&buf[i * 4096] - 5, "test");
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
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    if (oldPagedOut == pagedOut)
    {
        fail("should have paged out more pages after reading\n");
    }
    if (pageFaults <= 0)
    {
        fail(" should have at least one page fault\n");
    }

    printf(1, "before freeing buf:\n");

    printStats();

    sbrk(-(4096 * numberOfPages));
    printf(1, "after freeing buf:\n");
    pass("onPageFault-3 passed\n");
}
void nfu_test()
{
    int pages = 19;
    int loops = 2;
    int time = 5;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = sbrk(4096 * pages);

    // set data
    for (int i = 0; i < pages; i++)
    {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // access half data
    for (int j = 0; j < loops; ++j)
    {
        sleep(time);
        printf(1, "loop: %d\n", j);
        for (int i = pages / 2; i < pages; i++)
        {
            printf(1, "data: %c\n", buf[i * 4096]);
        }
    }
    // access all data
    for (int j = 0; j < loops; ++j)
    {
        sleep(time);
        printf(1, "\nloop: %d\n\n", j);
        for (int i = 0; i < pages; i++)
        {
            printf(1, "data: %c\n", buf[i * 4096]);
        }
    }
    sbrk(-4096 * pages);
    printf(1, "check prints!!\n");
    pass("nufa test passed\n");
    // sleep(10);
    // free(buf);
}

void scfifo_test()
{
    int pages = 20;
    //    int loops = 3;
    int time = 1;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = sbrk(4096 * pages);

    // set half data
    for (int i = 0; i < pages / 2; i++)
    {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // set half data again
    for (int i = 0; i < pages / 2; i++)
    {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // set all data
    for (int i = pages - 1; i >= 0; i--)
    {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    sbrk(-4096 * pages);
    printf(1, "check prints!!\n");
    pass("scfifo test passed\n");
}
void testONpageFault1()
{
    printf(1, "testONpageFault1:\n");

    int memoryPages = null;
    int swapPages = null;
    int pageFaults = null;
    int pagedOut = null;
    int numberOfPages = 5; //choos 5 to make sure pages are not allocated in ram.
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    //sometimes we have leftovers from other tests
    int oldNumberofSwapPages = swapPages;
    int oldNumberofRamPages = memoryPages;

    char *buf = sbrk(4096 * numberOfPages);

    for (int i = 0; i < numberOfPages; i++)
    {
        strcpy(&buf[i * 4096], "test");
    }
    getStats(&memoryPages, &swapPages, &pageFaults, &pagedOut);

    if (memoryPages < oldNumberofRamPages + 5)
    {
        fail("did not allocate pages in memory");
    }

    if (swapPages > oldNumberofSwapPages)
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
    printStats();
    free(buf);
    sbrk(-(4096 * numberOfPages));
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
    printStats();

    sbrk(-(4096 * numberOfPages));
    printf(1, "after freeing buf:\n");
    pass("onPageFault-2 passed\n");
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
        printf(1, "child number of free pages: %d\n", getNumberOfFreePages());
        while (1)
        {
        };
    }

    sleep(10);
    printf(1, "parent number of free pages: %d\n", getNumberOfFreePages());
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
        sleep(2);
        wait();
    }
    pass("test cow 2 passed\n");
}

// COW3: decrease the number of MAX_PYSHC_PAGES and check the behaviour
void testcow3()
{
    printf(1, "testcow3:\n");
    int numberOfPages = 16;
    sbrk(4096 * numberOfPages);
    printStats();

    testcow1();
    testcow2();

    printf(1, "before freeing buf:\n");
    printStats();
    sbrk(-(4096 * numberOfPages));
    printf(1, "after freeing buf:\n");
    pass("testcow-3 passed\n");
}

int main(int argc, char **argv)
{
    fork_cow_with_swap();
    fork_cow_no_swap();
    testAlloc();
    testfork();
    nfu_test();
    scfifo_test();
    testONpageFault1();
    testONpageFault2();
    testONpageFault3();
    testcow1();
    testcow2();
    testcow3();

    printf(1,"\nALL TESTS PASSED\n");
    exit();
}
