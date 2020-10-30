#include <iostream>

#include "RemoteShell.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "Usage: ./npshell {port}" << '\n';

        return 0;
    }

    RemoteShell remote_shell(atoi(argv[1]));
    remote_shell.run();

    return 0;
}
