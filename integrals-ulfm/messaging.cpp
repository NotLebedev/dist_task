#include <cctype>
#include <exception>
#include <mpi.h>

#include "messaging.h"

#define MESSAGE_JOB 0
#define MESSAGE_TERMINATE 1

void master_send_job(const Partition *p, int dest) {
    if (!dest)
        return;

    unsigned char buff[128];
    data_t a = p->get_a();
    data_t b = p->get_b();
    unsigned long long n = (unsigned long long) p->get_n();

    int pos = 0;
    int m = MESSAGE_JOB;

    MPI_Pack(&m, 1, MPI_INT,                buff, 128, &pos, MPI_COMM_WORLD);
    MPI_Pack(&a, 1, MPI_DATA_T,             buff, 128, &pos, MPI_COMM_WORLD);
    MPI_Pack(&b, 1, MPI_DATA_T,             buff, 128, &pos, MPI_COMM_WORLD);
    MPI_Pack(&n, 1, MPI_UNSIGNED_LONG_LONG, buff, 128, &pos, MPI_COMM_WORLD);

    MPI_Send(buff, pos, MPI_PACKED, dest, 0, MPI_COMM_WORLD);
}

void master_send_terminate(int dest) {
    unsigned char buff[128];
    int pos = 0;
    int m = MESSAGE_TERMINATE;

    MPI_Pack(&m, 1, MPI_INT, buff, 128, &pos, MPI_COMM_WORLD);

    MPI_Send(buff, pos, MPI_PACKED, dest, 0, MPI_COMM_WORLD);
}

Partition *slave_receive_job() {
    MPI_Status status;
    unsigned char buff[128];
    MPI_Recv(buff, 128, MPI_PACKED, 0, 0, MPI_COMM_WORLD, &status);

    int pos = 0;
    int m;
    MPI_Unpack(buff, 128, &pos, &m, 1, MPI_INT, MPI_COMM_WORLD);

    if (m == MESSAGE_TERMINATE)
        return NULL;

    data_t a;
    data_t b;
    unsigned long long n;
    MPI_Unpack(buff, 128, &pos, &a, 1, MPI_DATA_T,             MPI_COMM_WORLD);
    MPI_Unpack(buff, 128, &pos, &b, 1, MPI_DATA_T,             MPI_COMM_WORLD);
    MPI_Unpack(buff, 128, &pos, &n, 1, MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);

    return new Partition(a, b, n);
}

data_t reduce(data_t local_sum) {
    data_t global_sum;

    MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DATA_T, MPI_SUM, MPI_COMM_WORLD);
    return global_sum;
}