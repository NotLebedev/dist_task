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
        case CommandRead: {
            if (fail_next) {
                int pos = 0;
                int buf_len = 0;
                int next_size = 0;
                MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &next_size);
                buf_len += next_size;
                std::vector<uint8_t> buf(buf_len);

                int error = -1;
                MPI_Pack(&error, 1, MPI_INT, buf.data(), buf_len, &pos, MPI_COMM_WORLD);

                MPI_Send(buf.data(), pos, MPI_PACKED, 0, 0, MPI_COMM_WORLD);
            } else {
                auto commandRead = dynamic_cast<Read *>(command);
                const std::string &text = files[commandRead->getFilename()].getContent();
                int version = files[commandRead->getFilename()].getVersion();

                int pos = 0;
                int textLength = text.size() + 1;

                // Calculating length of packed message
                int buf_len = 0;
                int next_size = 0;
                // Length of string, version
                MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &next_size);
                buf_len += 3 * next_size;
                // First string
                MPI_Pack_size(textLength, MPI_CHAR, MPI_COMM_WORLD, &next_size);
                buf_len += next_size;

                std::vector<uint8_t> buf(buf_len);

                MPI_Pack(&version, 1, MPI_INT, buf.data(), buf_len, &pos, MPI_COMM_WORLD);
                MPI_Pack(&textLength, 1, MPI_INT, buf.data(), buf_len, &pos, MPI_COMM_WORLD);
                MPI_Pack(text.c_str(), textLength, MPI_CHAR, buf.data(), buf_len, &pos, MPI_COMM_WORLD);

                MPI_Send(buf.data(), pos, MPI_PACKED, 0, 0, MPI_COMM_WORLD);
            }
            break;
        }
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
