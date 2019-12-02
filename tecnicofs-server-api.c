#include "tecnicofs-server-api.h"

//Hashtable de tecnicofs
extern tecnicofs* hash_tab;
extern int numberBuckets;

//Vetor que guarda o numero de clientes que tem um determinado ficheiro aberto
int opened_files[INODE_TABLE_SIZE]; 
//trinco para o vetor opened_files
/* extern */ lock of_lock; 


//===============================================
//Funcoes relativas ao vetor de ficheiros abertos
//===============================================

void initOpenedFilesCounter() {
    int i = 0;
    //inicializacao do trinco
    of_lock = (lock)malloc(sizeof(struct lock));
    //
    puts(";");
    initLock(of_lock);

    for(i = 0; i < INODE_TABLE_SIZE; i++) {
        opened_files[i] = 0;
    }
}

void clearClientOpenedFilesCounter(tecnicofs_fd *file_tab) {
    int i = 0;
    
    closeWriteLock(of_lock);
    for(i = 0; i < MAX_FILES_OPENED; i++) {
        if(file_tab[i].iNumber != FD_EMPTY){
            opened_files[file_tab[i].iNumber] -= 1;
        }
    }
    openLock(of_lock);

}

void freeOpenedFilesCounter(){
    destroyLock(of_lock);
    free(of_lock);
}

//==============================
//Funcoes relativas aos comandos
//==============================

/*  Funcao do servidor que trata do pedido do cliente para criar um novo ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [c, 'filename', 'permissions']
*   Retorno:
*       - Erro de preexistencia de ficheiro (ALREADY_EXISTS)
*       - Caso contrario (SUCESSO) 
*/
int commandCreate(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid) {
    int iNumber;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; // arvore do nome atual
    

    closeWriteLock(fs->tecnicofs_lock); // permite leituras simultaneas, impede escrita
    if (lookup(fs, vec[0]) != -1){      // se ja existir 
        openLock(fs->tecnicofs_lock);
        return ALREADY_EXISTS;
    }

    iNumber = inode_create(uid, OWNER_PERMISSION(vec[1]), OTHER_PERMISSION(vec[1]));
    create(fs, vec[0], iNumber);
    openLock(fs->tecnicofs_lock);

    return SUCCESS;
}

/*  Funcao do servidor que trata do pedido do cliente para apagar um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [d, 'filename']
*   Retorno:
*       - Erro de inexistencia de ficheiro (DOESNT_EXIST)
*       - Erro se o ficheiro estiver aberto (FILE_IS_OPENED)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (PERMISSION_DENIED)
*       - Caso contrario (SUCCESS) 
*/
int commandDelete(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
    int search_result;
    uid_t owner;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual
    
    closeWriteLock(fs->tecnicofs_lock); // bloqueia leituras e escritas do fs

    if ((search_result = lookup(fs, vec[0])) == -1)     //se nao existir
        return DOESNT_EXIST;

    closeReadLock(of_lock);
    /* Procura no vetor de ficheiros abertos se algum cliente tem este ficheiro aberto
    Em caso afirmativo, cancela a operacao de apagar o ficheiro */
    if (opened_files[search_result] != 0) { 
        openLock(of_lock);
        openLock(fs->tecnicofs_lock);
        return FILE_IS_OPENED;
    }
    openLock(of_lock);

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);

    // confirmar permissao para apagar (apenas consegue apagar o dono)
    if (uid != owner)
        return PERMISSION_DENIED;
    
    inode_delete(search_result);
    delete(fs, vec[0]); 
    openLock(fs->tecnicofs_lock);   

    return SUCCESS;
}

