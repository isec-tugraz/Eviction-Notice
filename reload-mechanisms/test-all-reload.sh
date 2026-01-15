#!/bin/bash

FLUSH_PATH=./build/silent-flush
FILE_PATH=/tmp/ndss26-fallae-2.bin
TEMP_COPY_PATH=/tmp/ndss26-fallae-2-subset-copied.bin

function test_reload {
    echo "====================================================================="
    
    command_output=$(echo "$@")
    
    echo "$command_output"
    printf '%*s\n' "${#command_output}" '' | tr ' ' '-'
    
    # Flushing the file
    $FLUSH_PATH $FILE_PATH

    echo "[Flushed]"
    "$@"
    
    echo "[After Reload]"
    "$@"

    echo -e "\n"
}

test_reload ./build/copy_file_range $FILE_PATH $TEMP_COPY_PATH
test_reload ./build/mmap $FILE_PATH
test_reload ./build/posix_fadvise-willneed $FILE_PATH
test_reload ./build/pread $FILE_PATH
test_reload ./build/preadv $FILE_PATH
test_reload ./build/preadv2 $FILE_PATH
test_reload ./build/read $FILE_PATH
test_reload ./build/readahead $FILE_PATH
test_reload ./build/readv $FILE_PATH
test_reload ./build/sendfile $FILE_PATH $TEMP_COPY_PATH
test_reload ./build/splice $FILE_PATH