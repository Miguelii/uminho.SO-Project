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

// $ ./bin/sdstored etc/sdstored.conf bin/sdstore-transf

// ----
// Estruturas de dados
// ----
// Estrutura de um processo a ser guardado
typedef struct process
{
    char *pid;         // PID do processo
    char *comando;     // Comando a executar
    int pipe_escrever; // Pipe para escrever
    int prioridade;    // Prioridade do processo
} Process;

// Queue onde serão armazenados os processos em espera
typedef struct queue
{
    int first, last, size; // Primeiro, último e tamanho da queue
    int capacity;          // Capacidade máxima da queue
    Process *processes;    // Array de processos
} Queue;

// ----
// Funções de manipulação da queue
// ----
// Inicializa a queue
Queue *createQueue(int capacity)
{
    Queue *q = (Queue *)malloc(sizeof(Queue));
    q->first = 0;
    q->last = capacity - 1;
    q->size = 0;
    q->capacity = capacity;
    q->processes = (Process *)malloc(capacity * sizeof(Process));
    return q;
}

// Verifica se a Queue está cheia
int isFull(Queue *q)
{
    return q->size == q->capacity;
}

// Verifica se a Queue está vazio
int isEmpty(Queue *q)
{
    return q->size == 0;
}

// Adiciona um processo à queue
void enqueue(Queue *q, Process p)
{
    if (!isFull(q))
    {
        q->last = (q->last + 1) % q->capacity;
        q->processes[q->last] = p;
        q->size++;
    }
}

// Remove um processo da queue
Process dequeue(Queue *q)
{
    if (!isEmpty(q))
    {
        Process p = q->processes[q->first];
        q->first = (q->first + 1) % q->capacity;
        q->size--;
        return p;
    }
}

// função de ordenação de Queue por prioridade (maior prioridade primeiro)
void orderQueue(Queue *q)
{
    int i, j;
    for (i = q->first; i < q->size - 1; i++)
    {
        for (j = q->first; j < q->size - i - 1; j++)
        {
            if (q->processes[j].prioridade < q->processes[j + 1].prioridade)
            {
                Process aux = q->processes[j];
                q->processes[j] = q->processes[j + 1];
                q->processes[j + 1] = aux;
            }
        }
    }
}

// função de impressão de Queue
void printQueue(Queue *q)
{
    int i;
    for (i = q->first; i < q->size; i++)
    {
        printf("Pid: %s\n", q->processes[i].pid);
        printf("Comando: %s\n", q->processes[i].comando);
        printf("Pipe: %d\n", q->processes[i].pipe_escrever);
        printf("Prioridade: %d\n", q->processes[i].prioridade);
    }
}

// ----
// Headers
// ----
void term_handler(int signum);
void freeSlots(char *arg);
int check_disponibilidade(char *command);
int check_disponibilidadeMAX(char *command);
void updateSlots(char *arg);
int procfileAux(Process *p);
int procfile(char *pid, char *comando, int pipe_escrever);

// ----
// Variáveis globais
// ----
int nProcesses = 0;    // N de processos ativos
char *inProcess[1024]; // Processos em execução
Queue *q;              // Queue de processos
char *dir;             // Diretoria dos executáveis dos filtros.

// variaveis maximas de processos
int maxnop, maxbcompress, maxbdecompress, maxgcompress, maxgdecompress, maxencrypt, maxdecrypt;
// variaveis atuais de processos
int nop_cur, bcompress_cur, bdecompress_cur, gcompress_cur, gdecompress_cur, encrypt_cur, decrypt_cur;

// ----
// Funções
// ----

// Handler do sinal SIGTERM
void term_handler(int signum)
{

    unlink("/tmp/main");

    write(1, "\nA terminar servidor.\n", strlen("\nA terminar servidor.\n"));
    _exit(0);
}

