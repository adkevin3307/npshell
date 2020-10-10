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
    vector<string> _commands;
    vector<string> _histories;
    vector<Process> processes;

    string trim(string s);
    void parse_process(string buffer);
    void parse_argument();

public:
    Command();
    ~Command();

    vector<Process> parse(string buffer);
    vector<string> histories();
};
