#pragma once

#include <vector>
#include <unistd.h>

using namespace std;

class PipeElement {
private:
    int n, fd[2];
    vector<pid_t> pids;

public:
    PipeElement();
    PipeElement(int n);
    ~PipeElement();

    friend class Shell;
    friend class SingleProcessShell;

    friend bool operator<(const PipeElement& a, const PipeElement& b)
    {
        return a.n < b.n;
    }

    friend bool operator>(const PipeElement& a, const PipeElement& b)
    {
        return a.n > b.n;
    }

    friend bool operator==(const PipeElement& a, const PipeElement& b)
    {
        return a.n == b.n;
    }
};