#pragma once

#include <vector>

#include "Process.h"

using namespace std;

class Shell {
private:
    class HeapElement {
    private:
        int line;
        int fd[2];
        vector<pid_t> pids;

    public:
        HeapElement(int line);
        ~HeapElement();

        friend class Shell;

        friend bool operator<(const HeapElement& a, const HeapElement& b)
        {
            return a.line < b.line;
        }

        friend bool operator>(const HeapElement& a, const HeapElement& b)
        {
            return a.line > b.line;
        }

        friend bool operator==(const HeapElement& a, const HeapElement& b)
        {
            return a.line == b.line;
        }
    };

    vector<HeapElement> process_heap;

    void _wait(pid_t pid);
    void next_line();
    void get_pipe(int& in, int& out, Process last_process);

public:
    Shell();
    ~Shell();

    void run();
};
