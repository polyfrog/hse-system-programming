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

        int dev_null = open("/dev/null", O_RDWR);

        if (dev_null == -1) {
            printf("Failed to open /dev/null in read write mode\n");
            return 1;
        }

        dup2(dev_null, 0);
        dup2(dev_null, 1);
        dup2(dev_null, 2);
        close(dev_null);

        execv(full_path, &argv[1]);

        printf("Error executing %s\n", full_path);
        return 1;
    }
    
    return 0;
}