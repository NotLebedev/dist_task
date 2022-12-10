#include <iostream>
#include <mpi.h>
#include <vector>
#include "Server.h"
#include "MPIHelper.h"

void Server::run() {
    std::cout << "Server started" << std::endl;
    while (true) {
        auto command = receiveCommand();
        processCommand(command.get());
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

    switch ((CommandType)message_type) {
        case CommandRead: {
            auto filename = MPI_unpack_string(buf, &pos);

            return std::make_unique<Read>(filename);
        }
        case CommandWrite: {
            int version;
            MPI_Unpack(buf.data(), buf_len, &pos, &version, 1, MPI_INT, MPI_COMM_WORLD);

            auto filename = MPI_unpack_string(buf, &pos);

            auto content = MPI_unpack_string(buf, &pos);

            return std::make_unique<Write>(version, filename, content);
        }
        case CommandGetVersion: {
            auto filename = MPI_unpack_string(buf, &pos);

            return std::make_unique<GetVersion>(filename);
        }
        case CommandFailNext: {
            return std::make_unique<FailNext>(0);
        }
    }
}

void Server::processCommand(Command *command) {
    switch (command->getType()) {
        case CommandRead:
            break;
        case CommandWrite: {
            int response = 0;
            if (fail_next) {
                response = 1;
                fail_next = false;
            } else {
                auto commandWrite = dynamic_cast<Write *>(command);
                files[commandWrite->getFilename()].setContent(commandWrite->getContents(), commandWrite->getVersion());
            }

            MPI_Send(&response, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            break;
        }
        case CommandGetVersion: {
            int version;
            if (fail_next) {
                version = -1;
                fail_next = false;
            } else {
                auto commandGetVersion = dynamic_cast<GetVersion *>(command);
                version = files[commandGetVersion->getFilename()].getVersion();
            }
            MPI_Send(&version, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            break;
        }
        case CommandFailNext: {
            fail_next = true;
            break;
        }
    }
}

const std::string &Server::File::getContent() const {
    return content;
}

int Server::File::getVersion() const {
    return version;
}

void Server::File::setContent(const std::string &content_, int version_) {
    content = content_;
    version = version_;
}
