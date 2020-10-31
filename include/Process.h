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
        int n;
        string file;

        IOManagement();
        ~IOManagement();
    } in, out, err;

    char** args;
    vector<string> command;

    void _cd(string target);
    void _setenv(string target, string source);
    void _printenv(string target);

    void handle_io(int in, int out);
    void exec_check();

public:
    Process();
    ~Process();

    void set(Constant::IOTARGET target, Constant::IO type);
    void set(Constant::IOTARGET target, int n);
    void set(Constant::IOTARGET target, string file);

    Constant::IO type(Constant::IOTARGET target);
    int n(Constant::IOTARGET target);
    string operator[](int index);

    Constant::BUILTIN builtin();
    void add(string argument);
    void exec(int in, int out, bool enable_fork = true);
};
