#!/bin/bash

output_file="/tmp/ndss26-fallae-2.bin"
output_size=$((4096 * 64)) # 64 pages

# If the file already exists:
if [ -e "$output_file" ]; then
    current_size=$(stat -c%s "$output_file")

    if [ "$current_size" -eq "$output_size" ]; then
        echo "File $output_file already exists and size is the same. Quitting now..."
        echo -e "\n==============================================="
        echo "Use: $output_file"
        echo -e "==============================================="
        exit 0
    else
        echo "File $output_file exists, but size is different."
        read -p "Do you want to overwrite the existing file? (y/n): " choice
        
        if [[ "$choice" != [Yy] ]]; then
            echo "Exiting without overwriting the file."
            exit 0
        fi
    fi
fi

echo "Creating $output_file with 64 pages of random data..."

if dd if=/dev/urandom of="$output_file" bs=4096 count=64; then
    echo "File created!"

    echo -e "\n==============================================="
    echo "Use: $output_file"
    echo -e "==============================================="
else
    echo "Failed to create $output_file"
fi
