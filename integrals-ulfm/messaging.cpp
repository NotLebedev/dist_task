#include <exception>
#include <mpi.h>
#include <cstdio>

#include "messaging.h"

#define MESSAGE_JOB 0
#define MESSAGE_IDLE 1
#define MESSAGE_TERMINATE 2

extern MPI_Comm main_comm;

void master_send_job(const Partition *p, int dest) {
    if (!dest)
        return;

    unsigned char buff[128];
    data_t a = p->get_a();
    data_t b = p->get_b();
    auto n = (unsigned long long) p->get_n();

    int pos = 0;
    int m = MESSAGE_JOB;

    MPI_Pack(&m, 1, MPI_INT,                buff, 128, &pos, main_comm);
    MPI_Pack(&a, 1, MPI_DATA_T,             buff, 128, &pos, main_comm);
    MPI_Pack(&b, 1, MPI_DATA_T,             buff, 128, &pos, main_comm);
    MPI_Pack(&n, 1, MPI_UNSIGNED_LONG_LONG, buff, 128, &pos, main_comm);

    MPI_Send(buff, pos, MPI_PACKED, dest, 0, main_comm);
}

void master_send_idle(int dest) {
    unsigned char buff[128];
    int pos = 0;
    int m = MESSAGE_IDLE;

    MPI_Pack(&m, 1, MPI_INT, buff, 128, &pos, main_comm);

    MPI_Send(buff, pos, MPI_PACKED, dest, 0, main_comm);
}

void master_send_terminate(int dest) {
    unsigned char buff[128];
    int pos = 0;
    int m = MESSAGE_TERMINATE;

    MPI_Pack(&m, 1, MPI_INT, buff, 128, &pos, main_comm);

    MPI_Send(buff, pos, MPI_PACKED, dest, 0, main_comm);
}

int slave_receive_job(Partition **p) {
    MPI_Status status;
    unsigned char buff[128];
    MPI_Recv(buff, 128, MPI_PACKED, 0, 0, main_comm, &status);

    int pos = 0;
    int m;
    MPI_Unpack(buff, 128, &pos, &m, 1, MPI_INT, main_comm);

    if (m == MESSAGE_IDLE)
        return 1;
    if (m == MESSAGE_TERMINATE)
        return 2;

    data_t a;
    data_t b;
    unsigned long long n;
    MPI_Unpack(buff, 128, &pos, &a, 1, MPI_DATA_T,             main_comm);
    MPI_Unpack(buff, 128, &pos, &b, 1, MPI_DATA_T,             main_comm);
    MPI_Unpack(buff, 128, &pos, &n, 1, MPI_UNSIGNED_LONG_LONG, main_comm);

    *p = new Partition(a, b, n);
    return 0;
}

data_t reduce(data_t local_sum) {
    data_t global_sum;

    MPI_Barrier(main_comm);
    MPI_Allreduce(&local_sum, &global_sum, 1, MPI_DATA_T, MPI_SUM, main_comm);
    return global_sum;
}