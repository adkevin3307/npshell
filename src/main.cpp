#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Shell.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "Usage: ./npshell {port}" << '\n';

        return 0;
    }

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        cerr << "Server: Cannot open stream socket" << '\n';
    }

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Server: Cannot bind local address" << '\n';
    }

    listen(socket_fd, 5);

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int new_socket_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &client_len);

        if (new_socket_fd < 0) {
            cerr << "Server: Accept error" << '\n';
        }

        pid_t pid = fork();

        if (pid < 0) {
            cerr << "Server: Fork error" << '\n';
        }
        else if (pid == 0) {
            close(socket_fd);

            dup2(new_socket_fd, STDIN_FILENO);
            dup2(new_socket_fd, STDOUT_FILENO);
            dup2(new_socket_fd, STDERR_FILENO);

            Shell shell;
            shell.run();

            exit(EXIT_SUCCESS);
        }
        else {
            close(new_socket_fd);
        }
    }
}
