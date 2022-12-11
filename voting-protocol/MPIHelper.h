#ifndef DIST_TASK_MPIHELPER_H
#define DIST_TASK_MPIHELPER_H

#include <string>
#include <vector>
#include <mpi.h>
#include <iostream>

/**
 * Factory for stl buffers for creating MPI packed messages
 */
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

/**
 * Pack an int into stl buffer for MPI packed send
 */
void MPI_pack_int(int i, std::vector<uint8_t> &buf, int* pos);

/**
 * Pack a string into stl buffer for MPI packed send
 */
void MPI_pack_string(const std::string &string, std::vector<uint8_t> &buf, int* pos);

/**
 * Get next string from packed MPI message
 */
std::string MPI_unpack_string(std::vector<uint8_t> &buf, int* pos);

/**
 * Get next int from packed MPI message
 */
int MPI_unpack_int(std::vector<uint8_t> &buf, int* pos);

#endif //DIST_TASK_MPIHELPER_H
