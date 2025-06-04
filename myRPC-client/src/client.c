#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "libmysyslog.h"  // Подключение библиотеки логирования

#define MSG_SIZE 1024

void HELP() {
    printf("Usage: rpcClientApp [OPTIONS]\n");
    printf("Options:\n");
    printf("  -x, --exec \"bash_command\"   Command to execute\n");
    printf("  -a, --address \"ip_addr\"      Server IP address\n");
    printf("  -q, --queue PORT              Server port\n");
    printf("  -t, --tcp                    Use TCP socket\n");
    printf("  -u, --udp                    Use UDP socket\n");
    printf("      --help                   Show this help message\n");
}

int main(int argc, char *argv[]) {
    char *exec_cmd = NULL;
    char *server_addr = NULL;
    int server_port = 0;
    int tcp_mode = 1; // по умолчанию TCP
    int option_char;

    static struct option long_opts[] = {
        {"exec", required_argument, 0, 'x'},
        {"address", required_argument, 0, 'a'},
        {"queue", required_argument, 0, 'q'},
        {"tcp", no_argument, 0, 't'},
        {"udp", no_argument, 0, 'u'},
        {"help", no_argument, 0,  0 },
        {0, 0, 0, 0}
    };

    int opt_idx = 0;
    while ((option_char = getopt_long(argc, argv, "x:a:q:tu", long_opts, &opt_idx)) != -1) {
        switch (option_char) {
            case 'x':
                exec_cmd = optarg;
                break;
            case 'a':
                server_addr = optarg;
                break;
            case 'q':
                server_port = atoi(optarg);
                break;
            case 't':
                tcp_mode = 1;
                break;
            case 'u':
                tcp_mode = 0;
                break;
            case 0:
                HELP();
                return 0;
            default:
                HELP();
                return 1;
        }
    }

    if (!exec_cmd || !server_addr || server_port == 0) {
        fprintf(stderr, "Error: Required arguments missing\n");
        HELP();
        return 1;
    }

    struct passwd *user_info = getpwuid(getuid());
    char *user_name = user_info->pw_name;

    char message[MSG_SIZE];
    snprintf(message, MSG_SIZE, "%s: %s", user_name, exec_cmd);

    mysyslog("Starting connection to server...", INFO, 0, 0, "/var/log/myrpc.log");

    int socket_fd;
    if (tcp_mode) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (socket_fd < 0) {
        mysyslog("Failed to create socket", ERROR, 0, 0, "/var/log/myrpc.log");
        perror("Socket creation error");
        return 1;
    }

    struct sockaddr_in server_info;
    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(server_port);
    inet_pton(AF_INET, server_addr, &server_info.sin_addr);

    if (tcp_mode) {
        if (connect(socket_fd, (struct sockaddr*)&server_info, sizeof(server_info)) < 0) {
            mysyslog("Connection to server failed", ERROR, 0, 0, "/var/log/myrpc.log");
            perror("Connect error");
            close(socket_fd);
            return 1;
        }

        mysyslog("Connected to server successfully", INFO, 0, 0, "/var/log/myrpc.log");

        send(socket_fd, message, strlen(message), 0);

        char recv_buf[MSG_SIZE];
        int bytes_received = recv(socket_fd, recv_buf, MSG_SIZE, 0);
        if (bytes_received >= 0) {
            recv_buf[bytes_received] = '\0';
            printf("Response from server: %s\n", recv_buf);
            mysyslog("Server response received", INFO, 0, 0, "/var/log/myrpc.log");
        }

    } else {
        sendto(socket_fd, message, strlen(message), 0, (struct sockaddr*)&server_info, sizeof(server_info));

        char recv_buf[MSG_SIZE];
        socklen_t addr_len = sizeof(server_info);
        int bytes_received = recvfrom(socket_fd, recv_buf, MSG_SIZE, 0, (struct sockaddr*)&server_info, &addr_len);
        if (bytes_received >= 0) {
            recv_buf[bytes_received] = '\0';
            printf("Response from server: %s\n", recv_buf);
            mysyslog("Server response received", INFO, 0, 0, "/var/log/myrpc.log");
        }
    }

    close(socket_fd);
    return 0;
}
