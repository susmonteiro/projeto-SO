#include "fs.h"

//Cria um sistema de ficheiros (Tecnicofs)
tecnicofs new_tecnicofs(){
	tecnicofs fs = malloc(sizeof(struct tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->tecnicofs_lock = (lock)malloc(sizeof(struct lock));
	if (!fs->tecnicofs_lock) {
		perror("failed to allocate tecnicofs_lock");
		exit(EXIT_FAILURE);
	}

	return fs;
}

void free_tecnicofs(tecnicofs fs){
	free_tree(fs->bstRoot);
	free(fs->tecnicofs_lock);
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
	return -1;
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