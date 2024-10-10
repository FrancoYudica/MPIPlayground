/* Compilation
mkdir build
cd build
g++ -std=c++11 -pthread -O3 -o ejercicio2.out ../src/ejercicio2.cpp
*/

/* Command for testing results
grep -o `pattern` `file` | wc -l
*/

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mpi/mpi.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>

int32_t s_rank;
int32_t s_size;
const uint32_t s_root_rank = 0;

/// The most basic pattern matching method O(len(src) * len(pattern))
/// Returns the start index of the first match or -1 if not found
int32_t pattern_match(
    const std::string& src,
    const std::string& pattern)
{

    for (uint32_t i = 0; i <= src.length() - pattern.size(); i++) {
        if (!strncmp(src.c_str() + i, pattern.c_str(), pattern.size()))
            return i;
    }
    return -1;
}

/// Given an open file, counts the amount of occurrences of the given pattern
uint32_t file_pattern_match(
    std::shared_ptr<std::ifstream> file,
    const std::string pattern)
{

    char buffer[2048];
    uint32_t remainder = 0;
    uint32_t count = 0;

    while (file->read(buffer + remainder, sizeof(buffer) - remainder) || file->gcount() > 0) {

        uint32_t bytes_read = remainder + file->gcount();
        // Null-terminate the buffer for safe string operations
        buffer[bytes_read] = '\0';

        std::string line(buffer);

        int32_t match_index = pattern_match(line, pattern);

        // The pattern was found
        if (match_index != -1) {
            remainder = line.size() - match_index - pattern.size();
            count += 1;
        }

        // In case it did match, adds some remainder for patterns that
        // lay between buffer reads. Super important!
        else
            remainder = pattern.size();

        // Amount of chars read for finding the pattern
        uint32_t read_chars = line.size() - remainder;

        memmove(buffer, line.c_str() + read_chars, remainder);
    }
    return count;
}

void count_single_threaded(
    const std::vector<std::string>& patterns,
    const std::string& pattern_match_filepath)
{

    // Pattern matching for all the patterns
    for (uint32_t pattern_index = 0; pattern_index < patterns.size(); pattern_index++) {
        auto match_file = std::make_shared<std::ifstream>(pattern_match_filepath);

        if (!match_file->is_open()) {
            std::cerr << "Error opening file" << std::endl;
            return;
        }

        const std::string pattern = patterns[pattern_index];

        uint32_t count = file_pattern_match(
            match_file,
            pattern);

        match_file->close();
        std::cout << "Node " << s_rank << ", processed \"" << patterns[pattern_index] << "\": " << count << std::endl;
    }
}

/// @brief Returns a vector of the patterns that should be processed by the node
std::vector<std::string> scatter_patterns(const std::vector<std::string>& all_patterns)
{

    uint32_t max_pattern_size = 32;

    for (auto& pattern : all_patterns) {
        if (pattern.size() > max_pattern_size)
            max_pattern_size = pattern.size();
    }

    // Broadcasts the total amount of patterns ---------------------------
    uint32_t total_patterns = all_patterns.size();
    MPI_Bcast(&total_patterns, 1, MPI_INT, s_root_rank, MPI_COMM_WORLD);

    uint32_t patterns_per_process = (total_patterns + s_size - 1) / s_size; // Rounded up

    // Prepare the send buffer for root (flatten the strings into a char array)
    std::vector<char> send_buffer;
    if (s_rank == s_root_rank) {
        send_buffer.resize(patterns_per_process * s_size * max_pattern_size, '\0'); // Null-padded buffer

        for (uint32_t i = 0; i < total_patterns; ++i) {
            std::copy(all_patterns[i].begin(), all_patterns[i].end(),
                send_buffer.begin() + i * max_pattern_size);
        }
    }

    // Each process will receive a buffer for its patterns
    std::vector<char> recv_buffer(patterns_per_process * max_pattern_size, '\0');

    // Scatter the flattened patterns to all processes
    MPI_Scatter(
        send_buffer.data(), patterns_per_process * max_pattern_size, MPI_CHAR,
        recv_buffer.data(), patterns_per_process * max_pattern_size, MPI_CHAR,
        0, MPI_COMM_WORLD);

    // Reconstruct the local patterns from the received buffer
    std::vector<std::string> local_patterns;
    for (int i = 0; i < patterns_per_process; ++i) {
        std::string pattern(
            recv_buffer.begin() + i * max_pattern_size,
            recv_buffer.begin() + (i + 1) * max_pattern_size);
        pattern = pattern.c_str(); // Truncate any null padding
        if (!pattern.empty()) {
            local_patterns.push_back(pattern);
            std::cout << "Rank " << s_rank << " received pattern: " << pattern << std::endl;
        }
    }

    return local_patterns;
}

int main(int argc, char** argv)
{

    // Initializes MPI
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &s_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &s_size);

    std::string patterns_filepath;
    std::string pattern_match_filepath;

    if (s_rank == s_root_rank) {
        if (argc != 3) {
            std::cout << "The program expects 2 arguments. "
                      << "First the filepath that contains all the patterns, "
                      << "and second the filepath that contains the matching file\n";
            MPI_Finalize();
            return 1;
        }
        patterns_filepath = argv[1];
        pattern_match_filepath = argv[2];
    }

    // Broadcasts the pattern matching filepath -----------------------------------------------------------
    // Broadcast the length of the string first
    int length = pattern_match_filepath.size();
    MPI_Bcast(&length, 1, MPI_INT, s_root_rank, MPI_COMM_WORLD);

    // Resize the string on other ranks to accommodate the incoming data
    if (s_rank != s_root_rank)
        pattern_match_filepath.resize(length);

    // Broadcast the actual string data (as a char array)
    MPI_Bcast(&pattern_match_filepath[0], length, MPI_CHAR, s_root_rank, MPI_COMM_WORLD);

    // Now all ranks have `pattern_match_filepath`
    std::cout << "Rank " << s_rank << " received filepath: " << pattern_match_filepath << std::endl;

    /// Reads the patterns ----------------------------------------------
    std::vector<std::string> patterns;

    if (s_rank == s_root_rank) {
        auto patterns_file = std::ifstream(patterns_filepath);

        if (!patterns_file.is_open()) {
            std::cerr << "Error opening file" << std::endl;
            return 1;
        }

        std::string line;
        while (std::getline(patterns_file, line)) {
            patterns.push_back(line);
        }

        patterns_file.close();
    }

    /// Gets the patterns that the node should process
    std::vector<std::string> local_patterns = scatter_patterns(patterns);

    // Processes the patterns
    count_single_threaded(local_patterns, pattern_match_filepath);

    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }

    return 0;
}