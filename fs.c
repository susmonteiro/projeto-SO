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

	tecnicofs* hash_tab = (tecnicofs*)malloc(size * sizeof(tecnicofs));
	for(i = 0; i < size; i++) {
		hash_tab[i] = malloc(sizeof(struct tecnicofs));
		if (!hash_tab[i]) {
			perror("failed to allocate tecnicofs");
			exit(EXIT_FAILURE);
		}
		hash_tab[i]->bstRoot = NULL;
	}
	
	return hash_tab;
}

void free_hashTab(tecnicofs* hashtab, int size){
	int i;
	for(i = 0; i < size; i++) free_tecnicofs(hashtab[i]);
	free(hashtab);
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

void print_HashTab_tree(FILE * fp, tecnicofs* hashtab, int size){
	int i;
	for(i = 0; i < size; i++) print_tecnicofs_tree(fp, hashtab[i]);
}

void print_tecnicofs_tree(FILE * fp, tecnicofs fs){
	print_tree(fp, fs->bstRoot);
}

int searchHash(char *name, int size) {
	return hash(name, size);
}