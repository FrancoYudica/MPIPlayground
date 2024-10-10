/* Compilation
mkdir build
cd build
g++ -std=c++11 -pthread -O3 -o ejercicio4.out ../src/ejercicio4.cpp
*/

#include <algorithm>
#include <iostream>
#include <math.h>
#include <mpi/mpi.h>
#include <mutex>
#include <thread>
#include <vector>

bool is_prime(uint64_t n)
{
    if (n < 2)
        return false;

    for (uint64_t i = 2; i <= std::sqrt(n); i++) {
        if (n % i == 0)
            return false;
    }
    return true;
}

std::vector<uint64_t> find_primes_ranged(
    uint64_t start,
    uint64_t end)
{
    std::vector<uint64_t> prime_nums;
    for (uint64_t n = start; n < end; n++) {
        if (is_prime(n))
            prime_nums.push_back(n);
    }
    return prime_nums;
}

template <typename T>
void print_vector(const std::vector<T>& vec)
{
    std::cout << "[";

    for (uint64_t i = 0; i < vec.size(); i++) {
        std::cout << vec[i];

        if (i + 1 < vec.size())
            std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

int main(int argc, char** argv)
{

    // Initializes MPI
    // ----------------------------------------------------------------------
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int root_rank = 0;
    uint64_t max_num = 0;
    if (rank == root_rank) {
        if (argc != 2) {
            std::cout << "The program expects 1 arguments, the maximum number tested\n";
            MPI_Finalize();
            return 1;
        }

        max_num = atol(argv[1]);
        std::cout << "Finding numbers in range [0, " << max_num << "]" << std::endl;
    }

    // Broadcasts the maximum number
    MPI_Bcast(&max_num, 1, MPI_LONG_LONG_INT, root_rank, MPI_COMM_WORLD);

    // Calculates the range [start, end) for the node
    uint64_t numbers_per_node = (max_num + size - 1) / size;
    uint64_t start_num = rank * numbers_per_node;
    uint64_t end_num = (rank + 1) * numbers_per_node;

    if (rank == size - 1)
        end_num = max_num;

    std::vector<uint64_t> prime_numbers = find_primes_ranged(start_num, end_num);

    // Gathers the count of prime numbers found each node ----------------------------------------
    uint64_t prime_number_count_per_node[size];
    uint64_t prime_number_count = prime_numbers.size();
    MPI_Gather(
        &prime_number_count,
        1,
        MPI_LONG_LONG,
        prime_number_count_per_node,
        1,
        MPI_LONG_LONG,
        root_rank,
        MPI_COMM_WORLD);

    if (root_rank == rank) {
        for (uint32_t i = 0; i < size; i++) {
            std::cout << "Node " << i << " found " << prime_number_count_per_node[i] << std::endl;
        }
    }

    // Setup data to use Gather vary and gather all the found prime numbers ----------------------
    std::vector<int32_t> recv_counts(size); // How many numbers each process will send
    std::vector<int32_t> displacements(size); // Offsets of where to place the data
    uint64_t all_prime_numbers_count = 0;

    if (rank == root_rank) {
        uint64_t offset = 0;
        for (uint32_t i = 0; i < size; i++) {
            recv_counts[i] = prime_number_count_per_node[i];
            displacements[i] = offset;
            offset += prime_number_count_per_node[i];
        }

        all_prime_numbers_count = offset;
    }

    // Gathers the prime numbers of all the nodes ------------------------------------------------
    // Creates a vector with the required capacity to hold all the prime numbers
    std::vector<uint64_t> all_prime_numbers(all_prime_numbers_count);

    // Gathering in this way ensures that the vector `all_prime_numbers` is sorted, since
    // the ranges are sorted by the rank
    MPI_Gatherv(
        prime_numbers.data(),
        prime_number_count,
        MPI_LONG_LONG,
        all_prime_numbers.data(),
        (const int32_t*)recv_counts.data(),
        (const int32_t*)displacements.data(),
        MPI_LONG_LONG,
        root_rank,
        MPI_COMM_WORLD);

    // print_vector(all_prime_numbers);

    // Displays results
    if (rank == root_rank) {
        uint8_t display_count = 10;
        std::cout << "Largest " << display_count << " numbers: [";
        for (uint8_t i = 0; i < display_count; i++) {
            auto ptr = all_prime_numbers.end() - i - 1;
            std::cout << *ptr;
            if (i + 1 < display_count)
                std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }

    return 0;
}
