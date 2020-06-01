#include "types.h"
#include "stat.h"
#include "user.h"

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

   
int main (int argc, char **argv){
    
    printf(1,"hello world12345\n");
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