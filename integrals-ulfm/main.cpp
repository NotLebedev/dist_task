#include <iostream>
#include <mpi.h>
#include <cstdlib>

#include "types.h"
#include "functions/arcsin.h"
#include "functions/heaviside_step.h"
#include "functions/exp.h"
#include "messaging.h"

/**
 * Compound function with heavy calculations
 */
data_t func_big(data_t x) {
    return exp(x) + heaviside_step(x) + arcsin(x);
}

bool invalidate = false;

class Result {
public:
    void timer_start() {
        time = MPI_Wtime();
    }

    void timer_end() {
        time = MPI_Wtime() - time;
    }

    void set_result(data_t res) {
        result = res;
    }

    [[nodiscard]] data_t get_result() const {
        return result;
    }

    [[nodiscard]] double get_runtime() const {
        return time;
    }
private:
    double time;
    data_t result;
};

data_t run(Partition *x_, const func_type func) {
    data_t result = 0.0;

    size_t n = x_->get_n();
    for (size_t i = 0; i < n; i++) {
        result += func(x_->get(i));
    }

    return result;
}

Result *run_full(const size_t n, const data_t a, const data_t b, const func_type func) {
    auto *res = new Result();
    res->timer_start();

    auto *full = new Partition(a, b, n);

    int num_process;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);

    size_t partition_step = n / num_process;
    for (size_t i = 0, j = 0; j < num_process; j++) {
        if (j < n % num_process) {
            auto *p = new Partition(full->get(i), full->get(i + partition_step + 1), partition_step + 1);
            //printf("Partition %lu %lu %lu\n", i, i + partition_step + 1, partition_step + 1);
            i += partition_step + 1;
            master_send_job(p, (int) j);

            delete p;
        } else if (partition_step != 0) {
            auto *p = new Partition(full->get(i), full->get(i + partition_step), partition_step);
            //printf("Partition %lu %lu %lu\n", i, i + partition_step, partition_step);
            i += partition_step;
            master_send_job(p, (int) j);

            delete p;
        }
    }

    auto *p = new Partition(full->get(0), full->get(partition_step + (n % num_process > 0)),
                                 partition_step + (n % num_process > 0));
    data_t r = run(p, func);
    r = reduce(r);
    delete p;

    r += (func(full->get(n)) - func(full->get(0))) / 2;
    r *= full->get_delta();

    res->set_result(r);
    res->timer_end();

    return res;
}

void actual(const size_t n, const data_t a, const data_t b, const func_type func) {
    Result *res = run_full(n, a, b, func);

    std::cout << "Runtime: " << res->get_runtime() << " seconds. ";
    std::cout << "Result: " << res->get_result() << std::endl;

    delete res;
}

void run_slave() {
    while (true) {
        Partition *p = slave_receive_job();
        if (p == nullptr)
            return;
        const func_type func = func_big;
        data_t res = run(p, func);
        reduce(res);
        delete p;

        // Invalidate coefficient table
        invalidate = true;
        arcsin(0);
        exp(0);
        heaviside_step(0);
        invalidate = false;
    }
}

void run_master(int argc, char *argv[]) {
    size_t n = 100000;
    if (argc == 2) {
        char *end;
        size_t cnt = strtoull(argv[1], &end, 10);
        if (end != argv[1])
            n = cnt;
    }
    const data_t a = -1.0;
    const data_t b = 1.0;
    const func_type func = func_big;

    actual(n, a, b, func);

    int num_process;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
    for (int i = 1; i < num_process; ++i) {
        master_send_terminate(i);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc,&argv);
    int mpi_proc_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_proc_id);
    if (mpi_proc_id != 0)
        run_slave();
    else
        run_master(argc, argv);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
}