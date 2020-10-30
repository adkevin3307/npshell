#pragma once

#include <vector>
#include <sys/types.h>

using namespace std;

class RemoteShell {
private:
    int socket_fd;

public:
    RemoteShell(int port);
    ~RemoteShell();

    void run();
};
