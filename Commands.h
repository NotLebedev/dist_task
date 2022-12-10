#ifndef DIST_TASK_COMMANDS_H
#define DIST_TASK_COMMANDS_H

#include <string>

enum CommandType {
    CommandRead,
    CommandWrite,
    CommandGetVersion,
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
    Write(int version, std::string filename, std::string contents);

    std::string describe() override;

    CommandType getType() override;

    [[nodiscard]] int getVersion() const;

    [[nodiscard]] const std::string &getFilename() const;

    [[nodiscard]] const std::string &getContents() const;

    ~Write() override = default;
private:
    int version = -1;
    std::string filename;
    std::string contents;
};

class GetVersion: public Command {
public:
    explicit GetVersion(std::string filename) : filename(std::move(filename)) {}

    std::string describe() override;
    CommandType getType() override;

    const std::string &getFilename() const;

    ~GetVersion() override = default;
private:
    std::string filename;
};


#endif //DIST_TASK_COMMANDS_H
