/*
Complete guide for setting up and running.

1. Compile the program
    mpic++ src/log_taylor_aproximation_cluster.cpp -o build/taylor_cluster

2. Share public keys to all the nodes of the cluster
    ./src/cluster_setup/share_public_key.sh src/cluster_setup/ips_example.txt

3. Copy the program to all the cluster nodes in the same location
    ./src/cluster_setup/copy_files.sh src/cluster_setup/ips_example.txt build/taylor_cluster /home/franco/Documents

4. Run the program in the cluster
    mpirun -np 2 --hostfile src/cluster_setup/hostfile  /home/franco/Documents/taylor_cluster 1500000 10000000 > output.txt

*/

// This version uses standard arguments instead of console arguments, this way the
// arguments can be specified when executing the cluster
// mpirun -np 4 --hostfile hostfile ./your_program arg1, arg2, ..., argn > output.txt

#include <math.h>
#include <mpi/mpi.h>
#include <unistd.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

long double log_terms(
    uint64_t rank,
    long double x,
    uint64_t start_term_inclusive,
    uint64_t end_term_inclusive)
{
    long double sum = 0.0;

    // Extracts the pow base to avoid computing in the for loop
    long double pow_base = (x - 1.0) / (x + 1.0);

    // n ranges in [start, end]
    for (uint64_t n = start_term_inclusive; n <= end_term_inclusive; n++) {
        // The divisor and the pow exponent are the same
        long double divisor = 2.0 * n + 1.0;
        sum += powf64x(pow_base, divisor) / divisor;
    }

    // Saves result in vector
    return 2.0f * sum;
}

int main(int argc, char** argv)
{

    // User input data
    long double x = 1.0;
    uint64_t term_count = 1;

    // Initializes MPI by sending the arguments to all the nodes
    // ----------------------------------------------------------------------
    if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int root_rank = 0;

    if (rank == root_rank) {
        if (argc != 3) {
            std::cout << "The program expects 2 arguments. First X and then the amount of terms used for approximation\n";
            return 1;
        }

        x = atof(argv[1]);
        term_count = atol(argv[2]);
    }

    // Broadcasts parameters to all processes
    MPI_Bcast(&x, 1, MPI_LONG_DOUBLE, root_rank, MPI_COMM_WORLD);
    MPI_Bcast(&term_count, 1, MPI_UNSIGNED_LONG, root_rank, MPI_COMM_WORLD);

    // Each process calculates something using the broadcasted value
    uint64_t terms_x_thread = term_count / size;

    // Calculates result
    long double send_result = log_terms(
        rank,
        x,
        rank * terms_x_thread,
        rank * terms_x_thread + terms_x_thread - 1);

    std::cout << "Result in node of rank " << rank << ": " << std::setprecision(15) << send_result << std::endl;

    // Allocate space in root to gather results from all processes
    long double recv_buffer[size];

    // Gather the results in the root process
    int status = MPI_Gather(
        &send_result,
        1, // Each process sends one element
        MPI_LONG_DOUBLE, // Data type of the sent element
        recv_buffer, // Receive buffer (on root process)
        1, // Number of elements received from each process
        MPI_LONG_DOUBLE, // Data type of the received elements
        root_rank, // Root process that gathers data
        MPI_COMM_WORLD // MPI communicator
    );

    if (status != MPI_SUCCESS) {
        std::cout << "Error while gathering...\n";
        return 1;
    }

    if (rank == root_rank) {

        // Computes the result by adding all the terms
        long double result = 0.0;
        for (uint64_t process_rank = 0; process_rank < size; process_rank++) {
            result += recv_buffer[process_rank];
        }
        std::cout << "Result: " << std::setprecision(15) << result << std::endl;
    }

    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }

    return 0;
}