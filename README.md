Requirements installation:

    sudo apt install openmpi-bin libopenmpi-dev openmpi-common

The code is compiled with mpic++ compiler.

    mpic++ {source.cpp} -o {output_name}

For executing locally:

    mpirun -np {process_count} {program_filepath}
