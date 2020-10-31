#include <iostream>

#include "SingleProcessShell.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cerr << "Usage: ./np_single_proc {port}" << '\n';

        return 0;
    }

    SingleProcessShell single_process_shell(atoi(argv[1]));
    single_process_shell.run();

    return 0;
}