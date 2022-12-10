#ifndef DIST_TASK_CLIENT_H
#define DIST_TASK_CLIENT_H


#include <string>
#include <memory>
#include <vector>
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
    void copyFilesToOneServer(size_t serverIdx);
    int sendWriteMessage(size_t serverIdx, int nextVersion, const std::string &filename, const std::string &content);
    std::string sendReadMessage(size_t serverIdx, const std::string &filename);
    int sendGetVersion(size_t serverIdx, const std::string &filename);
    void sendFailNext(size_t serverIdx);

    std::unique_ptr<JobSequence> jobSequence;
    ssize_t server_count = -1;
};


#endif //DIST_TASK_CLIENT_H
