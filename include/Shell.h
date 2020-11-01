#pragma once

#include <vector>
#include <unistd.h>

#include "PipeElement.h"
#include "Process.h"
#include "constant.h"

using namespace std;

class Shell {
private:
    int stdin_no, stdout_no, stderr_no;
    long max_child_amount;
    vector<PipeElement> process_heap;
    vector<PipeElement> recycle_heap;

    void _wait(pid_t pid);
    void get_pipe(int& in, int& out, Process last_process);
    void clean();

public:
    Shell(int stdin_no = STDIN_FILENO, int stdout_no = STDOUT_FILENO, int stderr_no = STDERR_FILENO);
    ~Shell();

    void next_line();
    int get(Constant::IOTARGET target);
    void set(Constant::IOTARGET target, int fd);
    pid_t run(vector<Process>& processes);
    void run();
};
