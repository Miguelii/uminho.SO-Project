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

//Contem nome dos executaveis de cada proc-file
char *nop_f, *bcompress_f, *bdecompress_f, *gcompress_f, *gdecompress_f, *encrypt_f, *decrypt_f;
char *dir; //Diretoria dos executáveis dos filtros.

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
    if (unlink("/tmp/processing_fifo") == -1) {
        perror("[processing_fifo] Erro ao eliminar ficheiro temporário");
        _exit(-1);
    }
    _exit(0);
}

//Dependendo do filtro pedido no comando, retorna uma string com a diretoria e nome do executável com o correspondente filtro.
char *assignExec(char *nome) {
    if (!strcmp(nome, "nop")) return strdup(nop_f);
    if (!strcmp(nome, "bcompress")) return strdup(bcompress_f);
    if (!strcmp(nome, "bdecompress")) return strdup(bdecompress_f);
    if (!strcmp(nome, "gcompress")) return strdup(gcompress_f);
    if (!strcmp(nome, "gdecompress")) return strdup(gdecompress_f);
    if (!strcmp(nome, "encrypt")) return strdup(encrypt_f);
    if (!strcmp(nome, "decrypt")) return strdup(decrypt_f);
    return NULL;
}


//Set dos argumentos a serem inseridos na função execvp();
char **setArgs(char *input, char *output, char *remaining) {
    char **ret = (char **) calloc(100, sizeof(char *));
    int current = 0;
    char *aux = assignExec(strsep(&remaining, " "));
    char res[50];
    res[0] = 0;
    strcat(res, dir);
    strcat(res, aux);
    ret[current++] = strdup(res);
    ret[current] = NULL;
    return ret;
}

int executaProc(char *comando) {
    char *found;
    char *args = strdup(comando);

    found = strsep(&args, " ");

    char *input = strsep(&args, " "); //Guarda o nome e path do ficheiro de input.
    char *output = strsep(&args, " "); //Guarda nome e path do ficheiro de output.
    char *resto = strsep(&args, "\n"); //Guarda os filtros pedidos pelo utilizador.

    //inProcess[nProcesses++] = strdup(comando);
    //updateSlots(resto);

    char **argumentos = setArgs(input, output, resto); //Guarda os argumentos a serem fornecidos à função execvp().
    if (!fork()) {
        int exInput;
        if ((exInput = open(input, O_RDONLY)) < 0) { //Abre ficheiro de input.
            perror("[transform] Erro ao abrir ficheiro input");
            return -1;
        }
        dup2(exInput, 0); //Coloca o stdin no ficheiro de input.
        close(exInput);
        int outp;
        if ((outp = open(output, O_CREAT | O_TRUNC | O_WRONLY)) < 0) { //Cria ficheiro de output.
            perror("[transform] Erro ao criar ficheiro de output");
            return -1;
        }
        dup2(outp, 1); //Coloca o stdout no ficheiro de output.
        close(outp);
        if (execvp(*argumentos, argumentos) == -1) {
            perror("[transform] Erro em execvp");
            return -1;
        }
        _exit(0);
    }
    free(args);
    return 0;
}

int executaProc2(char *comando) {
    char *found;
    char *argumentos;
    char *args = strdup(comando);

    found = strsep(&args, " ");

    char *input = strsep(&args, " "); //Guarda o nome e path do ficheiro de input.
    char *output = strsep(&args, " "); //Guarda nome e path do ficheiro de output.
    char *resto = strsep(&args, "\n"); //Guarda os filtros pedidos pelo utilizador.


    pid_t pid;
    pid = fork();
    if(pid == 0){
        int exInput;
        if ((exInput = open(input, O_RDONLY)) < 0) { //Abre ficheiro de input.
            perror("[transform] Erro ao abrir ficheiro input");
            return -1;
        }
        dup2(exInput, 0); //Coloca o stdin no ficheiro de input.
        close(exInput);

        int outp;
        if ((outp = open(output, O_CREAT | O_TRUNC | O_WRONLY)) < 0) { //Cria ficheiro de output.
            perror("[transform] Erro ao criar ficheiro de output");
            return -1;
        }
        dup2(outp, 1); //Coloca o stdout no ficheiro de output.
        close(outp);


        argumentos = ("bin/SDStore-transf/bcompress");

        execvp(*argumentos,argumentos);

        _exit(0);
    } 

    return 0;
}



