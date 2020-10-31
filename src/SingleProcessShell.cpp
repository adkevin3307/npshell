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
    int port = ntohs(client.sin_port);

    string message =
        "***************************************\n"
        "** Welcome to the information server **\n"
        "***************************************\n";

    string info = "*** User ’(no name)’ entered from " + address + ":" + to_string(port) + ". ***\n";

    string prompt = "% ";

    write(fd, message.c_str(), message.length());
    write(fd, info.c_str(), info.length());
    write(fd, prompt.c_str(), prompt.length());
}

void SingleProcessShell::run()
{
    fd_set rfds, afds;
    int nfds = getdtablesize();

    FD_ZERO(&afds);
    FD_SET(this->socket_fd, &afds);

    while (true) {
        memcpy(&rfds, &afds, sizeof(rfds));

        if (select(nfds, &rfds, NULL, NULL, NULL) < 0) {
            cerr << "Server: select error" << '\n';
        }

        if (FD_ISSET(this->socket_fd, &rfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len);

            if (client_socket_fd < 0) {
                cerr << "Server: accept error" << '\n';
            }

            if (this->shell_map.find(client_socket_fd) == this->shell_map.end()) {
                Shell shell;

                this->shell_map[client_socket_fd] = shell;
            }

            this->welcome_message(client_socket_fd, client_addr);

            FD_SET(client_socket_fd, &afds);
        }

        for (int fd = 0; fd < nfds; fd++) {
            if (fd != this->socket_fd && FD_ISSET(fd, &rfds)) {
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

                this->shell_map[fd].run(buffer);

                write(fd, "% ", 2);

                // close(fd);
                // FD_CLR(fd, &afds);
            }
        }
    }
}