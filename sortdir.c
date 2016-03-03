#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>


#include "constants.h"


int main(int argc, char* argv[]){
    if (argc != ARGS_COUNT){
        fprintf(stderr, "%s: %s\n", argv[0], "wrong number of arguments");
        return 1;
    }

    struct stat paramInfo;
    if (stat(argv[1], &paramInfo) == -1){
        perror(argv[0]);
        return 1;
    }

    DIR *dir;
    dir = opendir(argv[1]);
    if (!dir) {
        fprintf(stderr, "%s: %s: %s\n", argv[0], strerror(errno), argv[1]);
        return 1;
    }

    if (mkdir(argv[2], 0700) == -1){
        fprintf(stderr, "%s: %s: %s\n", argv[0], strerror(errno), argv[2]);
        return 1;
    }
}
