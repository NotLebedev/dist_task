#ifndef DIST_TASK_CLIENT_H
#define DIST_TASK_CLIENT_H


#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <map>
#include "Commands.h"

class Client {
public:
    explicit Client(const std::string &filename);

    void run();
protected:
    class JobSequence {
    public:
        explicit JobSequence(const std::string &filename);

        [[nodiscard]] const std::map<std::string, std::string> &getInitialFiles() const;

        [[nodiscard]] const std::vector<std::unique_ptr<Command>> &getCommandSequence() const;

    private:
        std::map<std::string, std::string> initial_files;
        std::vector<std::unique_ptr<Command>> command_sequence;
    };

private:
    ssize_t getServerCount();
    void copyFilesToServers();
    void copyFilesToOneServer(int serverIdx);
    void processCommands();
    void stopServers();

    void handleCommandWrite(Write *command);
    void handleCommandRead(Read *command);
    static void handleCommandDisableServer(DisableServer *command);
    static void handleCommandEnableServer(EnableServer *command);

    static int sendWriteMessage(int serverIdx, int nextVersion, const std::string &filename, const std::string &content);
    static std::optional<std::tuple<int, std::string>> sendReadMessage(int serverIdx, const std::string &filename);
    static int sendGetVersion(int serverIdx, const std::string &filename);
    static void sendRawCommandType(int serverIdx, CommandType commandType);

    std::unique_ptr<JobSequence> jobSequence;
    ssize_t server_count = -1;
    size_t read_quorum = 3;
    size_t write_quorum = 5;
};


#endif //DIST_TASK_CLIENT_H
