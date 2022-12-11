#include <iostream>
#include <mpi.h>
#include <mpi-ext.h>
#include <cstdlib>
#include <vector>
#include <csignal>

#include "types.h"
#include "functions/arcsin.h"
#include "functions/heaviside_step.h"
#include "functions/exp.h"
#include "messaging.h"

MPI_Comm main_comm = MPI_COMM_WORLD;
int num_procs, mpi_proc_id; // Current number of processes and id of current process
int num_procs_initial; // Number of processes before any failures
std::vector<int> failures{};

void verbose_errhandler(MPI_Comm *pcomm, int *perr, ...) {
    int err = *perr;
    char errstr[MPI_MAX_ERROR_STRING];
    int i, size, nf=0, len, eclass;
    MPI_Group group_c, group_f;
    int *ranks_gc, *ranks_gf;

    MPI_Error_class(err, &eclass);
    if( MPIX_ERR_PROC_FAILED != eclass ) {
        MPI_Abort(*pcomm, err);
    }

    MPI_Comm_rank(*pcomm, &mpi_proc_id);
    MPI_Comm_size(*pcomm, &size);

    MPIX_Comm_failure_ack(*pcomm);
    MPIX_Comm_failure_get_acked(*pcomm, &group_f);
    MPI_Group_size(group_f, &nf);
    MPI_Error_string(err, errstr, &len);

    printf("Rank %d / %d: Notified of error %s. %d found dead: { ", mpi_proc_id, size, errstr, nf);

    MPI_Comm_group(*pcomm, &group_c);
    ranks_gf = (int*)malloc(nf * sizeof(int));
    ranks_gc = (int*)malloc(nf * sizeof(int));
    for(i = 0; i < nf; i++)
        ranks_gf[i] = i;

    MPI_Group_translate_ranks(group_f, nf, ranks_gf, group_c, ranks_gc);
    for (i = 0; i < nf; i++) {
        printf("%d ", ranks_gc[i]);
        if (ranks_gc[i] == 0) {
            printf("}\nCritical. Master died. Slaves will terminate\n");
            MPI_Abort(*pcomm, err);
        }
        failures.push_back(ranks_gc[i]);
    }
    printf("}\n");
    free(ranks_gf); free(ranks_gc);
    MPIX_Comm_shrink(*pcomm, &main_comm);
    MPI_Comm_rank(main_comm, &mpi_proc_id);
    MPI_Comm_size(main_comm, &num_procs);
}

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

    size_t partition_step = n / num_procs;
    std::vector<Partition> partitions{};
    for (size_t i = 0, j = 0; j < num_procs; j++) {
        if (j < n % num_procs) {
            partitions.emplace_back(full->get(i), full->get(i + partition_step + 1), partition_step + 1);
            //printf("Partition %lu %lu %lu\n", i, i + partition_step + 1, partition_step + 1);
            i += partition_step + 1;
        } else if (partition_step != 0) {
            partitions.emplace_back(full->get(i), full->get(i + partition_step), partition_step);
            //printf("Partition %lu %lu %lu\n", i, i + partition_step, partition_step);
            i += partition_step;
        }
    }
    partitions[0] = Partition(full->get(0), full->get(partition_step + (n % num_procs > 0)),
                        partition_step + (n % num_procs > 0));
    data_t r = 0.0;

    // While there are unfinished partitions
    do {
        int num_running = num_procs;
        for (size_t i = 1; i < num_running; i++) {
            // Send all but first to slaves
            if (i < partitions.size())
                master_send_job(&partitions[i], (int) i);
            else
                // All other slaves are ordered to idle (return 0.0 to reduce)
                master_send_idle((int) i);
        }
        // Process first job
        r += run(&partitions[0], func);

        // Get all results from live processes
        r = reduce(r);

        // Remove all successfully calculated partitions
        std::vector<Partition> nextPartitions{};
        printf("Unfinished jobs {");
        for (size_t i = 0; i < partitions.size(); i++) {
            if (std::find(failures.begin(), failures.end(), i) != failures.end() || i >= num_running) {
                printf("%lu, ", i);
                nextPartitions.push_back(partitions[i]);
            }
        }
        printf("}\n");
        partitions = nextPartitions;

        failures.clear();
    } while (!partitions.empty());

    printf("All done!\n");

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

void run_slave(int argc, char *argv[]) {
    size_t kill_proc_count = 0;
    if (argc == 3) {
        char *end;
        size_t cnt = strtoull(argv[2], &end, 10);
        if (end != argv[2])
            kill_proc_count = cnt;
    }

    while (true) {
        Partition *p;
        int err = slave_receive_job(&p);
        if (err == 2)
            return;

        data_t res = 0.0;

        // Kill last kill_proc_count processes. Killing last is important as to kill them only once
        if (mpi_proc_id >= num_procs_initial - kill_proc_count)
            raise(SIGKILL);

        if (err == 0) {
            const func_type func = func_big;
            res = run(p, func);
            delete p;
        }

        reduce(res);

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
    MPI_Comm_size(main_comm, &num_process);
    for (int i = 1; i < num_process; ++i) {
        master_send_terminate(i);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(main_comm, &mpi_proc_id);
    MPI_Comm_size(main_comm, &num_procs);
    num_procs_initial = num_procs;

    MPI_Errhandler errh;
    MPI_Comm_create_errhandler(verbose_errhandler, &errh);
    MPI_Comm_set_errhandler(main_comm, errh);

    if (mpi_proc_id != 0)
        run_slave(argc, argv);
    else
        run_master(argc, argv);

    MPI_Barrier(main_comm);
    MPI_Finalize();
}