int main(int argc, char *argv[]) {

    if(argc < 3) perror("falta argumentos ");

    
    int maxnop, maxbcompress, maxbdecompress, maxgcompress, maxgdecompress, maxencrypt, maxdencrypt = 0;

    char buffer[1024];
    int n;
    int fd_conf = open(argv[1],O_RDONLY);
    if(fd_conf == -1) perror("Erro no .conf");

    while((n = read(fd_conf,buffer,sizeof(buffer))) > 0) {
        char *token = strtok(buffer, "\n");  // breaks buffer into a series of tokens using the delimiter \n
        do {
            char *aux = strdup(token); //copia a string token para aux
            char *found = strsep(&aux, " "); // seleciona a string de aux até ao separador " "

            if(strcmp(found,"nop") == 0) {
                //nop_f = strdup(strsep(&aux, " "));
                maxnop = atoi(strsep(&aux, "\n"));
            }

            else if(strcmp(found,"bcompress") == 0) {
                //bcompress_f = strdup(strsep(&aux, " "));
                maxbcompress = atoi(strsep(&aux, "\n"));
            }        

            else if(strcmp(found,"bdecompress") == 0) {
                //bdecompress_f = strdup(strsep(&aux, " "));
                maxbdecompress = atoi(strsep(&aux, "\n"));
            }
            
            else if(strcmp(found,"gcompress") == 0) {
                //gcompress_f = strdup(strsep(&aux, " "));
                maxgcompress = atoi(strsep(&aux, "\n"));
            }  

            else if(strcmp(found,"gdecompress") == 0) {
                //gcompress_f = strdup(strsep(&aux, " "));
                maxgdecompress = atoi(strsep(&aux, "\n"));
            }        

            else if(strcmp(found,"encrypt") == 0) {
                //encrypt_f = strdup(strsep(&aux, " "));
                maxencrypt = atoi(strsep(&aux, "\n"));
            }
            
            else {
                //decrypt_f = strdup(strsep(&aux, " "));
                maxdencrypt = atoi(strsep(&aux, "\n"));
            } 
            free(aux);
        } while((token = strtok(NULL,"\n")));
    }
    dir = strcat(strdup(argv[2]), "/");
    write(1, "Servidor iniciado com sucesso!\n", strlen("Servidor iniciado com sucesso!\n"));

    close(fd_conf);


    //cria os named pipes
    int client_server_fifo;
    int server_client_fifo;
    int processing_fifo;

    if(mkfifo("/tmp/client_server_fifo",0600) == -1) {
        perror("Named pipe 1 error");
    }

    if(mkfifo("/tmp/server_client_fifo",0600) == -1) {
        perror("Named pipe 2 error");
    }

    if(mkfifo("/tmp/processing_fifo",0600) == -1) {
        perror("Pipe Fifo error");
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

    processing_fifo = open("/tmp/processing_fifo",O_WRONLY);
    if(processing_fifo == -1) perror("Erro fifo processing_fifo");

    //ler o pipe que vem do cliente
    int leitura = read(client_server_fifo,comando,sizeof(comando));
    if(leitura == -1) perror("Erro no read");

    while(1) {
        if(leitura > 0 && (strncmp(comando,"status",leitura) == 0)) {
            //printf("Pipe lido! \n");
            char mensagem[5000];
            char res[5000];

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

        else if(leitura > 0 && (strncmp(comando,"proc-file",9) == 0)) {
            write(processing_fifo, "Pending...\n", strlen("Pending...\n"));
            executaProc2(comando);
            write(processing_fifo, "Processing...\n", strlen("Processing...\n"));
        }

        unlink("/tmp/server_client_fifo");
        unlink("/tmp/client_server_fifo");
        unlink("/tmp/processing_fifo");
        printf("Server close\n");
    }
}