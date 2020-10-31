#include "SingleProcessShell.h"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Command.h"

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

    this->fds_amount = getdtablesize();

    FD_ZERO(&(this->active_fds));
    FD_SET(this->socket_fd, &(this->active_fds));
}

SingleProcessShell::~SingleProcessShell()
{
    close(this->socket_fd);
}

void SingleProcessShell::welcome_message(int fd)
{
    string message =
        "***************************************\n"
        "** Welcome to the information server **\n"
        "***************************************\n";

    write(fd, message.c_str(), message.length());
}

bool SingleProcessShell::getline(int fd, string& s)
{
    while (true) {
        char c;
        int count = read(fd, &c, 1);

        if (count != 1) return false;
        else {
            if (c == '\n') break;
            else if (c == 4) return false; // EOF

            s += c;
        }
    }

    return true;
}

void SingleProcessShell::_login()
{
    if (FD_ISSET(this->socket_fd, &(this->read_fds))) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket_fd = accept(this->socket_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket_fd < 0) {
            cerr << "Server: accept error" << '\n';
        }

        if (this->shell_map.find(client_socket_fd) == this->shell_map.end()) {
            this->shell_map[client_socket_fd] = Shell(client_socket_fd, client_socket_fd, client_socket_fd);
        }

        this->welcome_message(client_socket_fd);

        FD_SET(client_socket_fd, &(this->active_fds));

        string address = inet_ntoa(client_addr.sin_addr);
        string port = to_string(ntohs(client_addr.sin_port));

        string info = "*** User '(no name)' entered from " + address + ":" + port + ". ***\n";

        this->_yell(info);
        write(client_socket_fd, "% ", 2);
    }
}

void SingleProcessShell::_logout(int fd)
{
    close(fd);
    FD_CLR(fd, &(this->active_fds));

    string s = "*** User '(no name)' left. ***\n";

    this->_yell(s);
}

void SingleProcessShell::_yell(string s, int except_fd)
{
    for (int fd = 0; fd < this->fds_amount; fd++) {
        if (fd != this->socket_fd && FD_ISSET(fd, &(this->active_fds))) {
            if (except_fd != -1 && fd == except_fd) continue;

            write(fd, s.c_str(), s.length());
        }
    }
}

void SingleProcessShell::run()
{
    while (true) {
        memcpy(&(this->read_fds), &(this->active_fds), sizeof(this->read_fds));

        if (select(this->fds_amount, &(this->read_fds), NULL, NULL, NULL) < 0) {
            cerr << "Server: select error" << '\n';
        }

        this->_login();

        for (int fd = 0; fd < this->fds_amount; fd++) {
            if (fd != this->socket_fd && FD_ISSET(fd, &(this->read_fds))) {
                string buffer;

                if (!this->getline(fd, buffer)) {
                    this->_logout(fd);

                    continue;
                }

                Constant::BUILTIN builtin_type = this->shell_map[fd].run(buffer);

                if (builtin_type == Constant::BUILTIN::EXIT) {
                    this->_logout(fd);

                    continue;
                }
                else if (builtin_type == Constant::BUILTIN::WHO) {

                }
                else if (builtin_type == Constant::BUILTIN::TELL) {
                    Command command;
                    Process process = command.parse(buffer)[0];

                    // TODO tell
                }
                else if (builtin_type == Constant::BUILTIN::YELL) {
                    Command command;
                    Process process = command.parse(buffer)[0];

                    string s = "*** (no name) yelled ***: " + process[1] + '\n';

                    this->_yell(s);
                }
                else if (builtin_type == Constant::BUILTIN::NAME) {
                    Command command;
                    Process process = command.parse(buffer)[0];

                    // TODO name
                }

                write(fd, "% ", 2);
            }
        }
    }
}