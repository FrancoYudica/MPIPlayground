# Project Name

## Requirements Installation

To install the necessary dependencies for MPI, use the following command:

```bash
sudo apt install openmpi-bin libopenmpi-dev openmpi-common
```

## Code compilation

The code is compiled using the mpic++ compiler. Use the following command to compile your source code:

```bash
mpic++ {source.cpp} -o {output_name}
```

## Local Execution

To execute the program locally, use the following command, specifying the desired number of processes:

```bash
mpirun -np {slots} {program_filepath}
```

## Cluster Execution

### Compile the program with mpic++

Ensure that the program is compiled using the `mpic++` compiler as shown above.

### Enable Passwordless SSH

To enable passwordless SSH access to all nodes in the cluster, share your public key using the following script:

```bash
share_public_key.sh {cluster_users_ips.txt}
```

### Distribute the executable

To copy the compiled executable to all nodes, use the script:

```bash
copy_files.sh {cluster_users_ips.txt} {program_filepath} {destination_path}
```

### Run the Program on the Cluster

Create a **hostfile** that maps each node's IP address to its available slots. The hostfile format should look like this:

    10.0.0.128 slots=3
    10.0.0.132 slots=3

To run the distributed program, use the following command:

```bash
mpirun -np {slots} --hostfile {hostfile_filepath} {program_filepath} arg1 arg2 argN > output.txt
```

- `{slots}` Specify the total number of processes to run. Ensure this number is less than or equal to the total number of slots specified in the hostfile.
