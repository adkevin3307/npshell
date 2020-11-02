#include "Command.h"

#include <iostream>

#include "constant.h"

using namespace std;

Command::Command()
{
    this->process_regex = regex(R"([^|]+((\||!)\d+\s*$|\||$))");
    this->argument_regex = regex(R"((\||!|>|<)(\d+)|(\|)|(>\s+)(\S+)|(<\s+)?(\S+))");
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

    s.erase(s.begin(), first_not_space);

    auto last_not_space = find_if_not(s.rbegin(), s.rend(), [](int ch) {
        return isspace(ch);
    });

    s.erase(last_not_space.base(), s.end());

    return s;
}

void Command::parse_process(string buffer)
{
    smatch string_match;

    this->_commands.clear();
    buffer = Command::trim(buffer);

    while (regex_search(buffer, string_match, this->process_regex)) {
        string sub_string = this->trim(string_match[0]);

        this->_commands.push_back(sub_string);
        this->_histories.push_back(sub_string);

        buffer = Command::trim(string_match.suffix().str());
        if (buffer.length() == 0) break;
    }
}

void Command::parse_argument()
{
    smatch string_match;

    this->processes.clear();

    for (auto command : this->_commands) {
        Process process;

        while (regex_search(command, string_match, this->argument_regex)) {
            if (string_match[2].length() > 0) {
                int n = stoi(string_match[2]);

                if (string_match[1] == '>') {
                    process.set(Constant::IOTARGET::OUT, Constant::IO::U_PIPE);
                    process.set(Constant::IOTARGET::OUT, n);
                }
                else if (string_match[1] == '<') {
                    process.set(Constant::IOTARGET::IN, Constant::IO::U_PIPE);
                    process.set(Constant::IOTARGET::IN, n);
                }
                else {
                    process.set(Constant::IOTARGET::OUT, Constant::IO::N_PIPE);
                    process.set(Constant::IOTARGET::OUT, n);

                    if (string_match[1] == '!') {
                        process.set(Constant::IOTARGET::ERR, Constant::IO::N_PIPE);
                        process.set(Constant::IOTARGET::ERR, n);
                    }
                }
            }
            else if (string_match[3].length() > 0) {
                process.set(Constant::IOTARGET::OUT, Constant::IO::PIPE);
            }
            else if (string_match[4].length() > 0) {
                process.set(Constant::IOTARGET::OUT, Constant::IO::FILE);
                process.set(Constant::IOTARGET::OUT, string_match[5]);
            }
            else if (string_match[6].length() > 0) {
                process.set(Constant::IOTARGET::IN, Constant::IO::FILE);
                process.set(Constant::IOTARGET::IN, string_match[7]);
            }
            else {
                process.add(string_match[7]);
            }

            command = Command::trim(string_match.suffix().str());
            if (command.length() == 0) break;
        }

        this->processes.push_back(process);
    }
}

vector<Process> Command::parse(string buffer)
{
    this->parse_process(buffer);
    this->parse_argument();

    return this->processes;
}

vector<string> Command::histories()
{
    return this->_histories;
}
