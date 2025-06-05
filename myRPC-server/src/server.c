#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "settings_reader.h"
#include "libmysyslog.h"  // Логирование системы

#define MAX_BUF_LEN 1024  // Максимальный размер буфера для данных

volatile sig_atomic_t terminate_flag;

//Обработчик сигналов для корректного завершения работы приложения
void signal_handler(int signal_code) {
    terminate_flag = 1;
}

// Проверяка наличия пользователя в списке разрешённых
int is_user_permitted(const char *user) {
    FILE *conf_file = fopen("/etc/myRPC/config_files/users.conf", "r");
    if (!conf_file) {
        mysyslog("Cannot open users.conf", ERROR, 0, 0, "/var/log/myrpc.log");
        perror("Cannot open users.conf");
        return 0;
    }

    char line_buf[256];
    int permitted = 0;

    while (fgets(line_buf, sizeof(line_buf), conf_file)) {
        line_buf[strcspn(line_buf, "\n")] = '\0';

        if (line_buf[0] == '#' || strlen(line_buf) == 0)
            continue;

        if (strcmp(line_buf, user) == 0) {
            permitted = 1;
            break;
        }
    }

    fclose(conf_file);
    return permitted;
}

// Перенаправление stdout и stderr в файлы
void run_system_command(const char *cmd, char *out_file, char *err_file) {
    char full_cmd[MAX_BUF_LEN];
    snprintf(full_cmd, MAX_BUF_LEN, "%s >%s 2>%s", cmd, out_file, err_file);
    system(full_cmd);
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    Config srv_config = parse_config("/etc/myRPC/config_files/myRPC.conf");
    int srv_port = srv_config.port;
    int is_stream_socket = strcmp(srv_config.socket_type, "stream") == 0;

    mysyslog("Server initialization started", INFO, 0, 0, "/var/log/myrpc.log");

    int server_sock;
    if (is_stream_socket) {
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (server_sock < 0) {
        mysyslog("Failed to create socket", ERROR, 0, 0, "/var/log/myrpc.log");
        perror("Failed to create socket");
        return 1;
    }

    int reuse_flag = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_flag, sizeof(reuse_flag)) < 0) {
        mysyslog("setsockopt error", ERROR, 0, 0, "/var/log/myrpc.log");
        perror("setsockopt error");
        close(server_sock);
        return 1;
    }

    struct sockaddr_in address_server, address_client;
    socklen_t addr_len;
    memset(&address_server, 0, sizeof(address_server));
    address_server.sin_family = AF_INET;
    address_server.sin_addr.s_addr = INADDR_ANY;
    address_server.sin_port = htons(srv_port);

    if (bind(server_sock, (struct sockaddr*)&address_server, sizeof(address_server)) < 0) {
        mysyslog("Bind error", ERROR, 0, 0, "/var/log/myrpc.log");
        perror("Bind error");
        close(server_sock);
        return 1;
    }

    if (is_stream_socket) {
        listen(server_sock, 5);
        mysyslog("Listening on stream socket", INFO, 0, 0, "/var/log/myrpc.log");
    } else {
        mysyslog("Listening on datagram socket", INFO, 0, 0, "/var/log/myrpc.log");
    }

    while (!terminate_flag) {
        char recv_buffer[MAX_BUF_LEN];
        int bytes_count;

        if (is_stream_socket) {
            addr_len = sizeof(address_client);
            int client_fd = accept(server_sock, (struct sockaddr*)&address_client, &addr_len);
            if (client_fd < 0) {
                mysyslog("Accept error", ERROR, 0, 0, "/var/log/myrpc.log");
                perror("Accept error");
                continue;
            }

            bytes_count = recv(client_fd, recv_buffer, MAX_BUF_LEN, 0);
            if (bytes_count <= 0) {
                close(client_fd);
                continue;
            }
            recv_buffer[bytes_count] = '\0';

            mysyslog("Request received", INFO, 0, 0, "/var/log/myrpc.log");

            char *user_name = strtok(recv_buffer, ":");
            char *cmd_text = strtok(NULL, "");
            if (cmd_text) {
                while (*cmd_text == ' ')
                    cmd_text++;
            }

            char reply[MAX_BUF_LEN];

            if (is_user_permitted(user_name)) {
                mysyslog("User permitted", INFO, 0, 0, "/var/log/myrpc.log");

                char out_tmp[] = "/tmp/myRPC_XXXXXX.out";
                char err_tmp[] = "/tmp/myRPC_XXXXXX.err";
                mkstemp(out_tmp);
                mkstemp(err_tmp);

                run_system_command(cmd_text, out_tmp, err_tmp);

                FILE *out_f = fopen(out_tmp, "r");
                if (out_f) {
                    size_t read_bytes = fread(reply, 1, MAX_BUF_LEN, out_f);
                    reply[read_bytes] = '\0';
                    fclose(out_f);
                    mysyslog("Command executed successfully", INFO, 0, 0, "/var/log/myrpc.log");
                } else {
                    strcpy(reply, "Failed to read stdout");
                    mysyslog("Failed to read stdout", ERROR, 0, 0, "/var/log/myrpc.log");
                }

                remove(out_tmp);
                remove(err_tmp);

            } else {
                snprintf(reply, MAX_BUF_LEN, "1: User '%s' is not permitted", user_name);
                mysyslog("User not permitted", WARN, 0, 0, "/var/log/myrpc.log");
            }

            send(client_fd, reply, strlen(reply), 0);
            close(client_fd);

        } else {
            addr_len = sizeof(address_client);
            bytes_count = recvfrom(server_sock, recv_buffer, MAX_BUF_LEN, 0,
                                   (struct sockaddr*)&address_client, &addr_len);
            if (bytes_count <= 0) {
                continue;
            }
            recv_buffer[bytes_count] = '\0';

            mysyslog("Request received", INFO, 0, 0, "/var/log/myrpc.log");

            char *user_name = strtok(recv_buffer, ":");
            char *cmd_text = strtok(NULL, "");
            if (cmd_text) {
                while (*cmd_text == ' ')
                    cmd_text++;
            }

            char reply[MAX_BUF_LEN];

            if (is_user_permitted(user_name)) {
                mysyslog("User permitted", INFO, 0, 0, "/var/log/myrpc.log");

                char out_tmp[] = "/tmp/myRPC_XXXXXX.out";
                char err_tmp[] = "/tmp/myRPC_XXXXXX.err";
                mkstemp(out_tmp);
                mkstemp(err_tmp);

                run_system_command(cmd_text, out_tmp, err_tmp);

                FILE *out_f = fopen(out_tmp, "r");
                if (out_f) {
                    size_t read_bytes = fread(reply, 1, MAX_BUF_LEN, out_f);
                    reply[read_bytes] = '\0';
                    fclose(out_f);
                    mysyslog("Command executed successfully", INFO, 0, 0, "/var/log/myrpc.log");
                } else {
                    strcpy(reply, "Failed to read stdout");
                    mysyslog("Failed to read stdout", ERROR, 0, 0, "/var/log/myrpc.log");
                }

                remove(out_tmp);
                remove(err_tmp);

            } else {
                snprintf(reply, MAX_BUF_LEN, "1: User '%s' is not permitted", user_name);
                mysyslog("User not permitted", WARN, 0, 0, "/var/log/myrpc.log");
            }

            sendto(server_sock, reply, strlen(reply), 0,
                   (struct sockaddr*)&address_client, addr_len);
        }
    }

    close(server_sock);
    mysyslog("Server shutdown complete", INFO, 0, 0, "/var/log/myrpc.log");
    return 0;
}
