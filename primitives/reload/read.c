/* Reads a page into the cache
 *
 * Args:
 * 		- /path/to/file
 * 		- pages of file (separated by space): -1 represents the whole file
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "GeneralUtils.h"
#include "PageUtils.h"

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <file_path> [page_num]+...\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *file_path = argv[1];

	struct stat st;
	if (stat(file_path, &st) == -1) {
		perror("Unable to get file size");
		return EXIT_FAILURE;
	}

	int64_t file_size = st.st_size;
	int64_t n_file_pages = file_size / page_size;
	if (!n_file_pages)
		n_file_pages = 1;

	int64_t n_arg_pages = argc - 2;

	bool read_all = false;
	int64_t *pages =
		parse_page_list(n_arg_pages, &argv[2], &read_all, n_file_pages);

	// We DON'T things a little bit efficiently. Order matters here!
	// qsort(pages, n_arg_pages, sizeof(uint64_t), compare_uint64_t);

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(pages);
		return EXIT_FAILURE;
	}

	int total_pages = read_all ? n_file_pages : n_arg_pages;
	struct timespec *times = malloc(total_pages * sizeof(struct timespec));
	struct timespec end_time;

	char ch = 0x01;
	volatile char bufferfly = 0x24;

	for (int64_t i = 0; i < total_pages; ++i) {
		int64_t page_index = read_all ? i : pages[i];

		lseek(fd, page_index * page_size, SEEK_SET);

		clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);
		int64_t ret = read(fd, &ch, sizeof(ch));
		clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

		if (ret != 1)
			perror("read");

		times[i].tv_sec = end_time.tv_sec - times[i].tv_sec;
		times[i].tv_nsec = end_time.tv_nsec - times[i].tv_nsec;

		if (times[i].tv_nsec < 0) {
			times[i].tv_sec--;
			times[i].tv_nsec += 1e9;
		}

		bufferfly ^= ch; // I've noticed that the compiler sometimes
						 // optimizes the read out if it realizes that ch
						 // isn't being used.
	}

	fprintf(stdout, "page, time [s]\n");
	for (int64_t i = 0; i < total_pages; ++i) {
		fprintf(stdout, "%04ld, %ld.%09ld\n", i, times[i].tv_sec,
				times[i].tv_nsec);
	}

	return EXIT_SUCCESS;
}