// child_handler quando um filho termina
void child_handler(int signum)
{
    pid_t pid;
    int status;


    wait(&status);
    // while((pid = waitpid(-1, &status, WNOHANG)) > 0);

    if (nProcesses > 0)
    {
        freeSlots(inProcess[nProcesses - 1]);
    }

    // se a queue tiver elementos ordena a queue, depois dá dequeue do primeiro elemento (mais prioridade) e executa o comando
    if (q->size > 0)
    {
        orderQueue(q);
        //printf("\n\n--QUEUE--\n\n");
        printQueue(q);

        Process p = dequeue(q);
        procfileAux(&p);
    }
}

// Atualiza informação sobre filtros em uso.
void updateSlots(char *arg)
{
    char *dup = strdup(arg);
    char *tok;
    // Dependendo do comando inserido, atualiza as variáveis correspondentes aos filtros em uso.
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "nop"))
            nop_cur++;
        if (!strcmp(tok, "bcompress"))
            bcompress_cur++;
        if (!strcmp(tok, "bdecompress"))
            bdecompress_cur++;
        if (!strcmp(tok, "gcompress"))
            gcompress_cur++;
        if (!strcmp(tok, "gdecompress"))
            gdecompress_cur++;
        if (!strcmp(tok, "encrypt"))
            encrypt_cur++;
        if (!strcmp(tok, "decrypt"))
            decrypt_cur++;
    }
    free(dup);
}

// Atualiza informação sobre os filtros em uso
void freeSlots(char *arg)
{
    char *dup = strdup(arg);
    char *tokAux;
    // Funcionamento igual ao updateSlots() mas decrementa em vez de incrementar.
    while ((tokAux = strsep(&dup, " ")))
    {
        if (!strcmp(tokAux, "nop"))
        {
            nop_cur--;
            if (nop_cur < 0)
                nop_cur = 0;
        }
        if (!strcmp(tokAux, "bcompress"))
        {
            bcompress_cur--;
            if (bcompress_cur < 0)
                bcompress_cur = 0;
        }
        if (!strcmp(tokAux, "bdecompress"))
        {
            bdecompress_cur--;
            if (bdecompress_cur < 0)
                bdecompress_cur = 0;
        }
        if (!strcmp(tokAux, "gcompress"))
        {
            gcompress_cur--;
            if (gcompress_cur < 0)
                gcompress_cur = 0;
        }
        if (!strcmp(tokAux, "gdecompress"))
        {
            gdecompress_cur--;
            if (gdecompress_cur < 0)
                gdecompress_cur = 0;
        }
        if (!strcmp(tokAux, "encrypt"))
        {
            encrypt_cur--;
            if (encrypt_cur < 0)
                encrypt_cur = 0;
        }
        if (!strcmp(tokAux, "decrypt"))
        {
            decrypt_cur--;
            if (decrypt_cur < 0)
                decrypt_cur = 0;
        }
    }
    nProcesses--;
    free(dup);
}

// Retorna 0 se os filtros excederem os maximos permitidos, 1 caso contrário.
int check_disponibilidadeMAX(char *command)
{
    // verificar se as transormacoes excedem os filtros maximos
    int nop = 0;
    char *dup = strdup(command);
    char *tok;
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "nop"))
            nop++;
    }
    free(dup);
    if (nop > maxnop)
        return 0;

    int bcompress = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "bcompress"))
            bcompress++;
    }
    free(dup);
    if (bcompress > maxbcompress)
        return 0;

    int bdecompress = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "bdecompress"))
            bdecompress++;
    }
    free(dup);
    if (bdecompress > maxbdecompress)
        return 0;

    int gcompress = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "gcompress"))
            gcompress++;
    }
    free(dup);
    if (gcompress > maxgcompress)
        return 0;

    int gdecompress = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "gdecompress"))
            gdecompress++;
    }
    free(dup);
    if (gdecompress > maxgdecompress)
        return 0;

    int encrypt = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "encrypt"))
            encrypt++;
    }
    free(dup);
    if (encrypt > maxencrypt)
        return 0;

    int decrypt = 0;
    dup = strdup(command);
    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "decrypt"))
            decrypt++;
    }
    free(dup);
    if (decrypt > maxdecrypt)
        return 0;

    return 1;
}

