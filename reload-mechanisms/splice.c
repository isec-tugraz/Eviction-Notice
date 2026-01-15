#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
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
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *source_file = argv[1];

	int src_fd = open(source_file, O_RDONLY);
	if (src_fd == -1) {
		perror("open source_file");
		return EXIT_FAILURE;
	}

	off_t offset = PAGE_NUMBER * page_size;

	struct timespec start_time, end_time;

	lseek(src_fd, offset, SEEK_SET);

	int pipe_fd[2];
	if (pipe(pipe_fd) == -1) {
		perror("pipe");
		close(src_fd);
		return EXIT_FAILURE;
	}

	////
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	ssize_t bytes_spliced = splice(src_fd, NULL, pipe_fd[1], NULL, 16, 0);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	////

	print_timespec_diff(start_time, end_time);

	if (bytes_spliced < 0) {
		perror("splice");
		close(src_fd);
		close(pipe_fd[0]);
		close(pipe_fd[1]);
		return EXIT_FAILURE;
	}

	close(pipe_fd[1]);
	close(pipe_fd[0]);

	close(src_fd);
	return EXIT_SUCCESS;
}
