#ifndef FS_H
#define FS_H
#include <pthread.h>
#include "lib/bst.h"
#include "lib/hash.h"

typedef struct tecnicofs {
    node* bstRoot; //nó da raiz da arvore
    
    #ifdef MUTEX
        pthread_mutex_t mutex_ap;   // bloqueio para os comandos c, l, d
    #elif RWLOCK
        //mutex semelhante ao wrlock
        pthread_rwlock_t rwlock;    // bloqueio para os comandos c, l, d
    #endif
} *tecnicofs;

//int obtainNewInumber(int *nextINumber);
tecnicofs* new_tecnicofs(int size);
void free_hashTab(tecnicofs* hashtab, int size);
void free_tecnicofs(tecnicofs fs);
void create(tecnicofs fs, char *name, int inumber);
void delete(tecnicofs fs, char *name);
int lookup(tecnicofs fs, char *name);
void print_HashTab_tree(FILE * fp, tecnicofs* hashtab, int size);
void print_tecnicofs_tree(FILE * fp, tecnicofs fs);
int searchHash(char *name, int size);

#endif /* FS_H */