// Retorna 1 se tivermos filtros disponiveis para executar a transformação.
// Basta um filtro não estar disponível e a função retorna 0 (0 -> não há disponibilidade para execução do comando).
int check_disponibilidade(char *command)
{
    char *comando = strdup(command);
    char *found;
    for (int i = 0; i < 2; i++)
    {
        found = strsep(&comando, " ");
    }
    found = strsep(&comando, " ");
    do
    {
        if (!strcmp(found, "nop"))
            if (nop_cur >= maxnop)
                return 0;
        if (!strcmp(found, "bcompress"))
            if (bcompress_cur >= maxbcompress)
                return 0;
        if (!strcmp(found, "bdecompress"))
            if (bdecompress_cur >= maxbdecompress)
                return 0;
        if (!strcmp(found, "gcompress"))
            if (gcompress_cur >= maxgcompress)
                return 0;
        if (!strcmp(found, "gdecompress"))
            if (gdecompress_cur >= maxgdecompress)
                return 0;
        if (!strcmp(found, "encrypt"))
            if (encrypt_cur >= maxencrypt)
                return 0;
        if (!strcmp(found, "decrypt"))
            if (decrypt_cur >= maxdecrypt)
                return 0;
    } while ((found = strsep(&comando, " ")) != NULL);
    free(comando);
    return 1;
}

/* Pega no array resto e substitui as transformaçoes para as respetivas diretorias
Por exemplo, se o array tiver:
nop bcompress
Fica:
/bin/sdstore-transf/nop /bin/sdstore-transf/bcompress
*/
char **setArgs(char *resto)
{
    int current = 0;
    char res[50];
    res[0] = 0;

    char **argumentos = (char **)calloc(100, sizeof(char *));

    char *dup = strdup(resto);
    char *tok;

    while ((tok = strsep(&dup, " ")))
    {
        if (!strcmp(tok, "bcompress"))
        {
            strcat(res, dir);
            strcat(res, "bcompress");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "encrypt"))
        {
            strcat(res, dir);
            strcat(res, "encrypt");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "nop"))
        {
            strcat(res, dir);
            strcat(res, "nop");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "decrypt"))
        {
            strcat(res, dir);
            strcat(res, "decrypt");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "bdecompress"))
        {
            strcat(res, dir);
            strcat(res, "bdecompress");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "gcompress"))
        {
            strcat(res, dir);
            strcat(res, "gcompress");
            argumentos[current++] = strdup(res);
        }
        if (!strcmp(tok, "gdecompress"))
        {
            strcat(res, dir);
            strcat(res, "gdecompress");
            argumentos[current++] = strdup(res);
        }
        memset(res, 0, sizeof res);
    }
    free(dup);

    argumentos[current] = NULL;

    return argumentos;
}

// Handler dos filhos
void fecharfilho(char *pid)
{
    //printf("Filho fechado!\n");

    kill(getpid(), SIGUSR1);
    kill(atoi(pid), SIGUSR2);

    _exit(0);
}

// Função para executar as transformações
void execs(int input, int output, char **argumentos)
{
    int i = 0;
    int pip[2];

    signal(SIGCHLD, SIG_DFL);

    while (argumentos[i] != NULL)
    {
        if (i != 0)
        {
            dup2(pip[0], 0);
            close(pip[0]);
        }
        else
        {
            dup2(input, 0);
            close(input);
        }

        if (argumentos[i + 1] == NULL)
        {
            dup2(output, 1);
            close(output);
        }
        else
        {
            if (pipe(pip) == 0)
            {
                dup2(pip[1], 1);
                close(pip[1]);
            }
            else
            {
                perror("Pipe");
                _exit(-1);
            }
        }

        int f;
        f = fork();
        if (f == -1)
        {
            perror("Erro fork execs");
            _exit(-1);
        }
        else if (f == 0)
        { // Executa todas as transformações pedidas pelo utilizador
            char *executavel = malloc(sizeof(char) * 1024);
            strcpy(executavel, argumentos[i]);
            execlp(executavel, executavel, NULL);
            perror("Exec");
            _exit(-1);
        }

        i++;
    }
}

