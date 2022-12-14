#include <vector>
#include <map>
#include <toml++/toml.h>
#include <iostream>
#include <mpi.h>
#include "Client.h"
#include "Commands.h"
#include "MPIHelper.h"

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

    processCommands();

    stopServers();
}

Client::Client(const std::string &filename) : jobSequence(std::make_unique<JobSequence>(filename)) {
    if(const char* env_p = std::getenv("READ_QUORUM")) {
        try {
            int env_p_to_int = std::stoi(std::string(env_p));
            if (env_p_to_int > 0 && env_p_to_int <= getServerCount()) {
                read_quorum = env_p_to_int;
            } else
                std::cout << "Incorrect value in READ_QUORUM variable. Falling bact to defaults" << "\n";
        } catch (std::invalid_argument &e) {
            std::cout << "Incorrect value in READ_QUORUM variable. Falling bact to defaults" << "\n";
        }
    }
    write_quorum = getServerCount() - read_quorum + 1;
    std::cout << "Read quorum is: " << read_quorum << '\n' << "Write quorum is: " << write_quorum << std::endl;
}

void Client::processCommands() {
    for (const auto &command: jobSequence->getCommandSequence()) {
        std::cout << "Next command:\n\t" << command->describe() << std::endl;
        switch (command->getType()) {
            case CommandRead:
                handleCommandRead(dynamic_cast<Read *>(command.get()));
                break;
            case CommandWrite:
                handleCommandWrite(dynamic_cast<Write *>(command.get()));
                break;
            case CommandDisableServer:
                handleCommandDisableServer(dynamic_cast<DisableServer *>(command.get()));
                break;
            case CommandEnableServer:
                handleCommandEnableServer(dynamic_cast<EnableServer *>(command.get()));
                break;
            // Not available in config
            case CommandGetVersion:
            case CommandStopServer:
                break;
        }
        std::cout << "\n" << std::endl;
    }
}

ssize_t Client::getServerCount() {
    if (server_count == -1) {
        int num_process;
        MPI_Comm_size(MPI_COMM_WORLD, &num_process);

        server_count = num_process - 1;
    }

    return server_count;
}

void Client::copyFilesToServers() {
    for (int i = 0; i < this->getServerCount(); i++) {
        copyFilesToOneServer(i + 1);
    }
}

void Client::copyFilesToOneServer(int serverIdx) {
    for (const auto &file: jobSequence->getInitialFiles()) {
        sendWriteMessage(serverIdx, 1, file.first, file.second);
    }
}

int Client::sendWriteMessage(int serverIdx, int nextVersion, const std::string &filename, const std::string &content) {
    if (serverIdx == 0)
        return -1;

    int pos = 0;
    int message_type = CommandType::CommandWrite;

    MPIPackBufferFactory bufferFactory{};
    bufferFactory.addInt(2); // Type of message and version
    bufferFactory.addString(filename);
    bufferFactory.addString(content);

    std::vector<uint8_t> buf = bufferFactory.getBuf();

    MPI_pack_int(message_type, buf, &pos);
    MPI_pack_int(nextVersion, buf, &pos);
    MPI_pack_string(filename, buf, &pos);
    MPI_pack_string(content, buf, &pos);

    MPI_Send(buf.data(), pos, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD);

    MPI_Status status;
    int answer;
    MPI_Recv(&answer, 1, MPI_INT, serverIdx, 0, MPI_COMM_WORLD, &status);

    if (status.MPI_ERROR != MPI_SUCCESS || !answer)
        return 1;
    else
        return 0;
}

std::optional<std::tuple<int, std::string>> Client::sendReadMessage(int serverIdx, const std::string &filename) {
    if (serverIdx == 0)
        return {};

    int pos = 0;
    int message_type = CommandType::CommandRead;

    MPIPackBufferFactory bufferFactory{};
    bufferFactory.addInt(1); // Type of message
    bufferFactory.addString(filename);

    std::vector<uint8_t> buf = bufferFactory.getBuf();

    MPI_pack_int(message_type, buf, &pos);
    MPI_pack_string(filename, buf, &pos);

    MPI_Send(buf.data(), pos, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD);

    // Receive contents of file
    MPI_Status status;
    // Probe for message to get buffer size
    MPI_Probe(serverIdx, 0, MPI_COMM_WORLD, &status);
    // Get buffer size, before receiving
    int buf_len;
    MPI_Get_count(&status, MPI_PACKED, &buf_len);
    std::vector<uint8_t> getBuf(buf_len);
    MPI_Recv(getBuf.data(), buf_len, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD, &status);

    pos = 0;
    int version = MPI_unpack_int(getBuf, &pos);

    if (version < 0)
        return {};
    auto content = MPI_unpack_string(getBuf, &pos);

    return std::tuple(version, content);
}

