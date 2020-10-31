#pragma once

#include <vector>
#include <sys/types.h>

using namespace std;

class ConnectionOrientedShell {
private:
    int socket_fd;

public:
    ConnectionOrientedShell(int port);
    ~ConnectionOrientedShell();

    void run();
};
