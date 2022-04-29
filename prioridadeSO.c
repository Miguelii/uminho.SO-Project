/*
  FICHEIRO PARA TESTES 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct queue {
    char **line; //Comando em espera.
    int filled; //Inidice da cauda.
    int pos; //Proximo processo em execução.
    int *pri; //prioridade
} Queue;

void push(Queue *q, int length);

Queue *initQueue () {
    Queue *fila = calloc(1, sizeof(struct queue));
    fila->line = (char **) calloc(100, sizeof(char *));
    fila->pri = (int *) calloc(20, sizeof(int));
    fila->pos = 0;
    fila->filled = -1;
    return fila;
}

void push(Queue *q, int length) {

  int temp = 0;
  
  for (int i = 0; i < length; i++) {     
    for (int j = i+1; j < length; j++) {     
       if(q->pri[i] < q->pri[j]) { 
         
          //Swap no array dos comandos
          Queue *temp = q;
          temp = q->line[i];
          q->line[i] = q->line[j];
          q->line[j] = temp;
          
          //Swap no array das prioridades
          temp = q->pri[i];    
          q->pri[i] = q->pri[j];    
          q->pri[j] = temp;    
       }     
    }     
  } 
}


int main() {
  
  Queue *q = initQueue();
  
  char *auxComando = "Cenas 5";
  char *auxComando2 = "Cenas 4";
  char *auxComando3 = "Cenas 1";
  
  q->line[++q->filled] = strdup(auxComando);
  q->pri[q->filled] = 5;
  
  q->line[++q->filled] = strdup(auxComando2);
  q->pri[q->filled] = 4;
  
  q->line[++q->filled] = strdup(auxComando3);
  q->pri[q->filled] = 1;
  
  push(q,q->filled+1);
  return 0;
}