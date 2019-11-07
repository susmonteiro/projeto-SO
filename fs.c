#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* int obtainNewInumber(int *nextINumber) {
    // incrementa sequencialmente o Inumber
	int newInumber = ++(*nextINumber);
	return newInumber;
} */

tecnicofs* new_tecnicofs(int size){
    // cria o root do filesystem
	int i = 0;

	tecnicofs* hashTab = (tecnicofs*)malloc(size * sizeof(tecnicofs));
	for(i = 0; i < size; i++) {

		hashTab[i] = malloc(sizeof(struct tecnicofs));
		if (!hashTab[i]) {
			perror("failed to allocate tecnicofs");
			exit(EXIT_FAILURE);
		}

		initLock(hashTab[i]);
		hashTab[i]->bstRoot = NULL;
	}
	
	return hashTab;
}

void free_hashTab(tecnicofs* hashTab, int size){
	int i;
	for(i = 0; i < size; i++) {
		destroyLock(hashTab[i]);
		free_tecnicofs(hashTab[i]);
	}
	free(hashTab);
}

void free_tecnicofs(tecnicofs fs){
	free_tree(fs->bstRoot);
	free(fs);
}

void create(tecnicofs fs, char *name, int inumber){
	fs->bstRoot = insert(fs->bstRoot, name, inumber);
}

void delete(tecnicofs fs, char *name){
	fs->bstRoot = remove_item(fs->bstRoot, name);
}

int lookup(tecnicofs fs, char *name){
	node* searchNode = search(fs->bstRoot, name);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_HashTab_tree(FILE * fp, tecnicofs* hashTab, int size){
	int i;
	for(i = 0; i < size; i++) print_tecnicofs_tree(fp, hashTab[i]);
}

void print_tecnicofs_tree(FILE * fp, tecnicofs fs){
	print_tree(fp, fs->bstRoot);
}

int searchHash(char *name, int size) {
	return hash(name, size);
}


void initMutex(pthread_mutex_t *mutex) {
    pthread_mutex_init(mutex, NULL);
}

void destroyMutex(pthread_mutex_t *mutex) {
    pthread_mutex_destroy(mutex);
}

void initRWLock(pthread_rwlock_t *rwlock) {
    pthread_rwlock_init(rwlock, NULL);
    
}

void destroyRWLock(pthread_rwlock_t *rwlock) {
    pthread_rwlock_destroy(rwlock);
}

void initLock(tecnicofs fs){
    #ifdef MUTEX
        initMutex(&fs->mutex_ap);  //ERROOOOOOOOOOOOOOOO
    #elif RWLOCK
        initRWLock(&fs->rwlock);
    #endif
}

void destroyLock(tecnicofs fs) {
    #ifdef MUTEX
        destroyMutex(&fs->mutex_ap);
    #elif RWLOCK
        destroyRWLock(&fs->rwlock);
    #endif
}