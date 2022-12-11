#ifndef DIST_TASK_COMMANDS_H
#define DIST_TASK_COMMANDS_H

#include <string>

enum CommandType {
    CommandRead,
    CommandWrite,
    CommandGetVersion,
    CommandDisableServer,
    CommandEnableServer,
    CommandStopServer,
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

    [[nodiscard]] const std::string &getFilename() const;

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

class DisableServer: public Command {
public:
    explicit DisableServer(int server) : server(server) {};

    std::string describe() override {
        return "Disable server " + std::to_string(server);
    }

    CommandType getType() override {
        return CommandType::CommandDisableServer;
    }

    [[nodiscard]] int getServer() const {
        return server;
    }
private:
    int server;
};

class EnableServer: public Command {
public:
    explicit EnableServer(int server) : server(server) {};

    std::string describe() override {
        return "Enable server " + std::to_string(server);
    }

    CommandType getType() override {
        return CommandType::CommandEnableServer;
    }

    [[nodiscard]] int getServer() const {
        return server;
    }
private:
    int server;
};

class StopServer: public Command {
public:
    explicit StopServer(int server) : server(server) {};

    std::string describe() override {
        return "Enable server " + std::to_string(server);
    }

    CommandType getType() override {
        return CommandType::CommandStopServer;
    }

    [[nodiscard]] int getServer() const {
        return server;
    }
private:
    int server;
};

#endif //DIST_TASK_COMMANDS_H
