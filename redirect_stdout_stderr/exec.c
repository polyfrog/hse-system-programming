#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Please specify path to executable file\n");
        return 1;
    }

    char full_path[PATH_MAX];
    if (argv[1][0] != '/') {
        getcwd(full_path, PATH_MAX);
        strcat(full_path, "/");
    }
    strcat(full_path, argv[1]);

    int pid = fork();
    if (pid == -1) {
        printf("Error creating child process\n");
        return 1;
    } else if (pid == 0) {
        if (setsid() < 0) {
            printf("Failed creating new session for child process\n");
            return 1;
        }

        FILE *out = fopen("out.txt", "a");
        FILE *err = fopen("err.txt", "a");

        dup2(fileno(out), 1);
        dup2(fileno(err), 2);

        fclose(out);
        fclose(err);

        execv(full_path, &argv[1]);

        printf("Error executing %s\n", full_path);
        return 1;
    }
    return 0;
}