int Client::sendGetVersion(int serverIdx, const std::string &filename) {
    if (serverIdx == 0)
        return -1;

    int pos = 0;
    int message_type = CommandType::CommandGetVersion;

    MPIPackBufferFactory bufferFactory{};
    bufferFactory.addInt(1); // Type of message
    bufferFactory.addString(filename);

    std::vector<uint8_t> buf = bufferFactory.getBuf();

    MPI_pack_int(message_type, buf, &pos);
    MPI_pack_string(filename, buf, &pos);

    MPI_Send(buf.data(), pos, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD);

    MPI_Status status;
    int version;
    MPI_Recv(&version, 1, MPI_INT, serverIdx, 0, MPI_COMM_WORLD, &status);

    return version;
}

void Client::sendRawCommandType(int serverIdx, CommandType commandType) {
    if (serverIdx == 0)
        return;

    int pos = 0;
    int message_type = commandType;

    MPIPackBufferFactory bufferFactory{};
    bufferFactory.addInt(1); // Type of message

    std::vector<uint8_t> buf = bufferFactory.getBuf();

    MPI_pack_int(message_type, buf, &pos);

    MPI_Send(buf.data(), pos, MPI_PACKED, serverIdx, 0, MPI_COMM_WORLD);
}

void Client::handleCommandWrite(Write *command) {
    const std::string &filename = command->getFilename();
    const std::string &contents = command->getContents();

    std::vector<int> versions{};
    for (int i = 0; i < getServerCount(); i++) {
        int version = sendGetVersion(i + 1, filename);
        if (version > 0)
            versions.push_back(version);
        if (versions.size() == write_quorum)
            break;
    }

    // Here it is assumed that failure to get quorum on version is same as failure to write
    // It is totally possible to check that all servers accepted write, but that will require
    // implementing rollbacks and that is quite beyond scope of this task
    if (versions.size() < write_quorum) {
        std::cout << "Failed to write. Quorum not met" << std::endl;
        return;
    }

    int new_version = *std::max_element(versions.cbegin(), versions.cend()) + 1;
    for (int i = 0; i < getServerCount(); i++) {
        sendWriteMessage(i + 1, new_version, filename, contents);
    }
    std::cout << "Successful write. Updated files to version " << new_version << std::endl;
}

void Client::handleCommandRead(Read *command) {
    const std::string &filename = command->getFilename();

    std::vector<int> versions{};
    std::vector<std::string> contents{};
    for (int i = 0; i < getServerCount(); i++) {
        auto res = sendReadMessage(i + 1, filename);
        if (res.has_value()) {
            versions.push_back(get<0>(res.value()));
            contents.push_back(get<1>(res.value()));
        }
        if (versions.size() == read_quorum)
            break;
    }

    if (versions.size() < read_quorum) {
        std::cout << "Failed to read. Quorum not met" << std::endl;
        return;
    }

    auto max_index = std::distance(versions.begin(), std::max_element(versions.begin(), versions.end()));
    std::string &result = contents[max_index];

    std::cout << "Read successfull. Got versions ";
    for (const auto version: versions)
        std::cout << version << " ";
    std::cout << " Latest versions had contents:\n \"" << result << "\"" << std::endl;
}

void Client::handleCommandDisableServer(DisableServer *command) {
    sendRawCommandType(command->getServer(), CommandDisableServer);
}

void Client::stopServers() {
    for (int i = 0; i < this->getServerCount(); i++) {
        sendRawCommandType(i + 1, CommandType::CommandStopServer);
    }
}

void Client::handleCommandEnableServer(EnableServer *command) {
    sendRawCommandType(command->getServer(), CommandEnableServer);
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

    for (const auto &command: *tbl["Command"].as_array()) {
        const toml::table *tab = command.as_table();
        if ((*tab)["type"].value<std::string>().value() == "Read") {
            std::string file = (*tab)["file"].value<std::string>().value();
            command_sequence.push_back(std::make_unique<Read>(file));
        } else if((*tab)["type"].value<std::string>().value() == "DisableServer") {
            int server = (*tab)["server"].value<int>().value();
            command_sequence.push_back(std::make_unique<DisableServer>(server));
        } else if((*tab)["type"].value<std::string>().value() == "EnableServer") {
            int server = (*tab)["server"].value<int>().value();
            command_sequence.push_back(std::make_unique<EnableServer>(server));
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

const std::vector<std::unique_ptr<Command>> &Client::JobSequence::getCommandSequence() const {
    return command_sequence;
}

