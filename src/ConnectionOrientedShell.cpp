#include "ConnectionOrientedShell.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <arpa/inet.h>

#include "Shell.h"

using namespace std;

namespace Client {
    vector<pid_t> pids;
};

ConnectionOrientedShell::ConnectionOrientedShell(int port)
{
    signal(SIGCHLD, [](int signo) {
        pid_t wpid;
        for (int i = Client::pids.size() - 1; i >= 0; i--) {
            wpid = waitpid(Client::pids[i], NULL, WNOHANG);

            if (wpid == Client::pids[i]) {
                Client::pids.erase(Client::pids.begin() + i);
            }
        }
    });

    this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socket_fd < 0) {
        cerr << "Server: cannot open stream socket" << '\n';
    }

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(this->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Server: cannot bind local address" << '\n';
    }

    listen(this->socket_fd, 5);
}

ConnectionOrientedShell::~ConnectionOrientedShell()
{
    close(this->socket_fd);

    Client::pids.clear();
    Client::pids.shrink_to_fit();
}

void ConnectionOrientedShell::run()
{
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket_fd < 0) {
            cerr << "Server: accept error" << '\n';
        }

        pid_t pid = fork();

        if (pid < 0) {
            cerr << "Server: fork error" << '\n';
        }
        else if (pid == 0) {
            close(this->socket_fd);

            dup2(client_socket_fd, STDIN_FILENO);
            dup2(client_socket_fd, STDOUT_FILENO);
            dup2(client_socket_fd, STDERR_FILENO);
            close(client_socket_fd);

            Shell shell;
            shell.run();

            exit(EXIT_SUCCESS);
        }
        else {
            close(client_socket_fd);

            Client::pids.push_back(pid);
        }
    }
}
