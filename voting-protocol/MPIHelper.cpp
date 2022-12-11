#include "MPIHelper.h"

std::string MPI_unpack_string(std::vector<uint8_t> &buf, int* pos) {
    int length;
    MPI_Unpack(buf.data(), buf.size(), pos, &length, 1, MPI_INT, MPI_COMM_WORLD);
    std::vector<char> stringBuf(length);
    MPI_Unpack(buf.data(), buf.size(), pos, stringBuf.data(), length, MPI_CHAR, MPI_COMM_WORLD);

    return std::move(std::string(stringBuf.data()));
}

void MPI_pack_string(const std::string &string, std::vector<uint8_t> &buf, int *pos) {
    int stringLength = string.size() + 1;
    MPI_Pack(&stringLength, 1, MPI_INT, buf.data(), buf.size(), pos, MPI_COMM_WORLD);
    MPI_Pack(string.c_str(), stringLength, MPI_CHAR, buf.data(), buf.size(), pos, MPI_COMM_WORLD);
}

void MPI_pack_int(int i, std::vector<uint8_t> &buf, int *pos) {
    MPI_Pack(&i, 1, MPI_INT, buf.data(), buf.size(), pos, MPI_COMM_WORLD);
}

int MPI_unpack_int(std::vector<uint8_t> &buf, int *pos) {
    int i;
    MPI_Unpack(buf.data(), buf.size(), pos, &i, 1, MPI_INT, MPI_COMM_WORLD);
    return i;
}

MPIPackBufferFactory::MPIPackBufferFactory() : buf_len(0) {}

void MPIPackBufferFactory::addInt() {
    int next_size = 0;
    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &next_size);
    buf_len += next_size;
}

void MPIPackBufferFactory::addInt(int count) {
    int next_size = 0;
    MPI_Pack_size(count, MPI_INT, MPI_COMM_WORLD, &next_size);
    buf_len += next_size;
}

void MPIPackBufferFactory::addString(const std::string& s) {
    addInt();
    int next_size = 0;
    MPI_Pack_size(s.size() + 1, MPI_CHAR, MPI_COMM_WORLD, &next_size);
    buf_len += next_size;
}

std::vector<uint8_t> MPIPackBufferFactory::getBuf() const {
    return std::move(std::vector<uint8_t>(buf_len));
}
