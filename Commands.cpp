#include "Commands.h"

std::string Read::describe() {
    return "Read file " + filename;
}

CommandType Read::getType() {
    return CommandType::CommandRead;
}

std::string Write::describe() {
    return "Write contents " + contents + " to file " + filename;
}

CommandType Write::getType() {
    return CommandType::CommandWrite;
}

Write::Write(std::string filename, std::string contents) : filename(std::move(filename)),
                                                                   contents(std::move(contents)) {}
