#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>
#include <signal.h>

// CLIENT

// ./bin/sdstore status

void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[]){
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void term_handler() {
    char pid[50];
    itoa(getpid(), pid);

    char pid_escrever[strlen(pid)+6];
    strcpy(pid_escrever, "/tmp/w");
    strcpy(pid_escrever+6,pid);

    char pid_ler[strlen(pid)+6];
    strcpy(pid_escrever, "/tmp/r");
    strcpy(pid_escrever+6,pid);

    unlink(pid_ler);
    unlink(pid_escrever);
    _exit(0);
}

int main(int argc, char *argv[]) {
    

    if(strcmp(argv[1],"status") == 0) {
        printf("[DEGUG] Status lido! \n");

        //Criar pipes para o cliente
        char pid[50];
        itoa(getpid(), pid);

        printf("Pid Debug %lu\n",getpid());

        char pid_escrever[strlen(pid)+6];
        strcpy(pid_escrever, "/tmp/w");
        strcpy(pid_escrever+6,pid);

        char pid_ler[strlen(pid)+6];
        strcpy(pid_ler, "/tmp/r");
        strcpy(pid_ler+6,pid);


        if (mkfifo(pid_ler, 0666) == 0 && mkfifo(pid_escrever, 0666) == 0) {
            if (signal(SIGINT, term_handler) == SIG_ERR) {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal1");
                _exit(-1);
            }
            if (signal(SIGTERM, term_handler) == SIG_ERR) {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal2");
                _exit(-1);
            }

            int pipePrincipal = open("/tmp/main", O_WRONLY);

            if (pipePrincipal == -1) {
                perror("Server Offline - Erro ao abrir pipe main");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            //Escrever pid do cliente para o pipe main
            for (int i = 0; i < strlen(pid); i++)
                write(pipePrincipal, pid+i, 1);
            close(pipePrincipal);

            int pipe_escrever = open(pid_escrever, O_WRONLY);

            if (pipe_escrever == -1) {
                perror("Erro ao abrir pipe_escrever");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            write(pipe_escrever, "status", 6);
            close(pipe_escrever);


            //obter resposta
            char buffer;
            int pipe_ler = open(pid_ler, O_RDONLY);

            if (pipe_ler == -1) {
                perror("Erro ao abrir pipe_ler");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            while (read(pipe_ler, &buffer, 1) > 0) {
                write(1, &buffer, 1);
            }

            close(pipe_ler);
            
            unlink(pid_ler);
            unlink(pid_escrever);
            printf("[DEBUG] Unlinks done\n");
        } else {
            perror("Mkfifo status");
            unlink(pid_ler);
            unlink(pid_escrever);
            _exit(-1);
        }
    }

    if(strcmp(argv[1],"proc-file") == 0) {
        // ./sdstore proc-file priority input-filename output-filename transformation-id-1 transformation-id-2

        //Criar pipes para o cliente
        char pid[50];
        itoa(getpid(), pid);

        printf("Pid Debug %lu\n",getpid());
        
        char pid_escrever[strlen(pid)+6];
        strcpy(pid_escrever, "/tmp/w");
        strcpy(pid_escrever+6,pid);

        char pid_ler[strlen(pid)+6];
        strcpy(pid_ler, "/tmp/r");
        strcpy(pid_ler+6,pid);

        if (mkfifo(pid_ler, 0666) == 0 && mkfifo(pid_escrever, 0666) == 0) {
            if (signal(SIGINT, term_handler) == SIG_ERR) {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal1");
                _exit(-1);
            }
            if (signal(SIGTERM, term_handler) == SIG_ERR) {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal2");
                _exit(-1);
            }

            int pipePrincipal = open("/tmp/main", O_WRONLY);
        
            if (pipePrincipal == -1) {
                perror("Server Offline - Erro ao abrir pipe");
                unlink(pid_escrever);
                _exit(-1);
            }

            //Escrever pid do cliente para o pipe main
            for (int i = 0; i < strlen(pid); i++)
                write(pipePrincipal, pid+i, 1);
            close(pipePrincipal);

            int pipe_escrever = open(pid_escrever, O_WRONLY);

            if (pipe_escrever == -1) {
                perror("Erro ao abrir pipe_escrever");
                unlink(pid_escrever);
                _exit(-1);
            }

            //Escever comando para o pipe
            char mensagem[5000];
            char res[5000];
            res[0] = 0; 

            for(int cont = 1; cont<argc;cont++){
                sprintf(mensagem,argv[cont]);
                strcat(res,mensagem);
                strcat(res," ");
            }

            write(pipe_escrever,res,strlen(res));
            close(pipe_escrever);

            //obter resposta
            char buffer;
            int pipe_ler = open(pid_ler, O_RDONLY);

            if (pipe_ler == -1) {
                perror("Erro ao abrir pipe_ler");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            while (read(pipe_ler, &buffer, 1) > 0) {
                write(1, &buffer, 1);
            }

            close(pipe_ler);
            unlink(pid_ler);
            unlink(pid_escrever);
            printf("[DEBUG] Unlinks done\n");
        }

    }
}

