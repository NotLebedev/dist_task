#ifndef DIST_TASK_MPIHELPER_H
#define DIST_TASK_MPIHELPER_H

#include <string>
#include <vector>
#include <mpi.h>
#include <iostream>

std::string MPI_unpack_string(std::vector<uint8_t> &buf, int* pos);

#endif //DIST_TASK_MPIHELPER_H
