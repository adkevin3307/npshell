#include "Process.h"

#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

Process::Process()
{
}

Process::~Process()
{
}

void Process::add(string argument)
{
    this->command.push_back(argument);
}

void Process::exec(int in, int out)
{
    char** args = new char*[this->command.size() + 1];

    for (size_t i = 0; i < this->command.size(); i++) {
        args[i] = strdup(this->command[i].c_str());
    }
    args[this->command.size()] = NULL;

    pid_t pid, wpid;

    pid = fork();

    if (pid == -1) {
        cerr << "Failed to create child" << '\n';
    }
    else if (pid == 0) {
        if (in != STDIN_FILENO) {
            dup2(in, STDIN_FILENO);
            close(in);
        }
        if (out != STDOUT_FILENO) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }

        if (execvp(this->command[0].c_str(), args) < 0) {
            cerr << "Unknown command: [" << this->command[0] << "]." << '\n';

            exit(EXIT_FAILURE);
        }
    }
    else {
        int status;
        while (true) {
            wpid = waitpid(pid, &status, WNOHANG);

            if (wpid == pid) break;
        }
    }
}

ostream& operator<<(ostream& os, Process& process)
{
    os << "Process: ";
    for (size_t i = 0; i < process.command.size(); i++) {
        os << process.command[i] << (i == process.command.size() - 1 ? '\n' : ' ');
    }

    return os;
}