#pragma once

#include <string>
#include <vector>
#include <regex>

#include "Process.h"

using namespace std;

class Command {
private:
    regex process_regex;
    regex argument_regex;
    vector<Process> processes;
    vector<string> _histories;

    string trim(string s);

public:
    Command();
    ~Command();

    vector<Process> parse(string buffer);
    vector<string> histories();
};
