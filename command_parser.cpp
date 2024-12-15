#include "command_parser.h"
#include <iostream>
#include <stdexcept>

void CommandParser::execute(int argc, char** argv) {
    std::string cmd = argv[1];

    if (cmd == "init") {
        std::string path = parseOptionValue(argc, argv, "--path");
        repository.init(path);
    } else if (cmd == "status") {
        repository.status();
    } else if (cmd == "commit") {
        std::string msg = parseOptionValue(argc, argv, "-m");
        repository.commit(msg);
    } else if (cmd == "new_branch") {
        std::string branch_name = parseOptionValue(argc, argv, "-b");
        repository.new_branch(branch_name);
    } else if (cmd == "jump") {
        std::string branch_name = parseOptionValue(argc, argv, "-b");
        repository.jump(branch_name);
    } else if (cmd == "merge") {
        std::string branch_name = parseOptionValue(argc, argv, "-b");
        repository.merge(branch_name);
    } else if (cmd == "log") {
        repository.log();
    } else if (cmd == "revert") {
        if (argc < 3) {
            throw std::runtime_error("No commit_hash provided");
        }
        repository.revert(argv[2]);
    } else if (cmd == "reset") {
        if (argc < 3) {
            throw std::runtime_error("No commit_hash provided");
        }
        repository.reset(argv[2]);
    } else if (cmd == "tag") {
        if (argc < 4) {
            throw std::runtime_error("Not enough arguments for tag");
        }
        repository.tag(argv[2], argv[3]);
    } else if (cmd == "tags") {
        repository.tags();
    } else if (cmd == "--help") {
        printHelp();
    } else {
        printHelp();
    }
}

std::string CommandParser::parseOptionValue(int argc, char** argv, const std::string& option) {
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == option && (i + 1) < argc) {
            return argv[i+1];
        }
    }
    throw std::runtime_error("Missing option: " + option);
}

void CommandParser::printHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  init --path <directory_path>" << std::endl;
    std::cout << "  status" << std::endl;
    std::cout << "  commit -m \"<message>\"" << std::endl;
    std::cout << "  merge -b <branch_name>" << std::endl;
    std::cout << "  new_branch -b <branch_name>" << std::endl;
    std::cout << "  jump -b <branch_name>" << std::endl;
    std::cout << "  log" << std::endl;
    std::cout << "  revert <commit_hash>" << std::endl;
    std::cout << "  reset <commit_hash>" << std::endl;
    std::cout << "  tag <tag_name> <commit_hash>" << std::endl;
    std::cout << "  tags" << std::endl;
}
