//
// Created by artemiy on 10.12.2022.
//

#ifndef DIST_TASK_SERVER_H
#define DIST_TASK_SERVER_H


#include <map>

class Server {
public:
    Server() : files() {}
    void run();
private:
    class File {
    public:
        File() : content() {}

        [[nodiscard]] const std::string &getContent() const;

        [[nodiscard]] uint64_t getVersion() const;

        void setContent(const std::string &content_);

    private:
        std::string content;
        uint64_t version = 0;
    };

    std::map<std::string, File> files;
};


#endif //DIST_TASK_SERVER_H
