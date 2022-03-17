#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>
#include <signal.h>

// SERVER

// $ ./bin/sdstored etc/sdstored.conf bin/sdstore-transformations

void sigint_handler (int signum) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) wait(NULL); //Termina todos os filhos.

    //Remove os ficheiros temporários criados pelos named pipes
    write(1, "\nA terminar servidor.\n", strlen("\nA terminar servidor.\n"));
    if (unlink("/tmp/server_client_fifo") == -1) {
        perror("[server_client_fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    if (unlink("/tmp/client_server_fifo") == -1) {
        perror("[client-server-fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    _exit(0);
}

int main(int argc, char *argv[]) {

    if(argc < 3) perror("falta argumentos ");

    char buffer[1024];
    int n;
    int maxnop, maxbcompress, maxbdecompress, maxgcompress, maxgdecompress, maxencrypt, maxdencrypt = 0;

    int fd_conf = open(argv[1],O_RDONLY);
    if(fd_conf == -1) perror("Erro no .conf");

    while((n = read(fd_conf,buffer,sizeof(buffer))) > 0) {
        char *token = strtok(buffer, "\n");  // breaks buffer into a series of tokens using the delimiter \n

        do {
            char *aux = strdup(token); //copia a string token para aux
            char *found = strsep(&aux, " "); // seleciona a string de aux até ao separador " "

            if(strcmp(found,"nop") == 0) {
                maxnop = atoi(strsep(&aux, "\n"));
            }

            else if(strcmp(found,"bcompress") == 0) {
                maxbcompress = atoi(strsep(&aux, "\n"));
            }        

            else if(strcmp(found,"bdecompress") == 0) {
                maxbdecompress = atoi(strsep(&aux, "\n"));
            }
            
            else if(strcmp(found,"gcompress") == 0) {
                maxgcompress = atoi(strsep(&aux, "\n"));
            }  

            else if(strcmp(found,"gdecompress") == 0) {
                maxgdecompress = atoi(strsep(&aux, "\n"));
            }        

            else if(strcmp(found,"encrypt") == 0) {
                maxencrypt = atoi(strsep(&aux, "\n"));
            }
            
            else {
                maxdencrypt = atoi(strsep(&aux, "\n"));
            } 

        } while((token = strtok(NULL,"\n")));
    }

    close(fd_conf);

    /*debug
    printf("%d \n", maxnop);
    printf("%d \n", maxbcompress);
    printf("%d \n", maxbdecompress);
    printf("%d \n", maxgcompress);
    printf("%d \n", maxgdecompress);
    printf("%d \n", maxencrypt);
    printf("%d \n", maxdencrypt); */

    //cria os named pipes
    int client_server_fifo;
    int server_client_fifo;

    if(mkfifo("/tmp/client_server_fifo",0600) == -1) {
        perror("Named pipe 1 error");
    }

    if(mkfifo("/tmp/server_client_fifo",0600) == -1) {
        perror("Named pipe 2 error");
    }
    
    //Declaração dos handlers dos sinais.
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("[signal] erro da associação do signint_handler.");
        exit(-1);
    }

    //Abrir pipe
    char comando[1024];

    client_server_fifo = open("/tmp/client_server_fifo",O_RDONLY);
    if(client_server_fifo == -1) perror("Erro fifo cliente-server");
    
    server_client_fifo = open("/tmp/server_client_fifo",O_WRONLY);
    if(server_client_fifo == -1) perror("Erro fifo server-cliente");

    //ler o pipe que vem do cliente
    int leitura = read(client_server_fifo,comando,sizeof(comando));
    if(leitura == -1) perror("Erro no read");

    while(1) {
        if(leitura > 0 && (strcmp(comando,"status") == 0)) {
            //printf("Pipe lido! \n");
            char mensagem[5000];
            char res[5000];
            res[0] = 0;

            sprintf(mensagem, "Transf nop: 0/%d (Running/Max) \n",maxnop);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf bcompress: 0/%d (Running/Max) \n",maxbcompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf bdecompress: 0/%d (Running/Max) \n",maxbdecompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf gcompress: 0/%d (Running/Max) \n",maxgcompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf gdecompress: 0/%d (Running/Max) \n",maxgdecompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf encrypt: 0/%d (Running/Max) \n",maxencrypt);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf decrypt: 0/%d (Running/Max) \n",maxdencrypt);
            strcat(res,mensagem);
            strcat(res,"\0");
            write(server_client_fifo,res,strlen(res));
        }
    }
}