#include "SingleProcessShell.h"

#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
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
    FD_SET(STDIN_FILENO, &(this->active_fds));
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
        "****************************************\n"
        "** Welcome to the information server. **\n"
        "****************************************\n";

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

    s = Command::trim(s);

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

        int id = 1;
        for (auto it = this->client_map.begin(); it != this->client_map.end(); it++, id++) {
            if (it->first != id) break;
        }

        this->shell_map[client_socket_fd].id = id;
        this->shell_map[client_socket_fd].shell = Shell(client_socket_fd, client_socket_fd, client_socket_fd);

        Process env;
        env.add("setenv");
        env.add("PATH");
        env.add("bin:.");
        this->shell_map[client_socket_fd].envs.push_back(env);

        string ip = inet_ntoa(client_addr.sin_addr);
        string port = to_string(ntohs(client_addr.sin_port));

        this->client_map[id] = ClientInformation(client_socket_fd, ip, port);

        FD_SET(client_socket_fd, &(this->active_fds));

        this->welcome_message(client_socket_fd);

        string info = "*** User \'(no name)\' entered from " + ip + ":" + port + ". ***\n";

        this->_yell(info);
        write(client_socket_fd, "% ", 2);
    }
}

void SingleProcessShell::_logout(int fd)
{
    close(fd);
    FD_CLR(fd, &(this->active_fds));

    int id = this->shell_map[fd].id;

    string s = "*** User \'" + this->client_map[id].name + "\' left. ***\n";
    this->_yell(s);

    this->shell_map[fd].envs.clear();
    this->shell_map.erase(fd);

    for (auto user_pipe : this->client_map[id].user_pipes) {
        close(user_pipe.second.fd[0]);
        close(user_pipe.second.fd[1]);
    }
    this->client_map.erase(id);

    for (auto it = this->client_map.begin(); it != this->client_map.end(); it++) {
        if (it->second.user_pipes.find(id) != it->second.user_pipes.end()) {
            close(it->second.user_pipes[id].fd[0]);
            close(it->second.user_pipes[id].fd[1]);

            it->second.user_pipes.erase(id);
        }
    }
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
            string s = "*** User \'" + name + "\' already exists. ***\n";
            write(fd, s.c_str(), s.length());

            return;
        }
    }

    this->client_map[this->shell_map[fd].id].name = name;

    ClientInformation client_information = this->client_map[this->shell_map[fd].id];
    string s = "*** User from " + client_information.ip + ":" + client_information.port + " is named \'" + name + "\'. ***\n";

    this->_yell(s);
}

Constant::BUILTIN SingleProcessShell::builtin(int fd, string buffer, Process process)
{
    string error_message = "Invalid arguments\n";
    Constant::BUILTIN builtin_type = process.builtin(fd, fd, fd);

    switch (builtin_type) {
        case Constant::BUILTIN::EXIT:
            this->_logout(fd);

            break;
        case Constant::BUILTIN::SETENV:
            this->shell_map[fd].envs.push_back(process);

            break;
        case Constant::BUILTIN::WHO:
            this->_who(fd);

            break;
        case Constant::BUILTIN::TELL: {
            if (process.size() < 3) {
                write(fd, error_message.c_str(), error_message.length());
            }
            else {
                int id = atoi(process[1].c_str());

                if (this->client_map.find(id) != this->client_map.end()) {
                    stringstream ss;
                    ss << buffer;

                    string trash;
                    ss >> trash >> trash;

                    string text;
                    std::getline(ss, text);
                    text = Command::trim(text);

                    string s = "*** " + this->client_map[this->shell_map[fd].id].name + " told you ***: " + text + "\n";

                    write(this->client_map[id].fd, s.c_str(), s.length());
                }
                else {
                    string s = "*** Error: user #" + to_string(id) + " does not exist yet. ***\n";
                    write(fd, s.c_str(), s.length());
                }
            }

            break;
        }
        case Constant::BUILTIN::YELL: {
            if (process.size() < 2) {
                write(fd, error_message.c_str(), error_message.length());
            }
            else {
                stringstream ss;
                ss << buffer;

                string trash;
                ss >> trash;

                string text;
                std::getline(ss, text);
                text = Command::trim(text);

                string s = "*** " + this->client_map[this->shell_map[fd].id].name + " yelled ***: " + text + "\n";

                this->_yell(s);
            }

            break;
        }
        case Constant::BUILTIN::NAME:
            if (process.size() < 2) {
                write(fd, error_message.c_str(), error_message.length());
            }
            else {
                this->_name(fd, process[1]);
            }

            break;
        default:
            break;
    }

    return builtin_type;
}

