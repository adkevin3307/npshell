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
    this->args = NULL;
}

Process::~Process()
{
    if (this->args != NULL) {
        for (size_t i = 0; i < this->command.size() + 1; i++) {
            if (this->args[i] != NULL) {
                free(this->args[i]);
            }
        }

        delete[] this->args;
    }
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

void Process::_cd(string target)
{
    chdir(target.c_str());
}

void Process::_setenv(string target, string source)
{
    setenv(target.c_str(), source.c_str(), 1);
}

void Process::_printenv(string target)
{
    if (getenv(target.c_str())) {
        cout << getenv(target.c_str()) << '\n';
    }
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

Constant::IO Process::type(Constant::IOTARGET target)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            return this->in.type;
        case Constant::IOTARGET::OUT:
            return this->out.type;
        case Constant::IOTARGET::ERR:
            return this->err.type;
        default:
            return Constant::IO::STANDARD;
    }
}

int Process::line(Constant::IOTARGET target)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            return this->in.line;
        case Constant::IOTARGET::OUT:
            return this->out.line;
        case Constant::IOTARGET::ERR:
            return this->err.line;
        default:
            return 0;
    }
}

Constant::BUILTIN Process::builtin()
{
    if (this->command[0] == "exit") {
        return Constant::BUILTIN::EXIT;
    }
    else if (this->command[0] == "cd") {
        if (this->command.size() < 2) {
            this->command.push_back(getenv("HOME"));
        }
        this->_cd(this->command[1]);

        return Constant::BUILTIN::CD;
    }
    else if (this->command[0] == "setenv") {
        if (this->command.size() < 3) {
            cerr << "Invalid arguments" << '\n';
        }
        else {
            this->_setenv(this->command[1], this->command[2]);
        }

        return Constant::BUILTIN::SETENV;
    }
    else if (this->command[0] == "printenv") {
        if (this->command.size() < 2) {
            cerr << "Invalid arguments" << '\n';
        }
        else {
            this->_printenv(this->command[1]);
        }

        return Constant::BUILTIN::PRINTENV;
    }

    return Constant::BUILTIN::NONE;
}

void Process::handle_io(int in, int out)
{
    if (this->err.type == Constant::IO::PIPE) {
        dup2(out, STDERR_FILENO);
    }

    if (in != STDIN_FILENO) {
        dup2(in, STDIN_FILENO);
        close(in);
    }
    if (out != STDOUT_FILENO) {
        dup2(out, STDOUT_FILENO);
        close(out);
    }
}

void Process::exec_check()
{
    this->args = new char*[this->command.size() + 1];

    for (size_t i = 0; i < this->command.size(); i++) {
        this->args[i] = strdup(this->command[i].c_str());
    }
    this->args[this->command.size()] = NULL;

    if (execvp(this->command[0].c_str(), this->args) < 0) {
        cerr << "Unknown command: [" << this->command[0] << "]." << '\n';

        exit(EXIT_FAILURE);
    }
}

void Process::exec(int in, int out, bool enable_fork)
{
    if (this->in.type == Constant::IO::FILE) {
        in = open(this->in.file.c_str(), O_RDONLY);
    }

    if (this->out.type == Constant::IO::FILE) {
        out = open(this->out.file.c_str(), O_TRUNC | O_CREAT | O_WRONLY, 0644);
    }

    if (!enable_fork) {
        this->handle_io(in, out);

        this->exec_check();
    }
    else {
        pid_t pid = fork();

        if (pid == -1) {
            cerr << "Failed to create child" << '\n';
        }
        else if (pid == 0) {
            this->handle_io(in, out);

            this->exec_check();
        }
    }
}
