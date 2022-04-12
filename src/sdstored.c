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

int maxnop, maxbcompress, maxbdecompress, maxgcompress, maxgdecompress, maxencrypt, maxdecrypt;
int nop_cur, bcompress_cur, bdecompress_cur, gcompress_cur, gdecompress_cur, encrypt_cur, decrypt_cur;
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
                nop_f = strdup(strsep(&aux, " "));
                maxnop = atoi(strsep(&aux, "\n"));
            }

            else if(strcmp(found,"bcompress") == 0) {
                bcompress_f = strdup(strsep(&aux, " "));
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
                maxdecrypt = atoi(strsep(&aux, "\n"));
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
            sprintf(mensagem, "pid: %d\n", getpid());
            strcat(res, mensagem);
            strcat(res,"\0");
            write(server_client_fifo,res,strlen(res));
        }

        else if(leitura > 0 && (strncmp(comando,"proc-file",9) == 0)) {
            write(processing_fifo, "Pending...\n", strlen("Pending...\n"));

            char *comand;  
            comand = ("bin/sdstore-transf/bcompress");

            int input_f;
            input_f = open("samples/teste.txt", O_RDONLY);
            if(input_f == -1) perror("Erro no open 1");

            dup2(input_f,0);
            close(input_f);

            int output_f;
            output_f = open("output/output.txt", O_CREAT | O_TRUNC | O_WRONLY);
            if(output_f == -1) perror("Erro no open 2");

            dup2(output_f, 1);
            close(output_f);

            write(processing_fifo, "Processing...\n", strlen("Processing...\n"));

            execvp(comand,&comand);
            //execlp("ls","ls","-l",NULL);


        }

        unlink("/tmp/server_client_fifo");
        unlink("/tmp/client_server_fifo");
        unlink("/tmp/processing_fifo");
        printf("Server close\n");
    }
}