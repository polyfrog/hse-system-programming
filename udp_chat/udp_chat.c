#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <regex.h>

#define BUFFER_SIZE 1024
#define MESSAGE_SIZE 256

int udp_socket;
int client_id;

void close_socket(int signum) {
    close(udp_socket);
    exit(EXIT_SUCCESS);
}

void* send_message(void* arg) {
    struct sockaddr_in server_addr = *(struct sockaddr_in *)arg;
    char message[MESSAGE_SIZE];
    
    int client_id_length = snprintf(NULL, 0, "%d", client_id);
    int header_length = client_id_length;
    int payload_length = header_length + MESSAGE_SIZE;

    char* header = malloc(header_length + 1);
    char* client_id_string = malloc(client_id_length + 1);
    snprintf(client_id_string, client_id_length, "%d", client_id);
    strcat(header, client_id_string);

    char* payload = malloc(payload_length + 1);

    while (1) {
        payload[0] = '\0';
        fgets(message, sizeof(message), stdin);

        strcat(payload, header);
        strcat(payload, "\n");
        strcat(payload, message);

        int result = sendto(udp_socket, payload, strlen(payload) + 1, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        free(payload);
    }

    return NULL;
}

void* receive_message() {
    struct sockaddr_in peer_addr;
    char buffer[BUFFER_SIZE];
    int client_id_length = snprintf(NULL, 0, "%d", client_id);
    char* client_id_string = malloc(client_id_length + 1);
    snprintf(client_id_string, client_id_length, "%d", client_id);

    socklen_t address_length = sizeof(peer_addr);

    while (1) {
        int bytes_received = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&peer_addr, &address_length);

        if (bytes_received == 0) {
            printf("Failed receiving packet");
            exit(EXIT_FAILURE);
        }

        if (strcmp(strtok(buffer, "\n"), client_id_string)) {
            printf("Received packet from %s, message: %s\n", inet_ntoa(peer_addr.sin_addr), strtok(NULL, "\n"));
        }
    }

    return NULL;
}

int set_socket_option(int option) {
    const int enable = 1;
    int socket_options = setsockopt(udp_socket, SOL_SOCKET, option, &enable, sizeof(enable));
    if (socket_options == -1) {
        perror("Failed setting SO_BROADCAST socket option");
        return 1;
    }
    return 0;
}

int generate_client_id() {
    long int rand_seed = time(NULL) + getpid();
    srand(rand_seed);
    client_id = rand();

    return client_id;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage: %s [port_number]\n", argv[0]);
        return 1;
    }

    int port_number = atoi(argv[1]);
    const char *broadcast_address = "172.24.255.255";
    struct sockaddr_in server_addr = {.sin_family = AF_INET, .sin_port = htons(port_number)};

    client_id = generate_client_id();

    if (inet_pton(AF_INET, broadcast_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid IP");
        return 1;
    }

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("Failed creating socket");
        return 1;
    }

    set_socket_option(SO_BROADCAST);
    set_socket_option(SO_REUSEADDR);

    int bind_result = bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (bind_result != 0) {
        perror("Failed to bind socket to address");
        return 1;
    }

    signal(SIGINT, close_socket);

    pthread_t send_thread, receive_thread;

    pthread_create(&send_thread, NULL, send_message, (void *)&server_addr);
    pthread_create(&receive_thread, NULL, receive_message, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    close(udp_socket);

    return 0;
}