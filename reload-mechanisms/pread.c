#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "GeneralUtils.h"
#include "PageUtils.h"

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	ssize_t bytes_read;
	char buffer[16];

	const char *source_file = argv[1];

	int src_fd = open(source_file, O_RDONLY);
	if (src_fd == -1) {
		perror("open source_file");
		return EXIT_FAILURE;
	}

	off_t offset = PAGE_NUMBER * page_size;
	struct timespec start_time, end_time;

	////
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	bytes_read = pread(src_fd, buffer, sizeof(buffer), offset);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	////

	print_timespec_diff(start_time, end_time);

	if (bytes_read == -1) {
		perror("pread");
		close(src_fd);
		return EXIT_FAILURE;
	}

	close(src_fd);
	return EXIT_SUCCESS;
}
