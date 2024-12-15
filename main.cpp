#include "command_parser.h"
#include "repository.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            std::cerr << "No command provided. Use --help for instructions.\n";
            return 1;
        }

        Repository repo;
        CommandParser parser(repo);
        parser.execute(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
