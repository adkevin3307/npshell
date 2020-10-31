#pragma once

#include <map>

#include "Shell.h"

using namespace std;

class SingleProcessShell {
private:
    int socket_fd, fds_amount;
    fd_set read_fds, active_fds;
    map<int, Shell> shell_map;

    void welcome_message(int fd);
    bool getline(int fd, string& s);
    void _login();
    void _logout(int fd);
    void _yell(string s, int except_fd = -1);

public:
    SingleProcessShell(int port);
    ~SingleProcessShell();

    void run();
};