#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>


#include "constants.h"

int sort


int main(int argc, char* argv[]){
	if (argc != ARGS_COUNT){
		fprintf(stderr, "%s: %s", argv[0], "неверное количество аргументов\n");
		return 1;
	}
	
	struct stat paramInfo;
	if (stat(argv[1], &paramInfo) == -1){
		perror(argv[0]);
		return 1;		
	}
	
	if (~paramInfo.st_mode & S_IFDIR){
		fprintf(stderr, "%s: %s", argv[0], "первый параметр не является каталогом\n");
		return 1;
	}
	
}
