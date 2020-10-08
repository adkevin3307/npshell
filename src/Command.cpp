#include "Command.h"

#include <iostream>
#include <sstream>

using namespace std;

Command::Command()
{
    this->process_regex = regex(R"([^|!]+((\|\d+|!\d+)\s*$|\||$))");
    this->argument_regex = regex(R"((\||!)(\d+)?|(>)?\s*(\S+))");
}

Command::~Command()
{
}

string Command::trim(string s)
{
    if (s.empty()) return s;

    auto first_not_space = find_if_not(s.begin(), s.end(), [](int ch) {
        return isspace(ch);
    });

    auto last_not_space = find_if_not(s.rbegin(), s.rend(), [](int ch) {
        return isspace(ch);
    });

    s.erase(s.begin(), first_not_space);
    s.erase(last_not_space.base(), s.end());

    return s;
}

vector<Process> Command::parse(string buffer)
{
    smatch string_match;
    vector<string> commands;

    this->processes.clear();
    buffer = this->trim(buffer);

    while (regex_search(buffer, string_match, this->process_regex)) {
        string sub_string = this->trim(string_match[0]);

        commands.push_back(sub_string);
        this->_histories.push_back(sub_string);

        buffer = string_match.suffix().str();
        if (this->trim(buffer).length() == 0) break;
    }

    for (auto command : commands) {
        Process process;

        string s;
        stringstream ss(command);

        while (ss >> s) {
            if (s[0] != '|' && s[0] != '!') {
                process.add(s);
            }
        }

        // while (regex_search(command, string_match, this->argument_regex)) {

        //     command = string_match.suffix().str();
        //     if (this->trim(command).length() == 0) break;
        // }

        cout << process;

        this->processes.push_back(process);
    }

    return this->processes;
}

vector<string> Command::histories()
{
    return this->_histories;
}
