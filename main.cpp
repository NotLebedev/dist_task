
#include <mpi.h>
#include "Client.h"
#include "Server.h"

int main(int argc, char **argv) {
    MPI_Init(&argc,&argv);
    int mpi_proc_id;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_proc_id);

    if (mpi_proc_id == 0)
        Client(argv[1]).run();
    else
        Server().run();

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}
