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

int main()
{
    // Initializes MPI
    // ----------------------------------------------------------------------
    int rank, size;
    if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Inputs data in root
    // ----------------------------------------------------------------------

    // Root rank for gathering user input, broadcasting and gathering results
    int root_rank = 0;

    // User input data
    long double x = 0.0f;
    uint64_t term_count = 1;

    if (rank == root_rank) {
        std::string input_string;

        std::cout << "Enter value X: ";
        std::cin >> input_string;
        x = std::stold(input_string);

        std::cout << "Amount of terms: ";
        std::cin >> input_string;
        term_count = std::stoul(input_string);

        std::cout << "Broadcasting \"X=" << x << "\" with \"" << term_count << "\" terms\n";
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

        // Root process prints the gathered results
        std::cout << "Results: ";
        for (int i = 0; i < size; i++) {
            std::cout << std::setprecision(15) << recv_buffer[i] << ", ";
        }
        std::cout << std::endl;

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