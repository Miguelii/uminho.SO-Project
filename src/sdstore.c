#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>

// CLIENT

// ./bin/sdstore status

int main(int argc, char *argv[]) {
    
    //Pipe
    int client_server_fifo;
    char * myfifo = "/tmp/client_server_fifo";
    mkfifo(myfifo,0666);

    // Criar Pipe para cliente
    client_server_fifo = open(myfifo, O_WRONLY);
    if(client_server_fifo == 1) perror("Erro fifo cliente-server - CLIENTE");

    if(strcmp(argv[1],"status") == 0) {
        printf("Status lido! \n");
        write(client_server_fifo, argv[1],strlen(argv[1]));
        close(client_server_fifo);
    }
}

