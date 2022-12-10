#include <iostream>
#include <utility>
#include <toml++/toml.h>
#include <mpi.h>

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

    std::string describe() override {
        return "Read file " + filename;
    }
    CommandType getType() override {
        return CommandType::CommandRead;
    }
    ~Read() override = default;
private:
    std::string filename;
};

class Write: public Command {
public:
    Write(std::string filename, std::string contents) : filename(std::move(filename)), contents(std::move(contents)) {}

    std::string describe() override {
        return "Write contents " + contents + " to file " + filename;
    }

    CommandType getType() override {
        return CommandType::CommandWrite;
    }

    ~Write() override = default;
private:
    std::string filename;
    std::string contents;
};

class JobSequence {
public:
    JobSequence(const std::string &filename) : initial_files(), command_sequence() {
        toml::table tbl;
        try {
            tbl = toml::parse_file(filename);
        } catch (const toml::parse_error& err) {
            std::cerr << "Parsing failed:\n" << err << "\n";
            throw err;
        }

        for (const auto &file: *tbl["Initial"].as_table()) {
            initial_files[std::string(file.first)] = std::string(*file.second.as_string());
        }

        for (const auto &command: *tbl["CommandRead"].as_array()) {
            const toml::table *tab = command.as_table();
            if ((*tab)["type"].value<std::string>().value() == "CommandRead") {
                std::string file = (*tab)["file"].value<std::string>().value();
                command_sequence.push_back(std::make_unique<Read>(file));
            } else {
                std::string file = (*tab)["file"].value<std::string>().value();
                std::string content = (*tab)["content"].value<std::string>().value();
                command_sequence.push_back(std::make_unique<Write>(file, content));
            }
        }
    }

    [[nodiscard]] const std::map<std::string, std::string> &getInitialFiles() const {
        return initial_files;
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Command>> &getCommandSequence() const {
        return command_sequence;
    }

private:
    std::map<std::string, std::string> initial_files;
    std::vector<std::unique_ptr<Command>> command_sequence;
};

class Client {
public:
    explicit Client(const std::string &filename) : jobSequence(filename) {}

    void run();
private:
    JobSequence jobSequence;
};

void Client::run() {
    std::cout << "Files distributed before start:\n";
    for (const auto &file: jobSequence.getInitialFiles())
        std::cout << "\t" << file.first << " = \"" << file.second << "\"\n";

    std::cout << "Operations to be performed:\n";
    for (const auto &operation: jobSequence.getCommandSequence())
        std::cout << "\t" << operation->describe() << "\n";
    std::cout << std::endl;
}

class Server {
public:
    Server() : files() {}
    void run();
private:
    class File {
    public:
        File() : content() {}

        [[nodiscard]] const std::string &getContent() const {
            return content;
        }

        [[nodiscard]] uint64_t getVersion() const {
            return version;
        }

        void setContent(const std::string &content_) {
            content = content_;
            version++;
        }

    private:
        std::string content;
        uint64_t version = 0;
    };

    std::map<std::string, File> files;
};

void Server::run() {

}

int main(int argc, char **argv) {
    MPI_Init(&argc,&argv);
    int mpi_proc_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_proc_id);

    if (mpi_proc_id == 0)
        Client(argv[1]).run();
    else
        Server().run();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
