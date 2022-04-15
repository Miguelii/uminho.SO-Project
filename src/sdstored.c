#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>

// SERVER

// $ ./bin/sdstored etc/sdstored.conf bin/sdstore-transformations
void freeSlots(char *arg);
int check_disponibilidade (char *command);

typedef struct queue {
    char **line; //Comando em espera.
    int filled; //Inidice da cauda.
    int pos; //Proximo processo em execução.
} Queue;

Queue *initQueue () {
    Queue *fila = calloc(1, sizeof(struct queue));
    fila->line = (char **) calloc(100, sizeof(char *));
    fila->pos = 0;
    fila->filled = -1;
    return fila;
}

int canQ (Queue *q) {
    if (q->filled >= 0 && q->pos <= q->filled) {
        if (check_disponibilidade(q->line[q->pos])) return 1;
    }
    return 0;
}
//Contem nome dos executaveis de cada proc-file
int maxnop, maxbcompress, maxbdecompress, maxgcompress, maxgdecompress, maxencrypt, maxdecrypt;
int nop_cur, bcompress_cur, bdecompress_cur, gcompress_cur, gdecompress_cur, encrypt_cur, decrypt_cur;

int nProcesses = 0; //N de processos ativos
char *inProcess[1024]; //Processos em execução

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

void sigterm_handler (int signum) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) wait(NULL); //Termina todos os filhos.

    //Remove os ficheiros temporários criados durante a execução.
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
//Handler do sinal SIGCHLD
void sigchld_handler(int signum) {
    char *tok = strdup(inProcess[--nProcesses]); //Guarda o comando (Alterações necessárias aqui)
    strsep(&tok, " ");
    strsep(&tok, " ");
    strsep(&tok, " ");
    char *resto = strsep(&tok, "\n");
    freeSlots(resto); //Coloca os filtros como novamente disponíveis.
    free(tok);
}

//Atualiza informação sobre filtros em uso.
 void updateSlots(char *arg) {
    char *dup = strdup(arg);
    char *tok;
    //Dependendo do comando inserido, atualiza as variáveis correspondentes aos filtros em uso.
    while((tok = strsep(&dup, " "))) {
        if (!strcmp(tok, "nop")) nop_cur++;
        if (!strcmp(tok, "bcompress")) bcompress_cur++;
        if (!strcmp(tok, "bdecompress")) bdecompress_cur++;
        if (!strcmp(tok, "gcompress")) gcompress_cur++;
        if (!strcmp(tok, "gdecompress")) gdecompress_cur++;
        if (!strcmp(tok, "encrypt")) encrypt_cur++;
        if (!strcmp(tok, "decrypt")) decrypt_cur++;
    }
    free(dup);
}

//Atualiza informação sobre os filtros em uso
void freeSlots(char *arg) {
    char *dup = strdup(arg);
    char *tok;
    //Funcionamento igual ao updateSlots() mas decrementa em vez de incrementar.
    while((tok = strsep(&dup, " "))) {
        if (!strcmp(tok, "nop")) nop_cur--;
        if (!strcmp(tok, "bcompress")) bcompress_cur--;
        if (!strcmp(tok, "bdecompress")) bdecompress_cur--;
        if (!strcmp(tok, "gcompress")) gcompress_cur--;
        if (!strcmp(tok, "gdecompress")) gdecompress_cur--;
        if (!strcmp(tok, "encrypt")) encrypt_cur--;
        if (!strcmp(tok, "decrypt")) decrypt_cur--;
    }
    free(dup);
}

//Retorna 1 se tivermos filtros disponiveis para executar a transformação.
int check_disponibilidade (char *command) {
    char *comando = strdup(command);
    char *found;
    for (int i = 0; i < 3; i++) {
        found = strsep(&comando, " ");
    }
    found = strsep(&comando, " ");
    //Basta um filtro não estar disponível e a função retorna 0 (0 -> não há disponibilidade para execução do comando).
    do {
        if (!strcmp(found, "nop")) if (nop_cur >= maxnop) return 0;
        if (!strcmp(found, "bcompress")) if (bcompress_cur >= maxbcompress) return 0;
        if (!strcmp(found, "bdecompress")) if (bdecompress_cur >= maxbdecompress) return 0;
        if (!strcmp(found, "gcompress")) if (gcompress_cur >= maxgcompress) return 0;
        if (!strcmp(found, "gdecompress")) if (gdecompress_cur >= maxgdecompress) return 0;
        if (!strcmp(found, "encrypt")) if (encrypt_cur >= maxencrypt) return 0;
        if (!strcmp(found, "decrypt")) if (decrypt_cur >= maxdecrypt) return 0;
    } while ((found = strsep(&comando, " ")) != NULL);
    free(comando);
    return 1;
}


