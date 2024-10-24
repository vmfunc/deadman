#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 12345
#define HEARTBEAT_INTERVAL 5

void *client_handler(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);
    const char *heartbeat_msg = "HEARTBEAT";

    printf("Client handler thread started for socket %d\n", sock);

    while (1) {
        if (send(sock, heartbeat_msg, strlen(heartbeat_msg), 0) < 0) {
            perror("Send failed");
            break;
        }
        printf("Heartbeat sent to socket %d at %ld.\n", sock, time(NULL));

        sleep(HEARTBEAT_INTERVAL);
    }

    printf("Closing socket %d\n", sock);
    close(sock);
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is waiting for connections on port %d...\n", PORT);

    while (1) {
        int new_socket;
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);

        if ((new_socket = accept(server_fd, (struct sockaddr *)&client_address,
                                 &client_addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connected to client: %s:%d\n", inet_ntoa(client_address.sin_addr),
               ntohs(client_address.sin_port));

        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("Could not allocate memory for socket descriptor");
            close(new_socket);
            continue;
        }
        *new_sock = new_socket;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(new_socket);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(server_fd);
    return 0;
}
