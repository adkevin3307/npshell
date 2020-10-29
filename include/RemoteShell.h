#pragma once

#include <vector>
#include <sys/types.h>

using namespace std;

class RemoteShell {
private:
    int socket_fd;
    vector<pid_t> pids;

public:
    RemoteShell(int port);
    ~RemoteShell();

    void run();
};
