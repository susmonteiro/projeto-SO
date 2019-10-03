#ifndef FS_H
#define FS_H
#include "lib/bst.h"

typedef struct tecnicofs {
    node* bstRoot; //nó da raiz da arvore
    int nextINumber; //guarda o ultimo inumber atribuido (sequencial)
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);

#endif /* FS_H */
