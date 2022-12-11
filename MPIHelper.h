#ifndef DIST_TASK_MPIHELPER_H
#define DIST_TASK_MPIHELPER_H

#include <string>
#include <vector>
#include <mpi.h>
#include <iostream>

class MPIPackBufferFactory {
public:
    explicit MPIPackBufferFactory();

    void addInt();
    void addInt(int count);
    void addString(const std::string& s);

    [[nodiscard]] std::vector<uint8_t> getBuf() const;
private:
    int buf_len;
};

std::string MPI_unpack_string(std::vector<uint8_t> &buf, int* pos);

#endif //DIST_TASK_MPIHELPER_H