/*  Funcao do servidor que trata do pedido do cliente para renomear um ficheiro no servidor
*
*   Representacao do comando para criar um ficheiro:
*       [r, 'filenameOld', 'filenameNew']
*   Retorno:
*       - Erro se o novo nome ja existir (ALREADY_EXISTS)
*       - Erro de inexistencia de ficheiro antigo (DOESNT_EXIST)
*       - Erro se o cliente nao tiver permissao para renomear o ficheiro (PERMISSION_DENIED)
*       - Caso contrario (SUCCESS) 
*/
int commandRename(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid){
	int search_result;           //inumber retornado pela funcao lookup
    uid_t owner;
    int s1, s2;
    tecnicofs fs1, fs2;

    s1 = searchHash(vec[0], numberBuckets);
    s2 = searchHash(vec[1], numberBuckets);

    if (s1 > s2) { //para impedirmos interblocagem, percorremos hash_tab sempre na mesma direcao
        fs1 = hash_tab[s2]; //arvore do nome atual
        fs2 = hash_tab[s1]; //arvore onde vai ser inserido o ficheiro com o novo nome
    } else {
        fs1 = hash_tab[s1]; //arvore do nome atual
        fs2 = hash_tab[s2]; //arvore onde vai ser inserido o ficheiro com o novo nome
    }
    
    closeWriteLock(fs2->tecnicofs_lock);
    if (lookup(fs2, vec[1]) != -1) { // se o novo nome ja existir, a operacao e' cancelada sem devolver erro
		openLock(fs2->tecnicofs_lock);
		return ALREADY_EXISTS;
    }
    if (fs1 != fs2) closeWriteLock(fs1->tecnicofs_lock);

    if ((search_result = lookup(fs1, vec[0])) == -1) { // se o ficheiro atual nao existir, a operacao e' cancelada
		openLock(fs1->tecnicofs_lock);
		if (fs1 != fs2) openLock(fs2->tecnicofs_lock); // nao fazer unlock 2 vezes da mesma arvore
		return DOESNT_EXIST;
	}

    inode_get(search_result, &owner, NULL, NULL, NULL, 0);
    if(uid != owner){
        openLock(fs1->tecnicofs_lock);
        if (fs1 != fs2) openLock(fs2->tecnicofs_lock); // nao fazer unlock 2 vezes da mesma arvore
        return PERMISSION_DENIED;
    }

    delete(fs1, vec[0]);
	if (fs1 != fs2) openLock(fs1->tecnicofs_lock);
	// se as fs forem iguais nao podemos fazer unlock antes do create
	create(fs2, vec[1], search_result); // novo ficheiro: novo nome, mesmo Inumber
	openLock(fs2->tecnicofs_lock);

    return SUCCESS;
}


/*  Funcao do servidor que trata do pedido do cliente para abrir um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [o, 'filename', mode]
*   Retorno:
*       - Erro de inexistencia de ficheiro (DOESNT_EXISTS)
*       - Erro se o cliente nao tiver permissao para abrir o ficheiro (PERMISSION_DENIED)
*       - Erro se nao houver posicoes livres na tabela de ficheiros para abrir um novo ficheiro (MAX_OPENED_FILES)
*       - Caso contrario, retorna o fd onde o ficheiro ficou aberto 
*/
int commandOpen(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], uid_t uid, tecnicofs_fd *file_tab){
    int search_result, idx_fd;
    permission ownerp, otherp, mode;
    uid_t user;
    tecnicofs fs = hash_tab[searchHash(vec[0], numberBuckets)]; //arvore do nome atual

    closeReadLock(fs->tecnicofs_lock);    // permite leituras simultaneas, impede escrita
    
    if ((search_result = lookup(fs, vec[0])) == -1){     //se nao existir
        openLock(fs->tecnicofs_lock);
        return DOESNT_EXIST;
    }

    inode_get(search_result, &user, &ownerp, &otherp, NULL, 0);
    openLock(fs->tecnicofs_lock);

    mode = atoi(vec[1]);
    //consideramos que a permissao dos outros nao se aplica ao dono do ficheiro
    if (!((OWNER_HAS_PERMISSION(user, uid, mode, ownerp)) || (OTHER_HAS_PERMISSION(user, uid, mode, otherp))))
        return PERMISSION_DENIED;
    
    idx_fd = 0;
    while (file_tab[idx_fd++].iNumber != FD_EMPTY)
        //testa se o proximo valor de idx_fd e' superior a dimensao de file_tab
        if (idx_fd == MAX_FILES_OPENED) 
            return MAX_OPENED_FILES;
    
    idx_fd--; //idx_fd e' o valor da proxima posicao. Para acedermos a atual temos de decrementar o valor de idx_fd
    file_tab[idx_fd].iNumber = search_result;
    file_tab[idx_fd].open_as = mode;

    closeWriteLock(of_lock);
    opened_files[search_result]++; //incrementa o numero de clientes que tem o ficheiro (cujo iNumber e' search_result) aberto
    openLock(of_lock);

    return idx_fd;
}


