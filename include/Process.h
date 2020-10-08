#pragma once

#include <string>
#include <vector>
#include <iostream>

using namespace std;

class Process {
private:
    vector<string> command;

public:
    Process();
    ~Process();

    void add(string argument);
    void exec(int in, int out);

    friend ostream& operator<<(ostream& os, Process& process);
};