#include <iostream>
#include "Server.h"

void Server::run() {
    std::cout << "Server started" << std::endl;
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
