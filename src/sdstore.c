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

void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0) /* record sign */
        n = -n;         /* make n positive */
    i = 0;
    do
    {                          /* generate digits in reverse order */
        s[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);   /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

// Handler do sinal SIGTERM e SIGINT
void term_handler()
{
    char pid[50];
    itoa(getpid(), pid);

    char pid_escrever[strlen(pid) + 6];
    strcpy(pid_escrever, "/tmp/w");
    strcpy(pid_escrever + 6, pid);

    char pid_ler[strlen(pid) + 6];
    strcpy(pid_escrever, "/tmp/r");
    strcpy(pid_escrever + 6, pid);

    unlink(pid_ler);
    unlink(pid_escrever);
    _exit(0);
}

int main(int argc, char *argv[])
{

    /*
    Caso o cliente escreva no stdin "status"
    */
    if (strcmp(argv[1], "status") == 0)
    {

        char pid[50];

        // Copiamos o pid do cliente para o array pid
        itoa(getpid(), pid);

        // Criamos um array pid_escrever onde vai conter "/tmp/w" +  o pid do cliente
        char pid_escrever[strlen(pid) + 6];
        strcpy(pid_escrever, "/tmp/w");
        strcpy(pid_escrever + 6, pid);

        // Criamos um array pid_ler onde vai conter "/tmp/r" +  o pid do cliente
        char pid_ler[strlen(pid) + 6];
        strcpy(pid_ler, "/tmp/r");
        strcpy(pid_ler + 6, pid);

        // Criamos dois pipes com nome dos array pid_escrever e pid_ler
        if (mkfifo(pid_ler, 0666) == 0 && mkfifo(pid_escrever, 0666) == 0)
        {
            // Sinais
            if (signal(SIGINT, term_handler) == SIG_ERR)
            {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal1");
                _exit(-1);
            }
            if (signal(SIGTERM, term_handler) == SIG_ERR)
            {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal2");
                _exit(-1);
            }

            // Abrimos o pipe principal do servidor
            int pipePrincipal = open("/tmp/main", O_WRONLY);

            if (pipePrincipal == -1)
            {
                perror("Server Offline - Erro ao abrir pipe main");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            // Escreve pid do cliente para o pipe main
            for (int i = 0; i < strlen(pid); i++)
                write(pipePrincipal, pid + i, 1);
            close(pipePrincipal);

            // Abre o pipe_escrever para escrita
            int pipe_escrever = open(pid_escrever, O_WRONLY);

            if (pipe_escrever == -1)
            {
                perror("Erro ao abrir pipe_escrever");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            // Escrevemos no pipe_escrever "status"
            write(pipe_escrever, "status", 6);
            close(pipe_escrever);

            /*
            Resposta do servidor
            */
            char buffer;

            // Abre o pipe_ler para leitura
            int pipe_ler = open(pid_ler, O_RDONLY);

            if (pipe_ler == -1)
            {
                perror("Erro ao abrir pipe_ler");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            // Vamos ler do pipe e escrever o seu conteudo para o stdin
            while (read(pipe_ler, &buffer, 1) > 0)
            {
                write(1, &buffer, 1);
            }

            close(pipe_ler);

            // No fim fazemos unlink dos pipes criados para o cliente
            unlink(pid_ler);
            unlink(pid_escrever);
        }
        else
        {
            perror("Mkfifo status");
            unlink(pid_ler);
            unlink(pid_escrever);
            _exit(-1);
        }
    }

    /*
    Caso o cliente escreva no stdin "proc-file"
    */
    if (strcmp(argv[1], "proc-file") == 0)
    {
        // ./sdstore proc-file priority input-filename output-filename transformation-id-1 transformation-id-2

        char pid[50];

        // Copiamos o pid do cliente para o array pid
        itoa(getpid(), pid);

        // Criamos um array pid_escrever onde vai conter "/tmp/w" +  o pid do cliente
        char pid_escrever[strlen(pid) + 6];
        strcpy(pid_escrever, "/tmp/w");
        strcpy(pid_escrever + 6, pid);

        // Criamos um array pid_ler onde vai conter "/tmp/r" +  o pid do cliente
        char pid_ler[strlen(pid) + 6];
        strcpy(pid_ler, "/tmp/r");
        strcpy(pid_ler + 6, pid);

        // Criamos dois pipes com nome dos array pid_escrever e pid_ler
        if (mkfifo(pid_ler, 0666) == 0 && mkfifo(pid_escrever, 0666) == 0)
        {
            // Sinais
            if (signal(SIGTERM, term_handler) == SIG_ERR)
            {
                unlink(pid_ler);
                unlink(pid_escrever);
                perror("Signal2");
                _exit(-1);
            }

            int pipePrincipal = open("/tmp/main", O_WRONLY);

            if (pipePrincipal == -1)
            {
                perror("Server Offline - Erro ao abrir pipe");
                unlink(pid_escrever);
                _exit(-1);
            }

            // Escrever pid do cliente para o pipe main
            for (int i = 0; i < strlen(pid); i++)
                write(pipePrincipal, pid + i, 1);
            close(pipePrincipal);

            // Abrir o pipe principal do servidor
            int pipe_escrever = open(pid_escrever, O_WRONLY);

            if (pipe_escrever == -1)
            {
                perror("Erro ao abrir pipe_escrever");
                unlink(pid_escrever);
                _exit(-1);
            }

            // Escever comando para o pipe_escrever
            char mensagem[1024];
            char res[1024];
            res[0] = 0;

            for (int cont = 1; cont < argc; cont++)
            {
                sprintf(mensagem, argv[cont]);
                strcat(res, mensagem);
                strcat(res, " ");
            }

            write(pipe_escrever, res, strlen(res));
            close(pipe_escrever);

            /*
            Resposta do servidor
            */
            char buffer;

            // Abre o pipe_ler para leitura
            int pipe_ler = open(pid_ler, O_RDONLY);

            if (pipe_ler == -1)
            {
                perror("Erro ao abrir pipe_ler");
                unlink(pid_ler);
                unlink(pid_escrever);
                _exit(-1);
            }

            // Ler do pipe e escrever o seu conteudo para o stdin
            while (read(pipe_ler, &buffer, 1) > 0)
            {
                write(1, &buffer, 1);
            }

            close(pipe_ler);

            /*
            Obter bytes dos ficheiros input e output
            */
            long sizeI = 0;
            long sizeO = 0;
            char *bufferBytes[1024];

            // Copiar diretoria do input e output
            if (strcmp(argv[2], "-p") == 0)
            {
                int fdI = open(argv[4], O_RDONLY);
                int fdO = open(argv[5], O_RDONLY);

                int n;
                while ((n = read(fdI, bufferBytes, 1)) > 0)
                {
                    sizeI++;
                }

                close(fdI);

                while ((n = read(fdO, bufferBytes, 1)) > 0)
                {
                    sizeO++;
                }

                close(fdO);
            }
            else
            {
                int fdI = open(argv[2], O_RDONLY);
                int fdO = open(argv[3], O_RDONLY);

                int n;
                while ((n = read(fdI, bufferBytes, 1)) > 0)
                {
                    sizeI++;
                }

                close(fdI);

                int k;
                while ((k = read(fdO, bufferBytes, 1)) > 0)
                {
                    sizeO++;
                }

                close(fdO);
            }

            char *mensagemBytes = malloc(sizeof(sizeI) * sizeof(sizeO) + 50);
            sprintf(mensagemBytes, "Concluded (bytes-input: %ld, bytes-output: %ld)\n", sizeI, sizeO);
            write(1, mensagemBytes, strlen(mensagemBytes) + 1);

            free(mensagemBytes);

            // Unlink dos pipes criados para o cliente
            unlink(pid_ler);
            unlink(pid_escrever);
        }
    }
}
