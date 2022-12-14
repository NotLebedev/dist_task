#ifndef DIST_TASK_SERVER_H
#define DIST_TASK_SERVER_H


#include <map>
#include <memory>
#include "Commands.h"

class Server {
public:
    Server() : files{} {}
    void run();
private:
    class File {
    public:
        File() : content() {}

        [[nodiscard]] const std::string &getContent() const;

        [[nodiscard]] int getVersion() const;

        void setContent(const std::string &content_, int version_);

    private:
        std::string content;
        int version = 0;
    };

    static std::unique_ptr<Command> receiveCommand();
    void processCommand(Command *command);

    std::map<std::string, File> files;
    bool disabled = false;
};


#endif //DIST_TASK_SERVER_H
