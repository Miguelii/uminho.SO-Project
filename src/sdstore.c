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
    int client_server_fifo = open("/tmp/client_server_fifo",O_WRONLY);
    if(client_server_fifo == 1) perror("Erro fifo");
    
    int server_client_fifo = open("/tmp/server_client_fifo", O_RDONLY);
    if(server_client_fifo == -1) perror("Erro fifo");

    if(strcmp(argv[1],"status") == 0) {
        printf("Status lido! \n");
        write(client_server_fifo, argv[1],strlen(argv[1]));
        close(client_server_fifo);

        //obter a resposta
        int leitura = 0;
        char buffer[1024];
        while((leitura = read(server_client_fifo,buffer,sizeof(buffer))) > 0) {
            write(1,buffer,leitura);
            if(strstr(buffer, "\0")) break;
        }
        printf("write! \n");
        close(server_client_fifo);
    }
}

