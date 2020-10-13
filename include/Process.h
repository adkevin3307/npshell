#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "constant.h"

using namespace std;

class Process {
private:
    class IOManagement {
    public:
        Constant::IO type;
        int line;
        string file;

        IOManagement();
        ~IOManagement();
    } in, out, err;

    vector<string> command;

    void _cd(string target);
    void _setenv(string target, string source);
    void _printenv(string target);
    void _exit();

    void exec_check();

public:
    Process();
    ~Process();

    void set(Constant::IOTARGET target, Constant::IO type);
    void set(Constant::IOTARGET target, int line);
    void set(Constant::IOTARGET target, string file);

    Constant::IO type(Constant::IOTARGET target);
    int line(Constant::IOTARGET target);

    bool builtin();
    void add(string argument);
    void exec(int in, int out, bool enable_fork = true);
};
