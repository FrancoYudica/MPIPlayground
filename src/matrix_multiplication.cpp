/* Compilation
mkdir build
cd build
g++ -std=c++11 -pthread -O3 -o ejercicio3.out ../src/ejercicio3.cpp
*/

#include <iomanip>
#include <iostream>
#include <mpi/mpi.h>
#include <string.h>
#include <thread>
#include <vector>

class Matrix {
    std::vector<std::vector<float>> data;

public:
    Matrix(
        std::size_t size)
        : data(size, std::vector<float>(size))
    {
    }

    Matrix(
        std::size_t rows,
        std::size_t columns)
        : data(rows, std::vector<float>(columns))
    {
    }

    ~Matrix()
    {
        data.clear();
    }

    inline std::size_t nrows() const { return data.size(); }
    inline std::size_t ncolumns() const { return data[0].size(); }

    void print(uint32_t precision = 3)
    {
        for (uint32_t row = 0; row < nrows(); row++) {
            std::cout << "[ ";
            for (uint32_t column = 0; column < ncolumns(); column++) {
                std::cout << std::setprecision(precision) << data[row][column];
                if (column + 1 < ncolumns())
                    std::cout << ", ";
            }
            std::cout << " ]" << std::endl;
        }
    }

    void fill(float n)
    {
        for (uint32_t row = 0; row < nrows(); row++)
            data[row].assign(ncolumns(), n);
    }

    std::vector<float>& operator[](uint32_t index)
    {
        return data[index];
    }

    const std::vector<float>& operator[](uint32_t index) const
    {
        return data[index];
    }

    float sum_elements()
    {
        float sum = 0.0f;
        for (uint32_t row = 0; row < nrows(); row++) {
            for (uint32_t column = 0; column < ncolumns(); column++)
                sum += data[row][column];
        }
        return sum;
    }
};

Matrix matrix_mult_multithread(
    const Matrix& a,
    const Matrix& b,
    uint32_t start_column,
    uint32_t end_column)
{

    if (a.ncolumns() != b.nrows()) {
        std::cout << "Can't multiply matrices. Incompatible rows and columns sizes" << std::endl;
        return a;
    }

    Matrix c(a.nrows(), b.ncolumns());
    c.fill(0.0f);

    for (std::size_t row = 0; row < c.nrows(); row++) {
        for (std::size_t column = start_column; column < end_column; column++) {
            c[row][column] = 0.0f;
            for (std::size_t i = 0; i < a.ncolumns(); i++) {
                c[row][column] += a[row][i] * b[i][column];
            }
        }
    }

    return c;
}

int main(int argc, char** argv)
{

    // ----------------------------------------------------------------------
    if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        std::cout << "Error while initializing MPI" << std::endl;
        return 1;
    }

    int32_t rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int root_rank = 0;
    uint32_t matrix_size = 0;
    if (rank == root_rank) {
        if (argc != 2) {
            std::cout << "Node " << rank << " The program expects 1 argument. The matrix size\n";
            MPI_Finalize();
            return 1;
        }
        matrix_size = atol(argv[1]);
    }

    // Broadcasts the matrix size
    MPI_Bcast(&matrix_size, 1, MPI_UNSIGNED_LONG, root_rank, MPI_COMM_WORLD);

    Matrix a(matrix_size);
    Matrix b(matrix_size);
    a.fill(0.1f);
    b.fill(0.2f);

    // Ceil operation
    uint32_t columns_per_node = (matrix_size + size - 1) / size;

    uint32_t start_column = rank * columns_per_node; // Closed interval
    uint32_t end_column = (rank + 1) * columns_per_node; // Open interval

    // For the last node, ensures that it doesn't try to compute extra columns
    if (rank == size - 1)
        end_column = matrix_size;

    std::cout << "Node " << rank << " is computing column range: (" << start_column << ", " << end_column << ")\n";

    // Executes multiplication
    Matrix c = matrix_mult_multithread(
        a,
        b,
        start_column,
        end_column);

    // Sums the elements and gathers results
    float elements_sum = c.sum_elements();

    float sums[size];
    MPI_Gather(
        &elements_sum,
        1,
        MPI_FLOAT,
        sums,
        1,
        MPI_FLOAT,
        root_rank,
        MPI_COMM_WORLD);

    if (rank == root_rank) {
        // Computes the result by adding all the terms
        float result = 0.0f;
        for (uint32_t i = 0; i < size; i++)
            result += sums[i];
        std::cout << "Result: " << std::setprecision(15) << result << std::endl;
    }

    if (MPI_Finalize() != MPI_SUCCESS) {
        std::cout << "Error finalizing MPI" << std::endl;
        return 1;
    }
    return 0;
}