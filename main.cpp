#include <iostream>
#include <mpi.h>
#include <map>
#include "Client.h"

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
    std::cout << "Server started" << std::endl;
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
