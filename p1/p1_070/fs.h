#ifndef FS_H
#define FS_H
#include <pthread.h>
#include "lib/bst.h"


typedef struct tecnicofs {
    node* bstRoot; //nó da raiz da arvore
    int nextINumber; //guarda o ultimo inumber atribuido (sequencial)
    
    #ifdef MUTEX
        pthread_mutex_t mutex_rm;   // bloqueio para removeCommand()
        pthread_mutex_t mutex_ap;   // bloqueio para os comandos c, l, d
    #elif RWLOCK
        //mutex semelhante ao wrlock
        pthread_mutex_t mutex_rm;   // bloqueio para removeCommand()
        pthread_rwlock_t rwlock;    // bloqueio para os comandos c, l, d
    #endif
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);

#endif /* FS_H */
