#include "SingleProcessShell.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

SingleProcessShell::SingleProcessShell(int port)
{
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

SingleProcessShell::~SingleProcessShell()
{
    close(this->socket_fd);
}

void SingleProcessShell::welcome_message(int fd, struct sockaddr_in client)
{
    string address = inet_ntoa(client.sin_addr);
    string port = to_string(ntohs(client.sin_port));

    string message =
        "***************************************\n"
        "** Welcome to the information server **\n"
        "***************************************\n";

    string info = "*** User ’(no name)’ entered from " + address + ":" + port + ". ***\n";

    write(fd, message.c_str(), message.length());
    write(fd, info.c_str(), info.length());
    write(fd, "% ", 2);
}

void SingleProcessShell::run()
{
    fd_set read_fds, active_fds;
    int fds_amount = getdtablesize();

    FD_ZERO(&active_fds);
    FD_SET(this->socket_fd, &active_fds);

    while (true) {
        memcpy(&read_fds, &active_fds, sizeof(read_fds));

        if (select(fds_amount, &read_fds, NULL, NULL, NULL) < 0) {
            cerr << "Server: select error" << '\n';
        }

        if (FD_ISSET(this->socket_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len);

            if (client_socket_fd < 0) {
                cerr << "Server: accept error" << '\n';
            }

            if (this->shell_map.find(client_socket_fd) == this->shell_map.end()) {
                this->shell_map[client_socket_fd] = Shell(client_socket_fd, client_socket_fd, client_socket_fd);
            }

            this->welcome_message(client_socket_fd, client_addr);

            FD_SET(client_socket_fd, &active_fds);
        }

        for (int fd = 0; fd < fds_amount; fd++) {
            if (fd != this->socket_fd && FD_ISSET(fd, &read_fds)) {
                string buffer;

                while (true) {
                    char c;
                    int count = read(fd, &c, 1);

                    if (count != 1) break;
                    else {
                        if (c == '\n') break;

                        buffer += c;
                    }
                }

                if (!this->shell_map[fd].run(buffer)) {
                    close(fd);
                    FD_CLR(fd, &active_fds);

                    continue;
                }

                write(fd, "% ", 2);
            }
        }
    }
}