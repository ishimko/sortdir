#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <libgen.h>

#include <alloca.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define ARGS_COUNT 4
#define BUFFER_SIZE 512
#define FOLDER_PERMISSIONS 0744

typedef enum {
    BY_NAME = 0,
    BY_SIZE = 1
} sortType;

typedef struct {
    char path[PATH_MAX];
    char name[FILENAME_MAX];
    off_t file_size;
} fileInfo;

typedef struct {
    fileInfo **files;
    size_t files_count;
} filesList;

typedef ssize_t (*compareFunc)(const fileInfo *, const fileInfo *);

char *module_name;

void print_error(const char *module_name, const char *error_msg, const char *file_name) {
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
    for (position = 0;
         (position < sorted_files->files_count) && (compare_func(file, sorted_files->files[position]) > 0); position++);

    sorted_files->files = realloc(sorted_files->files, sizeof(fileInfo *) * (++(sorted_files->files_count)));

    for (size_t i = sorted_files->files_count - 1; i > position; i--) {
        sorted_files->files[i] = sorted_files->files[i - 1];
    };

    sorted_files->files[position] = file;
}

void file_path(char *dest, const char *const path, const char *const name) {
    strcpy(dest, path);
    strcat(dest, "/");
    strcat(dest, name);
}

int is_dir(const char *parent_path, const char *entry_name) {
    struct stat entry_info;
    char *full_path = alloca(strlen(parent_path) + strlen(entry_name) + 2);
    file_path(full_path, parent_path, entry_name);

    if (stat(full_path, &entry_info) == -1) {
        print_error(module_name, strerror(errno), full_path);
        return 0;
    }

    return entry_info.st_mode & S_IFDIR;
}

off_t file_size(const char *parent_path, const char *file_name) {
    struct stat file_info;
    char *full_path = alloca(strlen(parent_path) + strlen(file_name) + 2);

    file_path(full_path, parent_path, file_name);

    if (stat(full_path, &file_info) == -1) {
        print_error(module_name, strerror(errno), full_path);
        return 0;
    }

    return file_info.st_size;
}

void files_in_dir(const char *path, filesList *sorted_files, compareFunc compare_func) {
    DIR *dir_stream;
    struct dirent *dir_entry;
    if (!(dir_stream = opendir(path))) {
        print_error(module_name, strerror(errno), path);
        return;
    }

    while ((dir_entry = readdir(dir_stream)) != NULL) {
        char *entry_name = dir_entry->d_name;

        if (!strcmp(".", entry_name) || !strcmp("..", entry_name))
            continue;

        if (is_dir(path, entry_name)) {
            char *subdir = alloca(strlen(path) + strlen(entry_name) + 2);

            file_path(subdir, path, entry_name);
            files_in_dir(subdir, sorted_files, compare_func);
        } else {
            fileInfo *file = malloc(sizeof(fileInfo));
            strcpy(file->name, entry_name);
            strcpy(file->path, path);
            file->file_size = file_size(path, entry_name);
            add_file(file, sorted_files, compare_func);
        }
    }

    if (closedir(dir_stream)){
        print_error(module_name, strerror(errno), path);
    }
}

void copy_file(const char *src_path, const char *dest_path) {
    char buffer[BUFFER_SIZE];
    int src, dest;

    if ((src = open(src_path, O_RDONLY)) == -1) {
        print_error(module_name, strerror(errno), src_path);
        return;
    }

    int copies_count = 0;
    char *tmp_dest_path = alloca(strlen(dest_path) + 10);

    strcpy(tmp_dest_path, dest_path);
    while ((dest = open(tmp_dest_path, O_WRONLY | O_CREAT | O_EXCL)) == -1) {
        if (errno == EEXIST) {
            sprintf(tmp_dest_path, "%s (%d)", dest_path, ++copies_count);
        } else {
            print_error(module_name, strerror(errno), tmp_dest_path);
            close(src);
            return;
        }
    }

    ssize_t bytes_count = 0;
    while ((bytes_count = read(src, buffer, BUFFER_SIZE)) > 0 && bytes_count != -1) {
        if (write(dest, buffer, (size_t) bytes_count) == -1) {
            print_error(module_name, strerror(errno), dest_path);

            close(src);
            close(dest);

            return;
        }
    }

    close(src);
    close(dest);

    if (bytes_count == -1) {
        print_error(module_name, strerror(errno), src_path);
        return;
    }
}

void copy_files_list(filesList *files_to_copy, const char *dest_folder) {
    int i;
    char dest_path[PATH_MAX];
    char src_path[PATH_MAX];
    for (i = 0; i < files_to_copy->files_count; i++) {
        file_path(dest_path, dest_folder, files_to_copy->files[i]->name);
        file_path(src_path, files_to_copy->files[i]->path, files_to_copy->files[i]->name);
        copy_file(src_path, dest_path);
    }
}

int get_sort_type(const char *param_str) {
    if (strcmp(param_str, "1") == 0) {
        return BY_SIZE;
    }

    if (strcmp(param_str, "2") == 0) {
        return BY_NAME;
    }

    return -1;
}


int main(int argc, char *argv[]) {
    module_name = basename(argv[0]);

    if (argc != ARGS_COUNT) {
        print_error(module_name, "Wrong number of parameters", NULL);
        return 1;
    }

    char src_dir[PATH_MAX];
    char dest_dir[PATH_MAX];
    realpath(argv[1], src_dir);
    realpath(argv[2], dest_dir);

    struct stat out_dir_info;
    if (stat(dest_dir, &out_dir_info) == -1) {
        if (errno != ENOENT) {
            print_error(module_name, strerror(errno), dest_dir);
            return 1;
        }
        if (mkdir(dest_dir, FOLDER_PERMISSIONS) == -1) {
            print_error(module_name, strerror(errno), dest_dir);
            return 1;
        }
    } else {
        if (!S_ISDIR(out_dir_info.st_mode)) {
            print_error(module_name, "Not a directory", dest_dir);
            return 1;
        }
        if (access(dest_dir, W_OK) == -1){
            print_error(module_name, strerror(errno), dest_dir);
            return 1;
        }
    }


    sortType sort_type;
    if ((sort_type = (sortType) get_sort_type(argv[3])) == -1) {
        print_error(module_name, "Invalid sort type", NULL);
        return 1;
    }

    compareFunc compare_func = (sort_type == BY_NAME) ? compare_files_by_name : compare_files_by_size;

    filesList sorted_files;
    sorted_files.files_count = 0;
    sorted_files.files = NULL;

    files_in_dir(src_dir, &sorted_files, compare_func);
    copy_files_list(&sorted_files, dest_dir);

    for (int i = 0; i < sorted_files.files_count; i++){
        free(sorted_files.files[i]);
    }
}
