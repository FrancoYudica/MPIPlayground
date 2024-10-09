#!/bin/bash
# Script to copy a file to a list of remote computers using SCP
# The list in each line should contain the username and the IP

# Check if the correct number of arguments are provided
if [ "$#" -ne 3 ]; then
    echo "Usage: copy_files.sh ip_list.txt file_to_copy destination_path"
    exit 1
fi

# Set the file that contains the list of IPs (passed as the first parameter)
IP_FILE="$1"

# Set the file to copy (passed as the second parameter)
FILE_TO_COPY="$2"

# Set the destination path on the remote machines (passed as the third parameter)
DESTINATION_PATH="$3"

# Check if the IP list file exists
if [ ! -f "$IP_FILE" ]; then
    echo "The file $IP_FILE does not exist."
    exit 1
fi

# Check if the file to copy exists
if [ ! -f "$FILE_TO_COPY" ]; then
    echo "The file $FILE_TO_COPY does not exist."
    exit 1
fi

# Loop through each line in the IP list file
while IFS=' ' read -r username ip || [ -n "$username" ]; do
    # Skip empty lines
    if [ -z "$username" ] || [ -z "$ip" ]; then
        continue
    fi

    echo "Copying $FILE_TO_COPY to $username@$ip:$DESTINATION_PATH"
    
    # Run the scp command to copy the file
    scp "$FILE_TO_COPY" "$username@$ip:$DESTINATION_PATH"

    # Check if the SCP command was successful
    if [ "$?" -ne 0 ]; then
        echo "Error copying to $username@$ip"
    else
        echo "Successfully copied to $username@$ip"
    fi
done < "$IP_FILE"
