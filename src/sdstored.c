#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> 
#include <sys/wait.h>

// SERVER

// $ ./bin/sdstored etc/sdstored.conf bin/sdstore-transformations

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

    //debug
    printf("%d \n", maxnop);
    printf("%d \n", maxbcompress);
    printf("%d \n", maxbdecompress);
    printf("%d \n", maxgcompress);
    printf("%d \n", maxgdecompress);
    printf("%d \n", maxencrypt);
    printf("%d \n", maxdencrypt);


}