#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <libgen.h>

#include <alloca.h>
#include <stdlib.h>

#define ARGS_COUNT 4
#define FOLDER_PERMISSIONS 0700

typedef enum {
    BY_NAME,
    BY_SIZE
} sortType;

typedef struct {
    char path[PATH_MAX];
    char name[FILENAME_MAX];
    size_t file_size;
} fileInfo;

typedef struct{
    fileInfo **files;
    size_t files_count;
} filesList;

typedef ssize_t (*compareFunc)(const fileInfo *, const fileInfo *);

char *module_name;

void print_error(char *module_name, char *error_msg, char *file_name) {
    fprintf(stderr, "%s: %s %s\n", module_name, error_msg, file_name ? file_name : "");
}

ssize_t compare_files_by_name(const fileInfo *f1, const fileInfo *f2) {
    return strcmp(f1->name, f2->name);
}

ssize_t compare_files_by_size(const fileInfo *f1, const fileInfo *f2) {
    return f1->file_size - f2->file_size;
}

void add_file(fileInfo *file, filesList *sorted_files, compareFunc compare_func) {
    int position;
    for (position = 0; (position < sorted_files->files_count) && (compare_func(file, sorted_files->files[position]) < 0); position++);

    sorted_files->files = realloc(sorted_files->files, ++(sorted_files->files_count));

    for (int i = position; i < sorted_files->files_count - 1; i++){
        sorted_files->files[i+1] = sorted_files->files[i];
    };

    sorted_files->files[sorted_files->files_count - 1] = file;
}

void file_path(char *dest, const char *const path, const char *const name) {
    strcat(dest, path);
    strcat(dest, "/");
    strcat(dest, name);
}

int is_dir(const char *const parent_path, const char *const entry_name) {
    if (!strcmp(".", entry_name) || !strcmp("..", entry_name))
        return 0;

    struct stat entry_info;
    char *full_path = alloca(strlen(parent_path) + strlen(entry_name) + 2);

    file_path(full_path, parent_path, entry_name);

    if (lstat(full_path, &entry_info) == -1) {
        print_error(module_name, strerror(errno), full_path);
        return 0;
    }

    return (S_ISDIR(entry_info.st_mode));

}

void files_in_dir(const char *const path, filesList *sorted_files) {
    DIR *dir_stream;
    struct dirent *dir_entry;
    if (!(dir_entry = opendir(path))) {
        print_error(module_name, strerror(errno), path);
        return;
    }

    while ((dir_entry = readdir(dir_stream)) != NULL) {
        char *entry_name = dir_entry->d_name;

        if (is_dir(path, entry_name)) {
            char *subdir = alloca(strlen(path) + strlen(entry_name) + 2);
            file_path(subdir, path, entry_name);
            files_in_dir(subdir, sorted_files);
        } else {
            struct fileInfo *file = malloc(sizeof(struct fileInfo));
            add_file(sorted_files, file);
        }

    }
}

int get_sort_type(const char *param_str, sortType *sort_type){
    if (strcmp(param_str, "1") == 0){
        *sort_type = BY_SIZE;
        return 0;
    }

    if (strcmp(param_str, "2") == 0){
        *sort_type = BY_NAME;
        return 0;
    }

    return -1;
}



int main(int argc, char *argv[]) {
    module_name = basename(argv[0]);

    if (argc != ARGS_COUNT) {
        print_error(module_name, "wrong number of parameters", NULL);
        return 1;
    }

    struct stat param_info;
    if (stat(argv[1], &param_info) == -1) {
        print_error(module_name, strerror(errno), argv[1]);
        return 1;
    }

    if (mkdir(argv[2], FOLDER_PERMISSIONS) == -1) {
        print_error(module_name, strerror(errno), argv[2]);
        return 1;
    }

    sortType sort_type;
    if (get_sort_type(argv[3], &sort_type) == -1){
        print_error(module_name, "invalid sort type", NULL);
        return 1;
    }

    compareFunc compare_func = (sort_type == BY_NAME)?compare_files_by_name:compare_files_by_size;

    struct filesList sorted_files;
    sorted_files.files_count = 0;
    sorted_files.files = NULL;

    files_in_dir(argv[1], sorted_files, compare_func);
}
