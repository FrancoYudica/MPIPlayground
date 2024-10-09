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

    std::cout << "Process ID (PID): " << getpid() << " - MPI Rank: " << rank << " out of " << size << std::endl;

    // Define a value to be broadcasted (this could be an array or any other data type)
    long long broadcast_value;
    int root_rank = 0; // Root rank for both broadcasting and gathering

    // The root initializes the value to be broadcasted
    if (rank == root_rank) {
        broadcast_value = 5; // Example: broadcast the number 5
        std::cout << "Root (Rank " << root_rank << ") broadcasting value: " << broadcast_value << std::endl;
    }

    // Broadcast the value from root_rank to all processes
    MPI_Bcast(&broadcast_value, 1, MPI_LONG_LONG, root_rank, MPI_COMM_WORLD);

    // Each process calculates something using the broadcasted value
    long long send_buffer = broadcast_value * rank; // Example: broadcast_value * rank

    // Allocate space in root to gather results from all processes
    long long recv_buffer[size];

    // Gather the results in the root process
    int status = MPI_Gather(
        &send_buffer, // Each process sends one element
        1, // Number of elements to send
        MPI_LONG_LONG, // Data type of the sent element
        recv_buffer, // Receive buffer (on root process)
        1, // Number of elements received from each process
        MPI_LONG_LONG, // Data type of the received elements
        root_rank, // Root process that gathers data
        MPI_COMM_WORLD // MPI communicator
    );

    if (status != MPI_SUCCESS) {
        std::cout << "Error while gathering...\n";
        return 1;
    }

    // The root process prints the gathered results
    if (rank == root_rank) {
        std::cout << "Results: ";
        for (int i = 0; i < size; i++) {
            std::cout << recv_buffer[i] << ", ";
        }
        std::cout << std::endl;
    }

    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }

    return 0;
}