bool SingleProcessShell::get_pipe(int fd, int& in, int& out, string buffer)
{
    Command command;
    vector<Process> processes = command.parse(buffer);

    bool executable = true;

    if (processes.front().type(Constant::IOTARGET::IN) == Constant::IO::U_PIPE) {
        int source_id = processes.front().n(Constant::IOTARGET::IN);

        if (this->client_map.find(source_id) == this->client_map.end()) {
            executable = false;

            stringstream ss;
            ss << "*** Error: user #" << source_id << " does not exist yet. ***" << '\n';

            write(fd, ss.str().c_str(), ss.str().length());
        }
        else {
            bool pipe_exist = false;
            int id = this->shell_map[fd].id;

            if (this->client_map[id].user_pipes.find(source_id) != this->client_map[id].user_pipes.end()) {
                close(this->client_map[id].user_pipes[source_id].fd[1]);
                in = this->client_map[id].user_pipes[source_id].fd[0];

                pipe_exist = true;

                this->recycle_pipes.push_back(this->client_map[id].user_pipes[source_id]);
                this->client_map[id].user_pipes.erase(source_id);
            }

            if (pipe_exist) {
                stringstream ss;
                ss << "*** " << this->client_map[id].name << " (#" << id << ") just received from "
                   << this->client_map[source_id].name << " (#" << source_id << ") by \'" << buffer << "\' ***" << '\n';

                this->_yell(ss.str());
            }
            else {
                executable = false;

                stringstream ss;
                ss << "*** Error: the pipe #" << source_id << "->#" << id << " does not exist yet. ***" << '\n';

                write(fd, ss.str().c_str(), ss.str().length());
            }
        }
    }

    if (processes.back().type(Constant::IOTARGET::OUT) == Constant::IO::U_PIPE) {
        int target_id = processes.back().n(Constant::IOTARGET::OUT);

        if (this->client_map.find(target_id) == this->client_map.end()) {
            executable = false;

            stringstream ss;
            ss << "*** Error: user #" << target_id << " does not exist yet. ***" << '\n';

            write(fd, ss.str().c_str(), ss.str().length());
        }
        else {
            bool pipe_exist = false;
            int id = this->shell_map[fd].id;

            if (this->client_map[target_id].user_pipes.find(id) != this->client_map[target_id].user_pipes.end()) {
                stringstream ss;
                ss << "*** Error: the pipe #" << id << "->#" << target_id << " already exists. ***" << '\n';

                write(fd, ss.str().c_str(), ss.str().length());

                pipe_exist = true;
            }

            if (pipe_exist) {
                executable = false;
            }
            else {
                int pipe_fd[2];

                if (pipe(pipe_fd) < 0) {
                    cerr << "Pipe cannot be initialized" << '\n';

                    exit(EXIT_FAILURE);
                }

                out = pipe_fd[1];

                PipeElement pipe_element;
                pipe_element.fd[0] = pipe_fd[0];
                pipe_element.fd[1] = pipe_fd[1];
                pipe_element.n = this->shell_map[fd].id;

                this->client_map[target_id].user_pipes[id] = pipe_element;

                stringstream ss;
                ss << "*** " << this->client_map[id].name << " (#" << id << ") just piped \'"
                   << buffer << "\' to " << this->client_map[target_id].name << " (#" << target_id << ") ***" << '\n';

                this->_yell(ss.str());
            }
        }
    }

    return executable;
}

void SingleProcessShell::run()
{
    while (true) {
        memcpy(&(this->read_fds), &(this->active_fds), sizeof(this->read_fds));

        if (select(this->fds_amount, &(this->read_fds), NULL, NULL, NULL) < 0) {
            cerr << "Server: select error" << '\n';
        }

        if (FD_ISSET(STDIN_FILENO, &(this->read_fds))) {
            string s;
            if (!std::getline(cin, s)) break;

            continue;
        }

        this->_login();

        for (int fd = 0; fd < this->fds_amount; fd++) {
            if (fd != STDIN_FILENO && fd != this->socket_fd && FD_ISSET(fd, &(this->read_fds))) {
                string buffer;

                if (!this->getline(fd, buffer)) {
                    this->_logout(fd);

                    continue;
                }

                Command command;
                vector<Process> processes = command.parse(buffer);

                if (processes.empty()) {
                    write(fd, "% ", 2);

                    continue;
                }

                this->shell_map[fd].shell.next_line();

                for (auto env : this->shell_map[fd].envs) {
                    env.builtin(fd, fd, fd);
                }

                Constant::BUILTIN builtin_type = this->builtin(fd, buffer, processes[0]);

                if (builtin_type == Constant::BUILTIN::EXIT) continue;
                if (builtin_type == Constant::BUILTIN::NONE) {
                    int in = fd, out = fd;

                    if (this->get_pipe(fd, in, out, buffer)) {
                        this->shell_map[fd].shell.set(Constant::IOTARGET::IN, in);
                        this->shell_map[fd].shell.set(Constant::IOTARGET::OUT, out);

                        pid_t child_pid = this->shell_map[fd].shell.run(processes);

                        if (processes.front().type(Constant::IOTARGET::IN) == Constant::IO::U_PIPE && child_pid == 0) {
                            for (auto element : this->recycle_pipes) {
                                for (auto pid : element.pids) {
                                    kill(pid, SIGINT);

                                    pid_t wpid;
                                    while (true) {
                                        wpid = waitpid(pid, NULL, WNOHANG);

                                        if (wpid == pid) break;
                                    }
                                }

                                element.pids.clear();
                            }

                            this->recycle_pipes.clear();
                        }

                        if (processes.back().type(Constant::IOTARGET::OUT) == Constant::IO::U_PIPE && child_pid > 0) {
                            int id = this->shell_map[fd].id;
                            int target_id = processes.back().n(Constant::IOTARGET::OUT);

                            this->client_map[target_id].user_pipes[id].pids.push_back(child_pid);
                        }

                        if (in != fd) {
                            close(in);
                        }
                    }
                }

                write(fd, "% ", 2);
            }
        }
    }
}