// Função para executar as transformações
int monitor(char *input, char *output, char **argumentos, char *pid)
{

    int f = fork();
    if (f == -1)
    {
        perror("Fork");
        kill(atoi(pid), SIGUSR2);

        fecharfilho(pid);
    }
    else if (f == 0)
    {
        // Abre o ficheiro input fornecido pelo utilizador
        int input_f;
        input_f = open(input, O_RDONLY);
        if (input_f == -1)
        {
            perror("Erro no open input");
            _exit(-1);
        }

        // Abre o ficheiro output fornecido pelo utilizador
        int output_f;
        output_f = open(output, O_CREAT | O_TRUNC | O_WRONLY, 0666);
        if (output_f == -1)
        {
            perror("Erro no open output");
            _exit(-1);
        }

        sleep(5); // utilizado para conseguirmos executar o status com sentido caso contrário os comandos eram instantaneos e nao conseguiamos ver os status
        execs(input_f, output_f, argumentos);
        _exit(0);
    }
    else
    {
        int status;
        // kill(atoi(pid),SIGUSR1);

        wait(&status);
        fecharfilho(pid);
    }

    return 0;
}

// Função da opção de obter informação de utilização do servidor
void status(char *pid) {

    int pidServer = getpid();

    int f = fork();
    if (f == 0)
    {
        signal(SIGINT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);

        // Criamos um array pid_ler onde vai conter "/tmp/r" +  o pid do cliente recebido do pipe main
        char pid_escrever[strlen(pid) + 6];
        strcpy(pid_escrever, "/tmp/r");
        strcpy(pid_escrever + 6, pid);

        // Abre o pipe_escrever para escrita
        int pipe_escrever = open(pid_escrever, O_WRONLY);

        char mensagem[5000];
        char res[5000];
        res[0] = 0;

        for (int i = 0; i < nProcesses; i++)
        {
            sprintf(mensagem, "Task #%d: %s\n", i + 1, inProcess[i]);
            strcat(res, mensagem);
        }

        sprintf(mensagem, "Transf nop: %d/%d (Running/Max) \n", nop_cur, maxnop);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf bcompress: %d/%d (Running/Max) \n", bcompress_cur, maxbcompress);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf bdecompress: %d/%d (Running/Max) \n", bdecompress_cur, maxbdecompress);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf gcompress: %d/%d (Running/Max) \n", gcompress_cur, maxgcompress);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf gdecompress: %d/%d (Running/Max) \n", gdecompress_cur, maxgdecompress);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf encrypt: %d/%d (Running/Max) \n", encrypt_cur, maxencrypt);
        strcat(res, mensagem);
        sprintf(mensagem, "Transf decrypt: %d/%d (Running/Max) \n", decrypt_cur, maxdecrypt);
        strcat(res, mensagem);
        sprintf(mensagem, "pid: %d\n", pidServer);
        strcat(res, mensagem);
        strcat(res, "\0");
        write(pipe_escrever, res, strlen(res) + 1);

        close(pipe_escrever);

        _exit(0);
    }
}

// auxiliar que chama a procfile
int procfileAux(Process *p)
{
    procfile(p->pid, p->comando, p->pipe_escrever);
}

