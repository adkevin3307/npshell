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

SingleProcessShell::ClientShell::ClientShell()
{
}

SingleProcessShell::ClientShell::~ClientShell()
{
}

SingleProcessShell::ClientInformation::ClientInformation()
{
    this->name = "(no name)";
}

SingleProcessShell::ClientInformation::ClientInformation(int fd, string ip, string port)
    : SingleProcessShell::ClientInformation::ClientInformation()
{
    this->fd = fd;
    this->ip = ip;
    this->port = port;
}

SingleProcessShell::ClientInformation::~ClientInformation()
{
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

        if (count != 1)
            return false;
        else {
            if (c == '\n')
                break;
            else if (c == 4)
                return false;

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
            int id = 0;
            for (auto it = this->client_map.begin(); it != this->client_map.end(); it++, id++) {
                if (it->first != id) break;
            }

            this->shell_map[client_socket_fd].id = id;
            this->shell_map[client_socket_fd].shell = Shell(client_socket_fd, client_socket_fd, client_socket_fd);

            string ip = inet_ntoa(client_addr.sin_addr);
            string port = to_string(ntohs(client_addr.sin_port));

            this->client_map[id] = ClientInformation(client_socket_fd, ip, port);

            FD_SET(client_socket_fd, &(this->active_fds));

            this->welcome_message(client_socket_fd);

            string info = "*** User '(no name)' entered from " + ip + ":" + port + ". ***\n";

            this->_yell(info);
            write(client_socket_fd, "% ", 2);
        }
    }
}

void SingleProcessShell::_logout(int fd)
{
    close(fd);
    FD_CLR(fd, &(this->active_fds));

    string name = this->client_map[this->shell_map[fd].id].name;

    string s = "*** User '" + name + "' left. ***\n";
    this->_yell(s);

    this->client_map.erase(this->shell_map[fd].id);
    this->shell_map.erase(fd);
}

void SingleProcessShell::_who(int fd)
{
    string title = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
    write(fd, title.c_str(), title.length());

    string client;
    for (auto it = this->client_map.begin(); it != this->client_map.end(); it++) {
        client +=
            to_string(it->first) + "\t" +
            it->second.name + "\t" +
            it->second.ip + ":" + it->second.port + "\t";
        client += ((it->first == this->shell_map[fd].id) ? "<-me" : "");
        client += "\n";
    }

    write(fd, client.c_str(), client.length());
}

void SingleProcessShell::_yell(string s)
{
    for (int fd = 0; fd < this->fds_amount; fd++) {
        if (fd != this->socket_fd && FD_ISSET(fd, &(this->active_fds))) {
            write(fd, s.c_str(), s.length());
        }
    }
}

void SingleProcessShell::_name(int fd, string name)
{
    for (auto it = this->client_map.begin(); it != this->client_map.end(); it++) {
        if (fd == it->second.fd) continue;

        if (name == it->second.name) {
            string s = "*** User '" + name + "' already exists. ***\n";
            write(fd, s.c_str(), s.length());

            return;
        }
    }

    this->client_map[this->shell_map[fd].id].name = name;

    ClientInformation client_information = this->client_map[this->shell_map[fd].id];
    string s = "*** User from " + client_information.ip + ":" + client_information.port + " is named '" + name + "'. ***\n";

    this->_yell(s);
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

                Command command;
                vector<Process> processes = command.parse(buffer);

                if (processes.empty()) continue;

                Constant::BUILTIN builtin_type = processes[0].builtin();

                if (builtin_type == Constant::BUILTIN::EXIT) {
                    this->_logout(fd);

                    continue;
                }
                else if (builtin_type == Constant::BUILTIN::WHO) {
                    this->_who(fd);
                }
                else if (builtin_type == Constant::BUILTIN::TELL) {
                    Process process = command.parse(buffer)[0];

                    int id = atoi(process[1].c_str());

                    int target_fd;
                    string s;

                    if (this->client_map.find(id) != this->client_map.end()) {
                        target_fd = this->client_map[id].fd;
                        s = "*** " + this->client_map[this->shell_map[fd].id].name + " told you ***: " + process[2] + "\n";
                    }
                    else {
                        target_fd = fd;
                        s = "*** Error: user #" + to_string(id) + " does not exist yet. ***\n";
                    }

                    write(target_fd, s.c_str(), s.length());
                }
                else if (builtin_type == Constant::BUILTIN::YELL) {
                    Command command;
                    Process process = command.parse(buffer)[0];

                    string s = "*** " + this->client_map[this->shell_map[fd].id].name + " yelled ***: " + process[1] + '\n';

                    this->_yell(s);
                }
                else if (builtin_type == Constant::BUILTIN::NAME) {
                    Command command;
                    Process process = command.parse(buffer)[0];

                    this->_name(fd, process[1]);
                }
                else if (builtin_type == Constant::BUILTIN::NONE) {
                    this->shell_map[fd].shell.run(processes);

                    write(fd, "% ", 2);
                }
            }
        }
    }
}