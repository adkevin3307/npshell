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

    make_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
}

Shell::~Shell()
{
    this->process_heap.clear();
    this->process_heap.shrink_to_fit();
}

Shell::HeapElement::HeapElement(int line, int fd[])
{
    this->line = line;

    this->fd[0] = fd[0];
    this->fd[1] = fd[1];
}

Shell::HeapElement::~HeapElement()
{
}

void Shell::next_line()
{
    for (size_t i = 0; i < this->process_heap.size(); i++) {
        this->process_heap[i].line -= 1;
    }
}

void Shell::get_pipe(int& in, int& out, int fd[], Process last_process)
{
    in = STDIN_FILENO;
    out = STDOUT_FILENO;

    if (this->process_heap.size() != 0 && this->process_heap.front().line == 0) {
        close(this->process_heap.front().fd[1]);
        in = this->process_heap.front().fd[0];

        pop_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
        this->process_heap.pop_back();
    }

    if (last_process.type(Constant::IOTARGET::OUT) == Constant::IO::PIPE) {
        if (pipe(fd) < 0) {
            cerr << "Pipe cannot be initialized" << '\n';

            exit(EXIT_FAILURE);
        }

        HeapElement element(last_process.line(Constant::IOTARGET::OUT), fd);
        vector<HeapElement>::iterator it = find(this->process_heap.begin(), this->process_heap.end(), element);

        if (it != this->process_heap.end()) {
            out = it->fd[1];
        }
        else {
            out = fd[1];

            this->process_heap.push_back(HeapElement(last_process.line(Constant::IOTARGET::OUT), fd));
            push_heap(this->process_heap.begin(), this->process_heap.end(), greater<HeapElement>());
        }
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

        int in, out, fd[2];
        get_pipe(in, out, fd, processes[processes.size() - 1]);

        if (processes[0].builtin()) continue;

        pid_t pid, wpid;

        pid = fork();

        if (pid == -1) {
            cerr << "Failed to create child" << '\n';
        }
        else if (pid == 0) {
            for (size_t i = 0; i < processes.size() - 1; i++) {
                if (pipe(fd) < 0) {
                    cerr << "Pipe cannot be initialized" << '\n';

                    exit(EXIT_FAILURE);
                }

                processes[i].exec(in, fd[1]);

                close(fd[1]);
                in = fd[0];
            }

            processes[processes.size() - 1].exec(in, out, false);
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