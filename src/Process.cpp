#include "Process.h"

#include <cstring>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

using namespace std;

Process::Process()
{
}

Process::~Process()
{
}

Process::IOManagement::IOManagement()
{
    this->type = Constant::IO::STANDARD;
    this->line = 0;
    this->file = "";
}

Process::IOManagement::~IOManagement()
{
}

void Process::_setenv(string target, string source)
{
    setenv(target.c_str(), source.c_str(), 1);
}

void Process::_printenv(string target)
{
    cout << getenv(target.c_str()) << '\n';
}

void Process::_exit()
{
    exit(EXIT_SUCCESS);
}

void Process::add(string argument)
{
    this->command.push_back(argument);
}

void Process::set(Constant::IOTARGET target, Constant::IO type)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            this->in.type = type;
            break;
        case Constant::IOTARGET::OUT:
            this->out.type = type;
            break;
        case Constant::IOTARGET::ERR:
            this->err.type = type;
            break;
        default:
            break;
    }
}

void Process::set(Constant::IOTARGET target, int line)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            this->in.line = line;
            break;
        case Constant::IOTARGET::OUT:
            this->out.line = line;
            break;
        case Constant::IOTARGET::ERR:
            this->err.line = line;
            break;
        default:
            break;
    }
}

void Process::set(Constant::IOTARGET target, string file)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            this->in.file = file;
            break;
        case Constant::IOTARGET::OUT:
            this->out.file = file;
            break;
        case Constant::IOTARGET::ERR:
            this->err.file = file;
            break;
        default:
            break;
    }
}

bool Process::builtin()
{
    vector<string> builtin_function{ "setenv", "printenv", "exit" };

    if (find(builtin_function.begin(), builtin_function.end(), this->command[0]) != builtin_function.end()) {
        if (this->command[0] == "exit") {
            this->_exit();
        }
        else if (this->command[0] == "setenv") {
            if (this->command.size() < 3)
                cerr << "Invalid arguments" << '\n';
            else
                this->_setenv(this->command[1], this->command[2]);
        }
        else if (this->command[0] == "printenv") {
            if (this->command.size() < 2)
                cerr << "Invalid arguments" << '\n';
            else
                this->_printenv(this->command[1]);
        }

        return true;
    }

    return false;
}

void Process::exec_check()
{
    char** args = new char*[this->command.size() + 1];

    for (size_t i = 0; i < this->command.size(); i++) {
        args[i] = strdup(this->command[i].c_str());
    }
    args[this->command.size()] = NULL;

    if (execvp(this->command[0].c_str(), args) < 0) {
        cerr << "Unknown command: [" << this->command[0] << "]." << '\n';

        exit(EXIT_FAILURE);
    }
}

void Process::exec(int in, int out, bool enable_fork)
{
    if (this->out.type == Constant::IO::FILE) {
        out = open(this->out.file.c_str(), O_TRUNC | O_CREAT | O_WRONLY, 0644);
    }

    if (!enable_fork) {
        if (in != STDIN_FILENO) {
            dup2(in, STDIN_FILENO);
        }

        if (out != STDOUT_FILENO) {
            dup2(out, STDOUT_FILENO);
        }

        this->exec_check();
    }
    else {
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

            this->exec_check();
        }
        else {
            int status;
            while (true) {
                wpid = waitpid(pid, &status, WNOHANG);

                if (wpid == pid) break;
            }
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