//
// Created by artemiy on 10.12.2022.
//

#include <vector>
#include <map>
#include <toml++/toml.h>
#include <iostream>
#include "Client.h"

void Client::run() {
    std::cout << "Started client. Reading configuration.\n";
    std::cout << "Files distributed before start:\n";
    for (const auto &file: jobSequence->getInitialFiles())
        std::cout << "\t" << file.first << " = \"" << file.second << "\"\n";

    std::cout << "Operations to be performed:\n";
    for (const auto &operation: jobSequence->getCommandSequence())
        std::cout << "\t" << operation->describe() << "\n";
    std::cout << std::endl;
}

Client::Client(const std::string &filename) : jobSequence(std::make_unique<JobSequence>(filename)) {}

Client::JobSequence::JobSequence(const std::string &filename) : initial_files(), command_sequence() {
    toml::table tbl;
    try {
        tbl = toml::parse_file(filename);
    } catch (const toml::parse_error &err) {
        std::cerr << "Parsing failed:\n" << err << "\n";
        throw err;
    }

    for (const auto &file: *tbl["Initial"].as_table()) {
        initial_files[std::string(file.first)] = std::string(*file.second.as_string());
    }

    for (const auto &command: *tbl["CommandRead"].as_array()) {
        const toml::table *tab = command.as_table();
        if ((*tab)["type"].value<std::string>().value() == "CommandRead") {
            std::string file = (*tab)["file"].value<std::string>().value();
            command_sequence.push_back(std::make_unique<Read>(file));
        } else {
            std::string file = (*tab)["file"].value<std::string>().value();
            std::string content = (*tab)["content"].value<std::string>().value();
            command_sequence.push_back(std::make_unique<Write>(file, content));
        }
    }
}

const std::map<std::string, std::string> &Client::JobSequence::getInitialFiles() const {
    return initial_files;
}

const std::vector<std::unique_ptr<Client::Command>> &Client::JobSequence::getCommandSequence() const {
    return command_sequence;
}

std::string Client::Read::describe() {
    return "Read file " + filename;
}

Client::CommandType Client::Read::getType() {
    return CommandType::CommandRead;
}

std::string Client::Write::describe() {
    return "Write contents " + contents + " to file " + filename;
}

Client::CommandType Client::Write::getType() {
    return CommandType::CommandWrite;
}

Client::Write::Write(std::string filename, std::string contents) : filename(std::move(filename)),
                                                                   contents(std::move(contents)) {}
