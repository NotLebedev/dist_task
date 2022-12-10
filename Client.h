//
// Created by artemiy on 10.12.2022.
//

#ifndef DIST_TASK_CLIENT_H
#define DIST_TASK_CLIENT_H


#include <string>
#include <memory>
#include <vector>

class Client {
public:
    explicit Client(const std::string &filename);

    void run();
protected:
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
    std::unique_ptr<JobSequence> jobSequence;
};


#endif //DIST_TASK_CLIENT_H
