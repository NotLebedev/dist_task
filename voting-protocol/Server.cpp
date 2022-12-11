#include <iostream>
#include <mpi.h>
#include <vector>
#include "Server.h"
#include "MPIHelper.h"

void Server::run() {
    std::cout << "Server started" << std::endl;
    while (true) {
        auto command = receiveCommand();
        if (command->getType() == CommandType::CommandStopServer)
            break;
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
    int message_type = MPI_unpack_int(buf, &pos);

    switch ((CommandType)message_type) {
        case CommandRead: {
            auto filename = MPI_unpack_string(buf, &pos);

            return std::make_unique<Read>(filename);
        }
        case CommandWrite: {
            int version = MPI_unpack_int(buf, &pos);

            auto filename = MPI_unpack_string(buf, &pos);

            auto content = MPI_unpack_string(buf, &pos);

            return std::make_unique<Write>(version, filename, content);
        }
        case CommandGetVersion: {
            auto filename = MPI_unpack_string(buf, &pos);

            return std::make_unique<GetVersion>(filename);
        }
        case CommandDisableServer:
            return std::make_unique<DisableServer>(0);
        case CommandEnableServer:
            return std::make_unique<EnableServer>(0);
        case CommandStopServer:
            return std::make_unique<StopServer>(0);
    }
}

void Server::processCommand(Command *command) {
    switch (command->getType()) {
        case CommandRead: {
            if (disabled) {
                int pos = 0;
                MPIPackBufferFactory bufferFactory{};
                bufferFactory.addInt(1); // Error code

                std::vector<uint8_t> buf = bufferFactory.getBuf();

                int error = -1;
                MPI_pack_int(error, buf, &pos);

                MPI_Send(buf.data(), pos, MPI_PACKED, 0, 0, MPI_COMM_WORLD);
            } else {
                auto commandRead = dynamic_cast<Read *>(command);
                const std::string &text = files[commandRead->getFilename()].getContent();
                int version = files[commandRead->getFilename()].getVersion();

                int pos = 0;

                MPIPackBufferFactory bufferFactory{};
                bufferFactory.addInt(2); // Type of message
                bufferFactory.addString(text);

                std::vector<uint8_t> buf = bufferFactory.getBuf();

                MPI_pack_int(version, buf, &pos);
                MPI_pack_string(text, buf, &pos);

                MPI_Send(buf.data(), pos, MPI_PACKED, 0, 0, MPI_COMM_WORLD);
            }
            break;
        }
        case CommandWrite: {
            int response = 0;
            if (disabled) {
                response = 1;
            } else {
                auto commandWrite = dynamic_cast<Write *>(command);
                files[commandWrite->getFilename()].setContent(commandWrite->getContents(), commandWrite->getVersion());
            }

            MPI_Send(&response, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            break;
        }
        case CommandGetVersion: {
            int version;
            if (disabled) {
                version = -1;
            } else {
                auto commandGetVersion = dynamic_cast<GetVersion *>(command);
                version = files[commandGetVersion->getFilename()].getVersion();
            }
            MPI_Send(&version, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            break;
        }
        case CommandDisableServer: {
            disabled = true;
            break;
        }
        case CommandEnableServer: {
            disabled = false;
            break;
        }
        case CommandStopServer:
            break;
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