// Função da opção de executar uma ou várias transformações
int procfile(char *pid, char *comando, int pipe_escrever)
{

    // Criamos um array pid_ler onde vai conter "/tmp/r" +  o pid do cliente
    char pid_escrever[strlen(pid) + 6];
    strcpy(pid_escrever, "/tmp/r");
    strcpy(pid_escrever + 6, pid);

    // Abre o pipe_escrever para escrita
    if (pipe_escrever == -1)
    {
        pipe_escrever = open(pid_escrever, O_WRONLY);
        write(pipe_escrever, "Pending...\n", strlen("Pending...\n"));
    }

    char *auxComando;
    int prio = 0;

    // Verificar se o comando tem prioridade
    if (comando[10] == '-' && comando[11] == 'p')
    {

        char *args = strdup(comando);
        strsep(&args, " ");
        strsep(&args, " ");
        strsep(&args, " ");
        auxComando = strsep(&args, "\n");
        prio = atoi(&comando[13]);
    }
    else
    {
        char *args = strdup(comando);
        strsep(&args, " ");
        auxComando = strsep(&args, "\n");
        prio = 1; // Se nao tiver prioridade é atribuido o valor de 1
    }

    // verificar se o comando nao excede os maximos de filtros
    if (check_disponibilidadeMAX(strdup(auxComando)) == 0)
    {
        write(pipe_escrever, "A transformação pedida excede algum dos limites estabelecidos do Servidor. Verifique o status!\n", strlen("A transformação pedida excede algum dos limites estabelecidos do Servidor. Verifique o status!\n"));
        close(pipe_escrever);
        return -1;
    }

    // Verifica se temos filtros suficientes para executar o comando
    if (check_disponibilidade(strdup(auxComando)) == 1)
    {

        char *args = strdup(auxComando);
        char *input = strsep(&args, " ");  // Guarda o nome e path do ficheiro de input.
        char *output = strsep(&args, " "); // Guarda nome e path do ficheiro de output.
        char *resto = strsep(&args, "\n"); // Guarda os filtros pedidos pelo utilizador.

        // Guardar processo em execução
        inProcess[nProcesses++] = strdup(comando);

        // Aumenta o numero actual das transformações
        updateSlots(resto);

        // Set das diretorias das transformações
        char **argumentos = setArgs(resto);

        write(pipe_escrever, "Processing...\n", strlen("Processing...\n"));
        int f = fork();
        if (f == 0)
        {
            signal(SIGINT, SIG_IGN);
            signal(SIGTERM, SIG_IGN);
            monitor(input, output, argumentos, pid);
        }

        close(pipe_escrever);
    }
    else
    {   
        // Adicionar pedido à queue
        Process *p = malloc(sizeof(Process));
        p->pid = strdup(pid);
        p->comando = strdup(comando);
        p->pipe_escrever = pipe_escrever;
        p->prioridade = prio;

        enqueue(q, *p);
    }

    return 0;
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        perror("Falta argumentos ");
        return 0;
    }
    if (argc > 3)
    {
        perror("Muitos argumentos ");
        return 0;
    }

    maxnop = maxbcompress = maxbdecompress = maxgcompress = maxgdecompress = maxencrypt = maxdecrypt = 0;
    nop_cur = bcompress_cur = bdecompress_cur = gcompress_cur = gdecompress_cur = encrypt_cur = decrypt_cur = 0;

    char buffer[1024];
    int n;

    // Abrir a diretoria do ficheiro de configuração
    int fd_conf = open(argv[1], O_RDONLY);
    if (fd_conf == -1)
        perror("Erro no .conf");

    /*
    Ciclo para ler ficheiro de configuração e guardar o numero máximo de cada transformação
    */
    while ((n = read(fd_conf, buffer, sizeof(buffer))) > 0)
    {
        char *token = strtok(buffer, "\n");
        do
        {
            char *aux = strdup(token);       // copia a string token para aux
            char *found = strsep(&aux, " "); // seleciona a string de aux até ao separador " "

            if (strcmp(found, "nop") == 0)
            {
                maxnop = atoi(strsep(&aux, "\n"));
            }

            else if (strcmp(found, "bcompress") == 0)
            {
                maxbcompress = atoi(strsep(&aux, "\n"));
            }

            else if (strcmp(found, "bdecompress") == 0)
            {
                maxbdecompress = atoi(strsep(&aux, "\n"));
            }

            else if (strcmp(found, "gcompress") == 0)
            {
                maxgcompress = atoi(strsep(&aux, "\n"));
            }

            else if (strcmp(found, "gdecompress") == 0)
            {
                maxgdecompress = atoi(strsep(&aux, "\n"));
            }

            else if (strcmp(found, "encrypt") == 0)
            {
                maxencrypt = atoi(strsep(&aux, "\n"));
            }

            else
            {
                maxdecrypt = atoi(strsep(&aux, "\n"));
            }
            free(aux);
        } while ((token = strtok(NULL, "\n")));
    }

    // Copia para dir a diretoria da pasta das transformações
    dir = strcat(strdup(argv[2]), "/");

    write(1, "Servidor iniciado com sucesso!\n", strlen("Servidor iniciado com sucesso!\n"));

    // Fechar ficheiro de configuração
    close(fd_conf);

    // Iniciar a Queue
    q = createQueue(1024);

    // Criar o pipe com nome main
    if (mkfifo("/tmp/main", 0666) == -1)
    {
        perror("Mkfifo");
    }

    // Declaração dos handlers dos sinais.
    if (signal(SIGINT, term_handler) == SIG_ERR)
    {
        perror("[signal] erro da associação do signint_handler.");
        exit(-1);
    }
    // child_handler
    // SIG_IGN
    if (signal(SIGCHLD, child_handler) == SIG_ERR)
    {
        perror("[signal] erro da associação do sigchld_handler.");
        exit(-1);
    }
    if (signal(SIGTERM, term_handler) == SIG_ERR)
    {
        perror("[signal] erro da associação do sigterm_handler.");
        exit(-1);
    }

    char comando[1024];

    // Abrir pipe main para leitura
    int pipe = open("/tmp/main", O_RDONLY);
    if (pipe == -1)
        perror("/tmp/main");

    // Setup da função poll()
    struct pollfd *pfd = calloc(1, sizeof(struct pollfd));
    pfd->fd = pipe;
    pfd->events = POLLIN;
    pfd->revents = POLLOUT;

    int leitura = 0;

    while (1)
    {
        char pid[50];
        int res = 0;

        // Read do pipe main para guardar o pid do cliente
        while (read(pipe, pid + res, 1) > 0)
        {
            res++;
        }
        pid[res++] = '\0';

        // Criamos um array pid_ler_cliente onde vai conter "/tmp/w" +  o pid do cliente
        char pid_ler_cliente[strlen(pid) + 6];
        strcpy(pid_ler_cliente, "/tmp/w");
        strcpy(pid_ler_cliente + 6, pid);
        res = 0;

        // Abre o pipe_ler_cliente para escrita
        int pipe_ler_cliente = open(pid_ler_cliente, O_RDONLY);

        // Read do pipe_ler_cliente
        leitura = read(pipe_ler_cliente, comando, sizeof(comando));

        // Handling de erro no caso de ocorrer algum problema na leitura.
        // if(leitura == -1) perror("Erro no read");

        comando[leitura] = 0;

        /*
        Caso o primeiro argumento lido do pipe_ler_cliente seja "status"
        */
        if (leitura > 0 && (strncmp(comando, "status", leitura) == 0))
        {
            status(pid);
            // adicionar processo do status ao array de processos atual, para quando ele morrer não limpar outros processos e limpa o dele
            inProcess[nProcesses++] = strdup(comando);
        }

        /*
        Caso o primeiro argumento lido do pipe_ler_cliente seja "proc-file"
        */
        else if (leitura > 0 && (strncmp(comando, "proc-file", 9) == 0))
        {
            // procfile(q, pid, comando);

            procfile(pid, comando, -1);
        }
    }
}
