#ifndef INTEGRALOPENMP_TYPES_H
#define INTEGRALOPENMP_TYPES_H

#include <mpi.h>

typedef long double data_t;
typedef data_t (*func_type)(data_t);
#define MPI_DATA_T MPI_LONG_DOUBLE

class Partition {
public:
    Partition(data_t a, data_t b, size_t n_steps) {
        a_ = a;
        b_ = b;
        n_steps_ = n_steps;
        delta_ = (b - a) / (data_t)n_steps;
    }

    data_t get(size_t i) const {
        return a_ + ((double) i) * delta_;
    }

    data_t get_delta() const {
        return delta_;
    }

    size_t get_n() const {
        return n_steps_;
    }

    data_t get_a() const {
        return a_;
    }

    data_t get_b() const {
        return b_;
    }

private:
    data_t a_;
    data_t b_;
    size_t n_steps_;
    data_t delta_;
};
#endif //INTEGRALOPENMP_TYPES_H
