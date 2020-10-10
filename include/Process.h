#pragma once

#include <string>
#include <vector>
#include <iostream>

using namespace std;

class Process {
private:
    int line;
    int in, out, err;
    vector<string> command;

    void _setenv(string target, string source);
    void _printenv(string target);
    void _exit();

public:
    Process();
    ~Process();

    void add(string argument);
    bool builtin();
    void exec(int in, int out);

    friend ostream& operator<<(ostream& os, Process& process);
};