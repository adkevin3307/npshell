#pragma once

#include <string>
#include <vector>
#include <regex>

using namespace std;

class Command {
private:
    regex process_regex;
    regex argument_regex;
    vector<string> _commands;
    vector<string> _histories;

    string trim(string s);

public:
    Command();
    ~Command();

    void parse(string buffer);
    vector<string>& commands();
    vector<string>& histories();
};
