#pragma once

#include <map>
#include <vector>

#include "PipeElement.h"
#include "Shell.h"
#include "Process.h"

using namespace std;

class SingleProcessShell {
private:
    class ClientShell {
    private:
        int id;
        Shell shell;

    public:
        ClientShell();
        ~ClientShell();

        friend class SingleProcessShell;
    };

    class ClientInformation {
    private:
        int fd;
        string name, ip, port;
        vector<PipeElement> user_pipes;

    public:
        ClientInformation();
        ClientInformation(int fd, string ip, string port);
        ~ClientInformation();

        friend class SingleProcessShell;
    };

    int socket_fd, fds_amount;
    fd_set read_fds, active_fds;
    map<int, ClientShell> shell_map;
    map<int, ClientInformation> client_map;

    void welcome_message(int fd);
    bool getline(int fd, string& s);
    void _login();
    void _logout(int fd);
    void _who(int fd);
    void _yell(string s);
    void _name(int fd, string s);
    Constant::BUILTIN builtin(int fd, Process process);
    bool get_pipe(int fd, int& in, int& out, int& user_pipe_index, string buffer);

public:
    SingleProcessShell(int port);
    ~SingleProcessShell();

    void run();
};