#include "types.h"
#include "stat.h"
#include "user.h"

void fork_cow_with_swap() {
    //bug: exit garbage 
    int pages = 24;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = malloc(4096 * pages);
    for (int i = 0; i < pages; i++) {
        buf[i * 4096] = 'a';
    }
    if (fork() == 0) {
        for (int i = 0; i < pages; i++) {
            printf(1, "child data: %c\n", buf[i * 4096]);
        }

        printf(1,"heeeeeeeeeeeer\n");
        for (int i = 0; i < pages; i++) {
            buf[i * 4096] = 'b';
        }

        for (int i = 0; i < pages; i++) {
            printf(1, "child data after change: %c\n", buf[i * 4096]);
        }

    } else {
        for (int i = 0; i < pages; i++) {
            printf(1, "father data: %c\n", buf[i * 4096]);
        }
        wait();
    }
}

void fork_cow_no_swap() {
    int pages = 10;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = malloc(4096 * pages);
    for (int i = 0; i < pages; i++) {
        sleep(1);
        buf[i * 4096] = 'a';
    }
    if (fork() == 0) {
        for (int i = 0; i < pages; i++) {
            sleep(1);
            printf(1, "child data: %c\n", buf[i * 4096]);
        }
        for (int i = 0; i < pages; i++) {
            sleep(1);
            buf[i * 4096] = 'b';
        }
        for (int i = 0; i < pages; i++) {
            sleep(1);
            printf(1, "child data after change: %c\n", buf[i * 4096]);
        }

    } else {
        for (int i = 0; i < pages; i++) {
            sleep(1);
            printf(1, "father data: %c\n", buf[i * 4096]);
        }
        wait();
        printf(1, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFballoc\n");
    }
}

void simple_fork(){
    if (fork() == 0) {
        printf(1, "child \n");
    } else {
        printf(1, "father \n");
        wait();
    }

}

void swap_no_fork() {
    //dealloc version 1 (with pgdir):
    //  bug:in the end try to run exit on pid 2 (which is bad !!) and then panic acquire
    //  working pages Numbers:0-15,17,21,22,23,25...
    //dealloc version 2 (without pgdir):
    //  bug: in the end try to run exit with garbage va 
    //  working pages number:0-15, 16+(some works some dont)

    int pages = 25;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = sbrk(4096 * pages);
    for (int i = 0; i < pages; i++) {
        buf[i * 4096] = 'a';
    }
    printf(1, "good\n");
    for (int i = 0; i < pages; i++) {
        printf(1, "data: %c\n", buf[i * 4096]);
    }

    printf(1, "calling free\n");
    // sleep(10);
    //free(buf);
    sbrk(-4096 * pages);
}

void nfu_test() {
    int pages = 19;
    int loops = 2;
    int time = 5;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = malloc(4096 * pages);

    // set data
    for (int i = 0; i < pages; i++) {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // access half data
    for (int j = 0; j < loops; ++j) {
        sleep(time);
        printf(1, "loop: %d\n", j);
        for (int i = pages/2; i < pages; i++) {
            printf(1, "data: %c\n", buf[i * 4096]);
        }
    }
    // access all data
    for (int j = 0; j < loops; ++j) {
        sleep(time);
        printf(1, "loop: %d\n", j);
        for (int i = 0; i < pages; i++) {
            printf(1, "data: %c\n", buf[i * 4096]);
        }
    }
    // sleep(10);
    // free(buf);
}

void scfifo_test() {
    int pages = 20;
//    int loops = 3;
    int time = 1;
    // printf(1, "asking for %d pages\n",pages);
    char *buf = malloc(4096 * pages);

    // set half data
    for (int i = 0; i < pages/2; i++) {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // set half data again
    for (int i = 0; i < pages/2; i++) {
        sleep(time);
        buf[i * 4096] = 'a';
    }
    // set all data
    for (int i = pages-1; i >=0; i--) {
        sleep(time);
        buf[i * 4096] = 'a';
    }
}


void fork_test()
{

    int pages = 18;
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

        printf(1, "wrtting b\n");

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

void test1()
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

int main(int argc, char *argv[])
{
    //test1();
    //fork_cow_no_swap();
    swap_no_fork(); //working for some number of pages
    //nfu_test();
    //scfifo_test();
    //fork_cow_with_swap(); // not working (check maybe previous versions when it works sometimes)
    //simple_fork();
    exit();
}
