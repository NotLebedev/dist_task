#include <vector>
#include <map>
#include <toml++/toml.h>
#include <iostream>
#include <mpi.h>
#include "Client.h"
#include "Messaging.h"

void Client::run() {
    std::cout << "Started client. Reading configuration.\n";
    std::cout << "Files distributed before start:\n";
    for (const auto &file: jobSequence->getInitialFiles())
        std::cout << "\t" << file.first << " = \"" << file.second << "\"\n";

    std::cout << "Operations to be performed:\n";
    for (const auto &operation: jobSequence->getCommandSequence())
        std::cout << "\t" << operation->describe() << "\n";
    std::cout << std::endl;

    copyFilesToServers();
}

Client::Client(const std::string &filename) : jobSequence(std::make_unique<JobSequence>(filename)) {}

ssize_t Client::getServerCount() {
    if (server_count == -1) {
        int num_process;
        MPI_Comm_size(MPI_COMM_WORLD, &num_process);

        server_count = num_process - 1;
    }

    return server_count;
}

void Client::copyFilesToServers() {
    for (size_t i = 0; i < this->getServerCount(); i++) {
        copyFilesToOneServer(i + 1);
    }
}

void Client::copyFilesToOneServer(size_t serverIdx) {
    for (const auto &file: jobSequence->getInitialFiles()) {
        sendWriteMessage(serverIdx, file.first, file.second);
    }
}

void Client::sendWriteMessage(size_t serverIdx, const std::string &filename, const std::string &content) {
    if (serverIdx == 0)
        return;

    int pos = 0;
    int message_type = MESSAGE_WRITE_FILE;
    int filenameLength = filename.size() + 1;
    int contentLength = content.size() + 1;

    // Calculating length of packed message
    int buf_len = 0;
    int next_size = 0;
    // Type of message, and length of two strings
    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &next_size);
    buf_len += 3 * next_size;
    // First string
    MPI_Pack_size(filenameLength, MPI_CHAR, MPI_COMM_WORLD, &next_size);
    buf_len += next_size;
    // Second string
    MPI_Pack_size(contentLength, MPI_CHAR, MPI_COMM_WORLD, &next_size);
    buf_len += next_size;

    auto *buf = new uint8_t[buf_len];

    MPI_Pack(&message_type, 1, MPI_INT, buf, buf_len, &pos, MPI_COMM_WORLD);
    MPI_Pack(&filenameLength, 1, MPI_INT, buf, buf_len, &pos, MPI_COMM_WORLD);
    MPI_Pack(filename.c_str(), filenameLength, MPI_CHAR, buf, buf_len, &pos, MPI_COMM_WORLD);
    MPI_Pack(&contentLength, 1, MPI_INT, buf, buf_len, &pos, MPI_COMM_WORLD);
    MPI_Pack(content.c_str(), filenameLength, MPI_CHAR, buf, buf_len, &pos, MPI_COMM_WORLD);

    MPI_Send(buf, pos, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD);

    delete[] buf;
}

std::string Client::sendReadMessage(size_t serverIdx, const std::string &filename) {
    return {};
}

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
