#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "GeneralUtils.h"
#include "PageUtils.h"

int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <source_file> <destination_file>\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	const char *source_file = argv[1];
	const char *destination_file = argv[2];

	int src_fd = open(source_file, O_RDONLY);
	if (src_fd == -1) {
		perror("open source_file");
		return EXIT_FAILURE;
	}

	int dest_fd =
		open(destination_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (dest_fd == -1) {
		perror("open destination_file");
		close(src_fd);
		return EXIT_FAILURE;
	}

	off_t offset = PAGE_NUMBER * page_size;

	struct timespec start_time, end_time;

	////
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	ssize_t bytes_sent = sendfile(dest_fd, src_fd, &offset, 16);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	////

	print_timespec_diff(start_time, end_time);

	if (bytes_sent == -1) {
		perror("sendfile");
		close(src_fd);
		close(dest_fd);
		return EXIT_FAILURE;
	}

	close(src_fd);
	close(dest_fd);

	return EXIT_SUCCESS;
}
