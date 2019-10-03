#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
	int i;
	if(argc > 2)
		for (i = 0; i < atoi(argv[2]); i++) 

			printf("%s\n", argv[1]);


	return 0;
}

