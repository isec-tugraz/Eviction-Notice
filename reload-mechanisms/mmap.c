#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
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
		exit(EXIT_FAILURE);
	}

	off_t file_size = lseek(src_fd, 0, SEEK_END);
	if (file_size == -1) {
		perror("Error seeking to end of file");
		close(src_fd);
		return 1;
	}

	char *mapped_mem = mmap(NULL, file_size, PROT_READ, MAP_SHARED, src_fd, 0);

	if ((unsigned long)mapped_mem % 4096 != 0) {
		fprintf(stderr, "Unaligned address: %p\n", mapped_mem);
	}

	off_t offset = PAGE_NUMBER * page_size;
	struct timespec start_time, end_time;

	////
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	volatile char buffer = mapped_mem[offset];
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
	////

	print_timespec_diff(start_time, end_time);

	munmap(mapped_mem, file_size);

	close(src_fd);
	return EXIT_SUCCESS;

	// Just to make the compiler stop complaining about unused variable
	fprintf(stderr, "%d", buffer);
}
