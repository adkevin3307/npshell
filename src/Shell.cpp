#include "Shell.h"

#include <iostream>
#include <sys/wait.h>

#include "Command.h"

using namespace std;

Shell::Shell(int stdin_no, int stdout_no, int stderr_no)
{
    this->stdin_no = stdin_no;
    this->stdout_no = stdout_no;
    this->stderr_no = stderr_no;

    Process env;

    env.add("setenv");
    env.add("PATH");
    env.add("bin:.");

    env.builtin();

    this->max_child_amount = sysconf(_SC_CHILD_MAX);
    make_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
    make_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());
}

Shell::~Shell()
{
    for (auto element : this->process_heap) {
        for (auto pid : element.pids) {
            kill(pid, SIGINT);

            this->_wait(pid);
        }
        close(element.fd[0]);
        close(element.fd[1]);
    }

    this->process_heap.clear();
    this->process_heap.shrink_to_fit();

    for (auto element : this->recycle_heap) {
        for (auto pid : element.pids) {
            kill(pid, SIGINT);

            this->_wait(pid);
        }
        close(element.fd[0]);
        close(element.fd[1]);
    }

    this->recycle_heap.clear();
    this->recycle_heap.shrink_to_fit();
}

Shell::HeapElement::HeapElement(int line)
{
    this->line = line;
}

Shell::HeapElement::~HeapElement()
{
}

void Shell::_wait(pid_t pid)
{
    pid_t wpid;

    while (true) {
        wpid = waitpid(pid, NULL, WNOHANG);

        if (wpid == pid) break;
    }
}

void Shell::next_line()
{
    for (size_t i = 0; i < this->process_heap.size(); i++) {
        this->process_heap[i].line -= 1;
    }

    for (size_t i = 0; i < this->recycle_heap.size(); i++) {
        this->recycle_heap[i].line -= 1;
    }
}

void Shell::get_pipe(int& in, int& out, Process last_process)
{
    in = STDIN_FILENO;
    out = STDOUT_FILENO;

    if (!this->process_heap.empty() && this->process_heap.front().line == 0) {
        close(this->process_heap.front().fd[1]);
        in = this->process_heap.front().fd[0];

        this->recycle_heap.push_back(this->process_heap.front());
        push_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());

        pop_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
        this->process_heap.pop_back();
    }

    if (last_process.type(Constant::IOTARGET::OUT) == Constant::IO::PIPE) {
        HeapElement element(last_process.line(Constant::IOTARGET::OUT));
        vector<HeapElement>::iterator it = find(this->process_heap.begin(), this->process_heap.end(), element);

        if (it != this->process_heap.end()) {
            out = it->fd[1];
        }
        else {
            int fd[2];

            if (pipe(fd) < 0) {
                cerr << "Pipe cannot be initialized" << '\n';

                exit(EXIT_FAILURE);
            }

            out = fd[1];

            element.fd[0] = fd[0];
            element.fd[1] = fd[1];

            this->process_heap.push_back(element);
        }

        while (!this->recycle_heap.empty() && this->recycle_heap.front().line == 0) {
            pop_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());

            this->recycle_heap.back().line = last_process.line(Constant::IOTARGET::OUT);
            push_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());
        }
    }
}

Constant::BUILTIN Shell::run(string& buffer)
{
    Command command;
    vector<Process> processes = command.parse(buffer);

    if (processes.empty()) return Constant::BUILTIN::NONE;

    this->next_line();

    int in, out;
    get_pipe(in, out, processes.back());

    Constant::BUILTIN builtin_type = processes[0].builtin();

    if (builtin_type == Constant::BUILTIN::EXIT) return Constant::BUILTIN::EXIT;
    if (builtin_type != Constant::BUILTIN::NONE) return builtin_type;

    pid_t pid = fork();

    if (pid == -1) {
        cerr << "Failed to create child" << '\n';
    }
    else if (pid == 0) {
        dup2(this->stdin_no, STDIN_FILENO);
        dup2(this->stdout_no, STDOUT_FILENO);
        dup2(this->stderr_no, STDERR_FILENO);

        int fd[2];
        long cpid_amount = 0, max_cpid_amount = min(64l, this->max_child_amount);

        for (size_t i = 0; i < processes.size() - 1; i++, cpid_amount++) {
            if (pipe(fd) < 0) {
                cerr << "Pipe cannot be initialized" << '\n';

                exit(EXIT_FAILURE);
            }

            processes[i].exec(in, fd[1]);

            close(in);

            dup2(fd[0], in);

            close(fd[0]);
            close(fd[1]);

            if (cpid_amount > max_cpid_amount) {
                for (long j = 0; j < (long)(max_cpid_amount / 2); j++, cpid_amount--) {
                    this->_wait(-1);
                }

                if (max_cpid_amount < (long)(this->max_child_amount / 2)) max_cpid_amount <<= 2;
            }
        }

        processes.back().exec(in, out, false);
    }
    else {
        pid_t wpid;
        Constant::IO io_type = processes.back().type(Constant::IOTARGET::OUT);

        if (io_type != Constant::IO::PIPE) {
            this->_wait(pid);

            while (!this->recycle_heap.empty() && this->recycle_heap.front().line == 0) {
                for (auto pid : this->recycle_heap.front().pids) {
                    kill(pid, SIGINT);

                    this->_wait(pid);
                }

                pop_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());
                this->recycle_heap.pop_back();
            }
        }
        else {
            this->process_heap.back().pids.push_back(pid);
            push_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
        }

        for (int i = this->recycle_heap.size() - 1; i >= 0; i--) {
            for (int j = this->recycle_heap[i].pids.size() - 1; j >= 0; j--) {
                wpid = waitpid(this->recycle_heap[i].pids[j], NULL, WNOHANG);

                if (wpid == this->recycle_heap[i].pids[j]) this->recycle_heap[i].pids.erase(this->recycle_heap[i].pids.begin() + j);
            }

            if (this->recycle_heap[i].pids.size() == 0) {
                close(this->recycle_heap[i].fd[0]);
                this->recycle_heap.erase(this->recycle_heap.begin() + i);
            }
        }

        if (!this->recycle_heap.empty()) {
            push_heap(this->recycle_heap.begin(), this->recycle_heap.end(), greater<HeapElement>());
        }
    }

    return Constant::BUILTIN::NONE;
}

void Shell::run()
{
    while (true) {
        cout << "% ";

        string buffer;
        if (!getline(cin, buffer)) break;

        if (this->run(buffer) == Constant::BUILTIN::EXIT) break;
    }
}
