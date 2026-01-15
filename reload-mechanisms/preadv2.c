#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "GeneralUtils.h"
#include "PageUtils.h"

#define BUFFER_SIZE_1 16

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *source_file = argv[1];

	int src_fd = open(source_file, O_RDONLY);
	if (src_fd == -1) {
		perror("open source_file");
		return EXIT_FAILURE;
	}

	char buffer1[BUFFER_SIZE_1];

	struct iovec iov;
	iov.iov_base = buffer1;
	iov.iov_len = sizeof(buffer1);

	int flag = 0;

	off_t offset = PAGE_NUMBER * page_size;

	struct timespec start_time, end_time;

	////
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	ssize_t bytes_read = preadv2(src_fd, &iov, 1, offset, flag);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	////

	print_timespec_diff(start_time, end_time);

	if (bytes_read == -1) {
		perror("preadv2");
		close(src_fd);
		return EXIT_FAILURE;
	}

	close(src_fd);
	return EXIT_SUCCESS;
}