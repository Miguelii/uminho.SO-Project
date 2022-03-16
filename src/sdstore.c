#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>

// CLIENT

int main(int argc, char *argv[]) {
    if(strcmp(argv[1],"status") == 0) {
        write(1,"Transf nop: 3/3 (Running/Max)",29);
    }
}

