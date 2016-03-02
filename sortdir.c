#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "constants.h"


int main(int argc, char* argv[]){
	if (argc != ARGS_COUNT){
		fprintf(stderr, "%s: %s", basename(argv[0]), "неверное количество аргументов");
		exit(1);
	}
}
