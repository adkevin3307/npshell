#pragma once

#include <vector>

#include "Process.h"

using namespace std;

class HeapElement {
private:
    int line;
    int fd[2];

public:
    HeapElement(int line, int fd[]);
    ~HeapElement();

    friend class Shell;

    friend bool operator<(const HeapElement& a, const HeapElement& b);
    friend bool operator>(const HeapElement& a, const HeapElement& b);
    friend bool operator==(const HeapElement& a, const HeapElement& b);
};

class Shell {
private:
    vector<HeapElement> process_heap;

    void next_line();
    void get_pipe(int& in, int& out, int fd[], Process last_process);

public:
    Shell();
    ~Shell();

    void run();
};