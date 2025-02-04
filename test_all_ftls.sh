#!/bin/bash

# Check if result directory exists and is non-empty
if [ ! -d "./result" ] || [ -z "$(ls -A ./result)" ]; then
    echo "No files in ./result"
    exit 1
fi

cache_memory=20

# Corrected variable name and used glob expansion safely
test_set=(./result/*_driver)

# Check if array is empty (no matching files)
if [ ${#test_set[@]} -eq 0 ]; then
    echo "No matching *_driver files found in ./result"
    exit 1
fi

echo "Drivers found: ${test_set[@]}"

for driver in "${test_set[@]}"
do
    output_file="${driver}_res.txt"

    # Construct base command
    cmd="$driver -T 0"

    if [[ "$driver" != *oftl_driver* ]]; then
        cmd+=" -p $cache_memory"
    fi

    # Append additional flags based on filename
    if [[ "$driver" == *dftl_driver* ]]; then
        cmd+=" -t 0"
    elif [[ "$driver" == *sftl_driver* ]]; then
        cmd+=" -t 2"
    fi

    echo "Running: $cmd > $output_file 2>&1"

    # Execute the command and redirect output
    $cmd > "$output_file" 2>&1

    echo "Done: $driver"
done

echo "Please check the results:"
ls ./result/*_res.txt

