#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define DARK_DIR_PATH   "darkroom/"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        puts("Please specify filepath");
        return 1;
    }

    char cur_dir_path[PATH_MAX];
    char file_path[PATH_MAX];
    char new_file_path[PATH_MAX];
    char dark_dir_path[PATH_MAX];

    getcwd(cur_dir_path, PATH_MAX);
    strcat(cur_dir_path, "/");

    if (argv[1][0] != '/') {
        strcpy(file_path, cur_dir_path);
    }
    strcat(file_path, argv[1]);

    strcpy(dark_dir_path, cur_dir_path);
    strcat(dark_dir_path, DARK_DIR_PATH);
    strcpy(new_file_path, dark_dir_path);  
    strcat(new_file_path, argv[1]);  

    struct stat sb;

    if (stat(dark_dir_path, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
        if (mkdir(dark_dir_path, 0311) != 0) {
            printf("Error creating directory %s\n", dark_dir_path);
            return 1;
        }
    } else if (access(dark_dir_path, 0311) != 0) {
        if (chown(dark_dir_path, getuid(), getgid()) != 0 || chmod(dark_dir_path, 0311) != 0) {
            printf("Failed to change owner or access mode for %s\n", dark_dir_path);
            return 1;
        }

    }

    if (rename(file_path, new_file_path) != 0) {
        printf("Error moving file %s to %s\n", file_path, new_file_path);
        return 1;
    }

    return 0;
}