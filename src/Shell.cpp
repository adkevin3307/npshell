#include "Shell.h"

#include <iostream>

#include "Command.h"

using namespace std;

Shell::Shell()
{
}

Shell::~Shell()
{
}

void Shell::run()
{
    string buffer;
    Command command;

    while (true) {
        cout << "% ";

        if (!getline(cin, buffer)) break;

        command.parse(buffer);
        if (command.commands().empty()) continue;
    }
}
