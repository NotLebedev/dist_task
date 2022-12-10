#include <iostream>
#include <mpi.h>
#include <vector>
#include "Server.h"
#include "MPIHelper.h"

void Server::run() {
    std::cout << "Server started" << std::endl;
    while (true) {
        auto command = receiveCommand();
        std::cout << "Got message " << command->describe() << std::endl;
    }
}

std::unique_ptr<Command> Server::receiveCommand() {
    MPI_Status status;
    // Probe for message to get buffer size
    MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
    int buf_len = 0;
    // Get buffer size, before receiving
    MPI_Get_count(&status, MPI_PACKED, &buf_len);

    std::vector<uint8_t> buf(buf_len);
    MPI_Recv(buf.data(), buf_len, MPI_PACKED, 0, 0, MPI_COMM_WORLD, &status);

    int pos = 0;
    int message_type;

    MPI_Unpack(buf.data(), buf_len, &pos, &message_type, 1, MPI_INT, MPI_COMM_WORLD);
    if (message_type == CommandType::CommandWrite) {
        auto filename = MPI_unpack_string(buf, &pos);

        auto content = MPI_unpack_string(buf, &pos);

        return std::make_unique<Write>(filename, content);
    } else {
        return std::make_unique<Read>("<none>");
    }
}

const std::string &Server::File::getContent() const {
    return content;
}

uint64_t Server::File::getVersion() const {
    return version;
}

void Server::File::setContent(const std::string &content_) {
    content = content_;
    version++;
}