/*  Funcao do servidor que trata do pedido do cliente para fechar um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [x, fd]
*   Retorno:
*       - Erro se o ficheiro nao estiver aberto (NOT_OPENED)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (INDEX_OUT_OF_RANGE)
*       - Caso contrario (SUCCESS)
*/
int commandClose(char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    int idx_fd = atoi(vec[0]);  //indice da tabela de ficheiros do cliente

    if (idx_fd < 0 || idx_fd > 4) return INDEX_OUT_OF_RANGE;

    if (file_tab[idx_fd].iNumber == FD_EMPTY) return NOT_OPENED;

    closeWriteLock(of_lock);
    opened_files[file_tab[idx_fd].iNumber]--; //decrementa o numero de clientes que tem o ficheiro (cujo iNumber e' file_tab[idx_fd].iNumber) aberto
    openLock(of_lock);

    file_tab[idx_fd].iNumber = FD_EMPTY;    //coloca a posicao da tabela de ficheiros no estado "sem ficheiro aberto"

    return SUCCESS;
}


/*  Funcao do servidor que trata do pedido do cliente para ler de um ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [l, fd, len]
*   Retorno: nenhum (funcao void)
*   Mensagem enviada, como resposta:
*       - Erro se o ficheiro nao estiver aberto (NOT_OPENED)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (INDEX_OUT_OF_RANGE)
*       - Erro se o cliente nao tiver permissao para ler do ficheiro (PERMISSION_DENIED)
*       - Caso contrario, envia o numero de caracteres lidos
*/
void commandRead(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    int tab_idx = atoi(vec[0]);
    int buffer_len = atoi(vec[1]); //validade confirmada na funcao chamadora 
    tecnicofs_fd file;
    char buffer[buffer_len];
    int msg = SUCCESS;
    if(tab_idx < 0 || tab_idx > MAX_FILES_OPENED) msg = INDEX_OUT_OF_RANGE;

    file = file_tab[tab_idx];
    if (file.iNumber == FD_EMPTY) msg = NOT_OPENED;
    if (!(CAN_READ(file.open_as))) msg = PERMISSION_DENIED;
    
    //ler apenas em caso de sucesso
    if(msg == SUCCESS)
        msg = inode_get(file.iNumber, NULL, NULL, NULL, buffer, buffer_len-1); // buffer comeca em idx 0, -1 cuida do offset
        // msg passa a ser o numero de caracteres lidos
    
    feedback(fd, msg); //informar de possivel erro
    if(msg > 0){ // caso a msg nao seja erro
    // +1 para incluir o \0 (strlen nao contabiliza o '\0')
    if (write(fd, buffer, msg+1) != msg+1) sysError("commandRead(write)");
    }

}


/*  Funcao do servidor que trata do pedido do cliente para escrever num ficheiro
*
*   Representacao do comando para criar um ficheiro:
*       [w, fd, dataInBuffer]
*   Retorno:
*       - Erro se o ficheiro nao estiver aberto (NOT_OPEN)
*       - Erro se fd nao for uma posicao valida da tabela de ficheiros (INDEX_OUT_OF_RANGE)
*       - Erro se o cliente nao tiver permissao para ler do ficheiro (PERMISSION_DENIED)
*       - Caso contrario (SUCCESS)
*/
int commandWrite(int fd, char vec[MAX_ARGS_INPUTS][MAX_INPUT_SIZE], tecnicofs_fd *file_tab){
    int tab_idx = atoi(vec[0]);
    tecnicofs_fd file;
    if(tab_idx < 0 || tab_idx > MAX_FILES_OPENED) return INDEX_OUT_OF_RANGE;

    file = file_tab[tab_idx];
    if (file.iNumber == FD_EMPTY) return NOT_OPENED;
    if (!(CAN_WRITE(file.open_as))) return PERMISSION_DENIED;
    
    return inode_set(file.iNumber, vec[1], strlen(vec[1]));
}