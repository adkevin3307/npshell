#pragma once

#include <map>

#include "Shell.h"

using namespace std;

class SingleProcessShell {
private:
    int socket_fd;
    map<int, Shell> shell_map;

    void welcome_message(int fd, struct sockaddr_in client);

public:
    SingleProcessShell(int port);
    ~SingleProcessShell();

    void run();
};