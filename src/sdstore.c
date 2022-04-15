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

    int processing_fifo = open("/tmp/processing_fifo", O_RDONLY);
    if(processing_fifo == -1) perror("Erro fifo processing");

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
        close(server_client_fifo);
    }

    if(strcmp(argv[1],"proc-file") == 0) {
        // ./sdstore proc-file priority input-filename output-filename transformation-id-1 transformation-id-2
        
        char mensagem[5000];
        char res[5000];
        res[0] = 0; 

        for(int cont = 1; cont<argc;cont++){
            sprintf(mensagem,argv[cont]);
            strcat(res,mensagem);
            strcat(res," ");
        }

        write(client_server_fifo,res,strlen(res));
        close(client_server_fifo);

        int leitura = 0;
        char buffer[1024];
        while ((leitura = read(processing_fifo, buffer, sizeof(buffer))) > 0) {
            write(1,buffer,leitura);
            if (strstr(buffer, "Processing...")) break;
        }
        close(processing_fifo);

    }
}

