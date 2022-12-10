#include "Commands.h"

std::string Read::describe() {
    return "Read file " + filename;
}

CommandType Read::getType() {
    return CommandType::CommandRead;
}

const std::string &Read::getFilename() const {
    return filename;
}

std::string Write::describe() {
    if (version < 0)
        return "Write contents \"" + contents + "\" to file " + filename;
    else
        return "Write contents \"" + contents + "\" to file " + filename + " set version " + std::to_string(version);
}

CommandType Write::getType() {
    return CommandType::CommandWrite;
}

Write::Write(std::string filename, std::string contents) : filename(std::move(filename)),
                                                           contents(std::move(contents)) {}

int Write::getVersion() const {
    return version;
}

Write::Write(int version, std::string filename, std::string contents) : version(version),
                                                                        filename(std::move(filename)),
                                                                        contents(std::move(contents)) {}

const std::string &Write::getFilename() const {
    return filename;
}

const std::string &Write::getContents() const {
    return contents;
}

CommandType GetVersion::getType() {
    return CommandType::CommandGetVersion;
}

std::string GetVersion::describe() {
    return "Get version of file " + filename;
}

const std::string &GetVersion::getFilename() const {
    return filename;
}
