#ifndef DIST_TASK_COMMANDS_H
#define DIST_TASK_COMMANDS_H

#include <string>

enum CommandType {
    CommandRead,
    CommandWrite,
};

class Command {
public:
    virtual std::string describe() = 0;
    virtual CommandType getType() = 0;
    virtual ~Command() = default;
};

class Read: public Command {
public:
    explicit Read(std::string filename) : filename(std::move(filename)) {}

    std::string describe() override;
    CommandType getType() override;
    ~Read() override = default;
private:
    std::string filename;
};

class Write: public Command {
public:
    Write(std::string filename, std::string contents);

    std::string describe() override;

    CommandType getType() override;

    ~Write() override = default;
private:
    std::string filename;
    std::string contents;
};


#endif //DIST_TASK_COMMANDS_H
