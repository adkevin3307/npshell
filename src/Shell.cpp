#include "Shell.h"

#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

#include "Command.h"
#include "Process.h"

using namespace std;

Shell::Shell()
{
    Process env;

    env.add("setenv");
    env.add("PATH");
    env.add("bin:.");

    env.builtin();
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

        vector<Process> processes = command.parse(buffer);

        if (processes.empty()) continue;
        else if (processes[0].builtin()) continue;

        pid_t pid, wpid;

        pid = fork();

        if (pid == -1) {
            cerr << "Failed to create child" << '\n';
        }
        else if (pid == 0) {
            int in, out, fd[2];

            in = STDIN_FILENO;

            for (size_t i = 0; i < processes.size() - 1; i++) {
                if (pipe(fd) < 0) {
                    cerr << "Pipe cannot be initialized" << '\n';

                    exit(EXIT_FAILURE);
                }

                processes[i].exec(in, fd[1]);

                close(fd[1]);
                in = fd[0];
            }

            out = STDOUT_FILENO;

            if (in != STDIN_FILENO) {
                dup2(in, STDIN_FILENO);
            }

            if (out != STDOUT_FILENO) {
                dup2(out, STDOUT_FILENO);
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
