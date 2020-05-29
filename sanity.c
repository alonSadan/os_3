#include "types.h"
#include "stat.h"
#include "user.h"

   
int main (int argc, char **argv){
    
    int pid,processNum=1;
    int i = 1234;


    for (int kid = 0; kid < processNum; kid++) {
        //Child process
        if ((pid = fork()) == 0){
            i++;
            exit();
        }
    }
    for (int kid = 0; kid < processNum; ++kid) {
        wait(); // kids could be ready in any order    
    }  
        
    exit();
}