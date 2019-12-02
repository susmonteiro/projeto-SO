#include "../tecnicofs-client-api.h"
#include "../tecnicofs-api-constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    //
    
    char readBuffer[20] = {0};
    char lixo[6];
    assert(tfsMount(argv[1]) == 0);
	
    puts("cria abc");
    assert(tfsCreate("abc", RW, WRITE) == 0 );
	puts("espera");
//cria abc
//espera
    fgets(lixo, 5, stdin);
	puts("abre o abc RW");
    int fd = -1;
    assert((fd = tfsOpen("abc", RW)) == 0);
	puts("escreve hmm no abc");
    assert(tfsWrite(fd, "hmmmmmm", 3) == 0);
//le o 
	puts("espera");
	fgets(lixo, 5, stdin);

	puts("le max15 do abc");
    tfsRead(fd, readBuffer, 15);

    puts(readBuffer);

    puts("delete abc (nao devera dar porque aberto no outro cliente)");
    
    assert(tfsDelete("abc") == TECNICOFS_ERROR_FILE_IS_OPEN);
    puts("espera");
    fgets(lixo, 5, stdin);

    puts("delete abc (deve dar)");
    
    assert(tfsClose(fd) == 0);
    assert(tfsDelete("abc") == 0);
    assert(tfsUnmount() == 0);

    return 0;
}
