#include "types.h"
#include "stat.h"
#include "user.h"

   
int main (int argc, char **argv){
    
    int pid;//,processNum=1;
    int i = 1234;
    printf(1,"start: pid=%d i=%d free pages=%d\n",getpid(),i, getNumberOfFreePages());
    sleep(10);
    printf(1,"start2: pid=%d i=%d free pages=%d\n",getpid(),i, getNumberOfFreePages());
    if ((pid = fork()) == 0){
        printf(1,"child pid %d i=%d, free pages=%d \n",getpid(),i,getNumberOfFreePages());
        i = 6666;
        printf(1,"child pid %d i=%d, free pages=%d \n",getpid(),i,getNumberOfFreePages());
        exit();
    }else{
        //printf(1,"parent pid %d,i=%d, freePages=%d \n",getpid(),i,getNumberOfFreePages());
        wait();
    }

    printf(1,"end: i=%d free pages: %d\n",i, getNumberOfFreePages());
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