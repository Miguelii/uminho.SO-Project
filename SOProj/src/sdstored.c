#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>

// SERVER

int main(int argc, char argv[]) {
    if(argc < 2) perror("falta argumentos ");

    int maxbcompress;
    char comando[100];

    int maxbdecompress;

    if(strcmp(argv[1],"bcompress") == 0) {
        maxbcompress = argv[2];
        //strcpy(buf,argv[3]);
        
    }
}