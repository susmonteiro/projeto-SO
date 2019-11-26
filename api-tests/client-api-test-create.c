#include "../tecnicofs-api-constants.h"
#include "../tecnicofs-client-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int main(int argc, char** argv) {
     if (argc != 2) {
        printf("Usage: %s sock_path\n", argv[0]);
        exit(0);
    }
    assert(tfsMount(argv[1]) == 0);
    printf("Test: create file sucess");
    assert(tfsCreate("a", RW, READ) == 0);
    printf("==============\n");
    assert(tfsCreate("r", RW, READ) == 0);
    puts("fim");
    getchar(); //nao sair
    return 0;
}