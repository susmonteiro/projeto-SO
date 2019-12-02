#ifndef FS_H
#define FS_H
#include <pthread.h>
#include "erro.h"
#include "lib/bst.h"
#include "lib/hash.h"

typedef struct lock {
    #ifdef MUTEX
        pthread_mutex_t mutex;   // bloqueio para os comandos c, l, d
    #elif RWLOCK
        pthread_rwlock_t rwlock;    // bloqueio para os comandos c, l, d
    #endif
} *lock;

typedef struct tecnicofs {
    node* bstRoot; // nó da raiz da arvore
    lock tecnicofs_lock;
} *tecnicofs;

tecnicofs new_tecnicofs();
void free_tecnicofs(tecnicofs fs);
void create(tecnicofs fs, char *name, int inumber);
void delete(tecnicofs fs, char *name);
int lookup(tecnicofs fs, char *name);
void print_HashTab_tree(FILE * fp, tecnicofs* hashTab, int size);
void print_tecnicofs_tree(FILE * fp, tecnicofs fs);
int searchHash(char *name, int size);

#endif /* FS_H */
