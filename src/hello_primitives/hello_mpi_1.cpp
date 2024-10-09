#include <iostream>
#include <mpi/mpi.h>
#include <unistd.h> // For getpid()

int main()
{
    int rank, size;
    if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }

    std::cout << "Process ID (PID): " << getpid() << " - MPI Rank: " << rank << " out of " << size << std::endl;
    return 0;
}
