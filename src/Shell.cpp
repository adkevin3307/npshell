#include "Shell.h"

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "Command.h"

using namespace std;

Shell::Shell()
{
    Process env;

    env.add("setenv");
    env.add("PATH");
    env.add("bin:.");

    env.builtin();

    this->max_child_amount = sysconf(_SC_CHILD_MAX);
    make_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
}

Shell::~Shell()
{
    this->process_heap.clear();
    this->process_heap.shrink_to_fit();
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
    if (pid == -1) waitpid(-1, NULL, 0);
    else {
        pid_t wpid;

        while (true) {
            wpid = waitpid(pid, NULL, WNOHANG);

            if (wpid == pid) break;
        }
    }
}

void Shell::next_line()
{
    for (size_t i = 0; i < this->process_heap.size(); i++) {
        this->process_heap[i].line -= 1;
    }
}

void Shell::get_pipe(int& in, int& out, Process last_process)
{
    in = STDIN_FILENO;
    out = STDOUT_FILENO;

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
    }

    if (this->process_heap.size() != 0 && this->process_heap.front().line == 0) {
        close(this->process_heap.front().fd[1]);
        in = this->process_heap.front().fd[0];

        pop_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
    }
}

void Shell::run()
{
    string buffer;
    Command command;

    while (true) {
        cout << "% ";

        if (!getline(cin, buffer)) break;

        vector<Process> processes = command.parse(buffer);

        if (processes.empty()) continue;

        this->next_line();

        int in, out;
        get_pipe(in, out, processes.back());

        if (processes[0].builtin()) continue;

        pid_t pid = fork();

        if (pid == -1) {
            cerr << "Failed to create child" << '\n';
        }
        else if (pid == 0) {
            int fd[2];
            long cpid_amount = 0, max_cpid_amount = 64;

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
            if (!this->process_heap.empty() && this->process_heap.back().line == 0) {
                for (auto pid : this->process_heap.back().pids) {
                    this->_wait(pid);
                }

                this->process_heap.pop_back();
            }

            if (processes.back().type(Constant::IOTARGET::OUT) == Constant::IO::PIPE) {
                this->process_heap.back().pids.push_back(pid);
                push_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());

                usleep(5000);
            }
            else {
                this->_wait(pid);
            }
        }
    }
}
