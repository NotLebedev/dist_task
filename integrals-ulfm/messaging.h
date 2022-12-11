#ifndef INTEGRALOPENMP_MESSAGING_H
#define INTEGRALOPENMP_MESSAGING_H
#include "types.h"

void master_send_job(const Partition *p, int dest);
void master_send_idle(int dest);
void master_send_terminate(int dest);

int slave_receive_job(Partition **p);

data_t reduce(data_t local_sum);
#endif //INTEGRALOPENMP_MESSAGING_H
