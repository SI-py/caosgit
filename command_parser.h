#pragma once
#include <string>
#include "repository.h"

class CommandParser {
public:
    CommandParser(Repository& repo) : repository(repo) {}
    void execute(int argc, char** argv);

private:
    Repository& repository;
    std::string parseOptionValue(int argc, char** argv, const std::string& option);
    void printHelp();
};
