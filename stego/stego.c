#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAGIC_NUM_BYTES 3
#define WRITE_MESSAGE     "write"
#define READ_MESSAGE    "read"
#define REMOVE_MESSAGE  "remove"

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: %s write | read | remove [filename] [message]\n", argv[0]);
        return 1;
    }

    char *operation = argv[1];
    char *filename = argv[2];

    if (strcmp(operation, WRITE_MESSAGE) == 0 && argc < 4) {
        printf("Usage: %s write | read | remove [filename] [message]. Please specify message\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file %s\n", filename);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    const long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data;
    data = malloc(file_size);

    char magic_numbers[MAGIC_NUM_BYTES] = {0};
    char expected_magic_numbers[MAGIC_NUM_BYTES] = {0xFF, 0xD8, 0xFF};
    char end_sequence[2] = {0xFF, 0xD9};

    int num_read = fread(magic_numbers, 1, MAGIC_NUM_BYTES, file);

    if (num_read != MAGIC_NUM_BYTES) {
        printf("Failed to check magic bytes\n");
        return 1;
    }

    if (memcmp(magic_numbers, expected_magic_numbers, MAGIC_NUM_BYTES) != 0) {
        printf("%s is not a jpeg\n", filename);
        return 1;
    }

    fseek(file, 0, SEEK_SET);
    fread(data, 1, file_size, file);
    fclose(file);

    long int total_jpeg_bytes = 0;
    int flag = 0;

    for (int i = 0; i < file_size - 1; i++) {
        char sequence[2] = {data[i], data[i + 1]};
        if (memcmp(sequence, end_sequence, 2) == 0) {
            total_jpeg_bytes = i + 2;
            break;
        }
    }

    if (strcmp(operation, WRITE_MESSAGE) == 0) {
        char *message = argv[3];
        truncate(filename, total_jpeg_bytes);
        file = fopen(filename, "a");
        fputs(message, file);
    } else if (strcmp(operation, READ_MESSAGE) == 0) {
        for (int i = total_jpeg_bytes; i < file_size; i++) {
            printf("%c", data[i]);
        }
        printf("\n");
    } else if (strcmp(operation, REMOVE_MESSAGE) == 0) {
        truncate(filename, total_jpeg_bytes);
    }

    free(data);

    return 0;
}