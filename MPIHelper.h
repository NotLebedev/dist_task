#ifndef DIST_TASK_MPIHELPER_H
#define DIST_TASK_MPIHELPER_H

#include <string>
#include <vector>
#include <mpi.h>
#include <iostream>

std::string MPI_unpack_string(std::vector<uint8_t> &buf, int* pos) {
    int length;
    MPI_Unpack(buf.data(), buf.size(), pos, &length, 1, MPI_INT, MPI_COMM_WORLD);
    std::vector<char> stringBuf(length);
    MPI_Unpack(buf.data(), buf.size(), pos, stringBuf.data(), length, MPI_CHAR, MPI_COMM_WORLD);

    return std::move(std::string(stringBuf.data()));
}

#endif //DIST_TASK_MPIHELPER_H
