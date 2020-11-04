#include "Process.h"

#include <cstring>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <fcntl.h>
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
        this->args = NULL;
    }
}

Process::IOManagement::IOManagement()
{
    this->type = Constant::IO::STANDARD;
    this->n = 0;
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

void Process::_unsetenv(string target)
{
    unsetenv(target.c_str());
}

void Process::_printenv(int out, string target)
{
    if (getenv(target.c_str())) {
        stringstream ss;
        ss << getenv(target.c_str()) << '\n';

        write(out, ss.str().c_str(), ss.str().length());
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

void Process::set(Constant::IOTARGET target, int n)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            this->in.n = n;
            break;
        case Constant::IOTARGET::OUT:
            this->out.n = n;
            break;
        case Constant::IOTARGET::ERR:
            this->err.n = n;
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

int Process::n(Constant::IOTARGET target)
{
    switch (target) {
        case Constant::IOTARGET::IN:
            return this->in.n;
        case Constant::IOTARGET::OUT:
            return this->out.n;
        case Constant::IOTARGET::ERR:
            return this->err.n;
        default:
            return 0;
    }
}

size_t Process::size()
{
    return this->command.size();
}

string Process::operator[](size_t index)
{
    if (index < this->command.size()) {
        return this->command[index];
    }

    throw out_of_range("Command index out of range");
}

Constant::BUILTIN Process::builtin(int in, int out, int err)
{
    string error_message = "Invalid arguments\n";

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
            write(err, error_message.c_str(), error_message.length());

            return Constant::BUILTIN::FAIL;
        }

        this->_setenv(this->command[1], this->command[2]);

        return Constant::BUILTIN::SETENV;
    }
    else if (this->command[0] == "printenv") {
        if (this->command.size() < 2) {
            write(err, error_message.c_str(), error_message.length());

            return Constant::BUILTIN::FAIL;
        }

        this->_printenv(out, this->command[1]);

        return Constant::BUILTIN::PRINTENV;
    }
    else if (this->command[0] == "who") {
        return Constant::BUILTIN::WHO;
    }
    else if (this->command[0] == "tell") {
        if (this->command.size() < 3) {
            write(err, error_message.c_str(), error_message.length());

            return Constant::BUILTIN::FAIL;
        }

        return Constant::BUILTIN::TELL;
    }
    else if (this->command[0] == "yell") {
        if (this->command.size() < 2) {
            write(err, error_message.c_str(), error_message.length());

            return Constant::BUILTIN::FAIL;
        }

        return Constant::BUILTIN::YELL;
    }
    else if (this->command[0] == "name") {
        if (this->command.size() < 2) {
            write(err, error_message.c_str(), error_message.length());

            return Constant::BUILTIN::FAIL;
        }

        return Constant::BUILTIN::NAME;
    }

    return Constant::BUILTIN::NONE;
}

void Process::handle_io(int in, int out)
{
    if (this->err.type == Constant::IO::N_PIPE) {
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
