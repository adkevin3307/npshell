#include <iostream>

#include "ConnectionOrientedShell.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "Usage: ./np_simple {port}" << '\n';

        return 0;
    }

    ConnectionOrientedShell connection_oriented_shell(atoi(argv[1]));
    connection_oriented_shell.run();

    return 0;
}
