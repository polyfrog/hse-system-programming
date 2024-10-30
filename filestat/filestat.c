#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

int main(int argc, char* argv[])
{
    DIR *directory;
    struct dirent *entry;
    struct stat file_stat;
    char dir_path[PATH_MAX];
    char file_path[PATH_MAX];
    int block_count = 0, char_special_count = 0, directory_count = 0, pipe_count = 0, regular_count = 0, symlink_count = 0, socket_count = 0, unknown_count = 0;

    getcwd(dir_path, PATH_MAX);
    if (dir_path[strlen(dir_path) -1] != '/') {
        strcat(dir_path, "/");
    }

    directory = opendir(dir_path);
    if (directory == NULL) {
        printf("Failed opening directory %s\n", dir_path);
        return 1;
    }

    while ((entry = readdir(directory)) != NULL) {
        strcpy(file_path, dir_path);
        strcat(file_path, entry->d_name);

        if (stat(file_path, &file_stat) != -1) {
            if (S_ISBLK(file_stat.st_mode)) {
                block_count++;
            } else if (S_ISCHR(file_stat.st_mode)) {
                char_special_count++;
            } else if (S_ISDIR(file_stat.st_mode)) {
                directory_count++;
            } else if (S_ISFIFO(file_stat.st_mode)) {
                pipe_count++;
            } else if (S_ISREG(file_stat.st_mode)) {
                regular_count++;
            } else if (S_ISLNK(file_stat.st_mode)) {
                symlink_count++;
            } else if (S_ISSOCK(file_stat.st_mode)) {
                socket_count++;
            } else {
                unknown_count++;
            }
        } else {
            unknown_count++;
        }
    }

    if (closedir(directory) == -1) {
        puts("Failed closing directory\n");
        return 1;
    }
    
    printf(
        "Directory %s contains:\n%d\tblock files\n%d\tcharacter special files\n%d\tdirectories\n%d\tpipes or FIFO special files\n%d\tregular files\n%d\tsymbolic links\n%d\tsocket files\n%d\tunknown files\n",
        dir_path, block_count, char_special_count, directory_count, pipe_count,  regular_count, symlink_count, socket_count, unknown_count
    );

    return 0;
}