int executaProc(char *comando) {
    char *found;
    char *args = strdup(comando);
    found = strsep(&args, " ");
    int status;

    char *input = strsep(&args, " "); //Guarda o nome e path do ficheiro de input.
    char *output = strsep(&args, " "); //Guarda nome e path do ficheiro de output.
    char *resto = strsep(&args, "\n"); //Guarda os filtros pedidos pelo utilizador.

    inProcess[nProcesses++] = strdup(comando);
    char *argumentos;
    updateSlots(resto);
    
    char *tokens = strtok(resto, "\n");
    do {
        char *aux = strdup(tokens);
        char *argumento = strsep(&aux, " ");
        
        if(strcmp(argumento,"bcompress") == 0) {
            argumentos = ("bin/sdstore-transf/bcompress");
        }
        else if(strcmp(argumento,"nop") == 0) {
            argumentos = ("bin/sdstore-transf/nop");
        }
        else {
            argumentos = ("bin/sdstore-transf/bdecompress");
        } 

    } while((tokens = strtok(NULL,"\n")));

    //printf("%s \n",argumentos);

    pid_t pid;
    pid = fork();

    if(pid == 0) {
        int input_f;
        input_f = open(input, O_RDONLY);
        if(input_f == -1) perror("Erro no open input");

        dup2(input_f,0);
        close(input_f);

        int output_f;
        output_f = open(output, O_CREAT | O_TRUNC | O_WRONLY);
        if(output_f == -1) perror("Erro no open output");

        dup2(output_f, 1);
        close(output_f);

        execvp(argumentos,&argumentos);

        _exit(0);

    }

    free(args);

    return 0;
}

int main(int argc, char *argv[]) {

    if(argc < 3) perror("Falta argumentos ");
    if(argc > 3) perror("Muitos argumentos ");

    
    maxnop = maxbcompress = maxbdecompress = maxgcompress = maxgdecompress = maxencrypt = maxdecrypt = 0;
    nop_cur = bcompress_cur = bdecompress_cur = gcompress_cur = gdecompress_cur = encrypt_cur = decrypt_cur = 0;

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
                maxdecrypt = atoi(strsep(&aux, "\n"));
            } 
            free(aux);
        } while((token = strtok(NULL,"\n")));
    }

    write(1, "Servidor iniciado com sucesso!\n", strlen("Servidor iniciado com sucesso!\n"));

    close(fd_conf);


    //cria os named pipes
    int client_server_fifo;
    int server_client_fifo;
    int processing_fifo;
    Queue *q = initQueue();

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
    if (signal(SIGCHLD, sigchld_handler) == SIG_ERR) {
        perror("[signal] erro da associação do sigchld_handler.");
        exit(-1);
    }
    if (signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        perror("[signal] erro da associação do sigterm_handler.");
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


    //Setup da função poll()
    struct pollfd *pfd = calloc(1, sizeof(struct pollfd));
    pfd->fd = client_server_fifo;
    pfd->events = POLLIN;
    pfd->revents = POLLOUT;

    int leitura = 0;

    while(1) {

        if (canQ(q) == 1) { //Verifica sempre se pode executar o que esta na fila.
            //Caso de poder executar a fila, usa mesmo código do transform que esta mais em baixo.
            char *comandoQ = strdup(q->line[q->pos]);
            q->pos++;
            write(processing_fifo, "Processing...\n", strlen("Processing...\n")); //informa o cliente que o pedido começou a ser processado.
            executaProc(comandoQ);
        }
        //Execução bloqueada até ser lida alguma coisa no pipe. Diminui utilização de CPU. 
        else {
            poll(pfd, 1, -1); //Verifica se o pipe está disponivel para leitura
        }

        leitura = read(client_server_fifo,comando,sizeof(comando));
        //Handling de erro no caso de ocorrer algum problema na leitura.
        if(leitura == -1) perror("Erro no read");        
        
        comando[leitura] = 0;

        if(leitura > 0 && (strncmp(comando,"status",leitura) == 0)) {
            //printf("Pipe lido! \n");
            char mensagem[5000];
            char res[5000];
            res[0] = 0;

            for (int i = 0; i < nProcesses; i++) {
                sprintf(mensagem, "Task #%d: %s\n", i+1, inProcess[i]);
                strcat(res, mensagem);
            }

            sprintf(mensagem, "Transf nop: %d/%d (Running/Max) \n",nop_cur,maxnop);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf bcompress: %d/%d (Running/Max) \n",bcompress_cur,maxbcompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf bdecompress: %d/%d (Running/Max) \n",bdecompress_cur,maxbdecompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf gcompress: %d/%d (Running/Max) \n",gcompress_cur,maxgcompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf gdecompress: %d/%d (Running/Max) \n",gdecompress_cur,maxgdecompress);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf encrypt: %d/%d (Running/Max) \n",encrypt_cur,maxencrypt);
            strcat(res,mensagem);
            sprintf(mensagem, "Transf decrypt: %d/%d (Running/Max) \n",decrypt_cur,maxdecrypt);
            strcat(res,mensagem);
            sprintf(mensagem, "pid: %d\n", getpid());
            strcat(res, mensagem);
            strcat(res,"\0");
            write(server_client_fifo,res,strlen(res));
        }

        else if(leitura > 0 && (strncmp(comando,"proc-file",9) == 0)) {
            write(processing_fifo, "Pending...\n", strlen("Pending...\n"));

            printf("%s \n",comando);

            if(check_disponibilidade(strdup(comando)) == 1) { //Verifica se temos filtros suficientes para executar o comando
                write(processing_fifo, "Processing...\n", strlen("Processing...\n")); //informa o cliente que o pedido começou a ser processado.
                executaProc(comando);
            }
            else {
                q->line[++q->filled] = strdup(comando);
            }
            
        }
    }
}