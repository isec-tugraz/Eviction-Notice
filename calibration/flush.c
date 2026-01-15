/* Flush specific pages of a file
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
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "PageUtils.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];

	struct stat st;
	if (stat(file_path, &st) == -1) {
		perror("Unable to get file size");
		return EXIT_FAILURE;
	}

	int64_t file_size = st.st_size;
	int64_t n_file_pages = file_size / page_size;
	if (!n_file_pages)
		n_file_pages = 1;

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return EXIT_FAILURE;
	}
	struct timespec *times = malloc(n_file_pages * sizeof(struct timespec));
	struct timespec end_time;

	// Flushed the whole file
	posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

	double time_not_cache = 0.0;

	for (int64_t i = 0; i < n_file_pages; ++i) {
		int64_t page_index = i;

		// Timing a flush: not in cache
		clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);
		posix_fadvise(fd, page_index * page_size, page_size,
					  POSIX_FADV_DONTNEED);
		clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

		times[i].tv_sec = end_time.tv_sec - times[i].tv_sec;
		times[i].tv_nsec = end_time.tv_nsec - times[i].tv_nsec;

		if (times[i].tv_nsec < 0) {
			times[i].tv_sec--;
			times[i].tv_nsec += 1e9;
		}

		double page_time =
			(double)times[i].tv_sec + (double)times[i].tv_nsec / 1e9;

		time_not_cache += page_time;
	}

	// Read!
	char ch = 0x01;
	volatile char bufferfly = 0x24;

	for (int64_t i = 0; i < n_file_pages; ++i) {
		int64_t page_index = i;

		lseek(fd, page_index * page_size, SEEK_SET);

		clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);
		int64_t ret = read(fd, &ch, sizeof(ch));
		clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

		if (ret != 1)
			perror("read");

		bufferfly ^= ch;
	}

	double time_in_cache = 0.0;

	for (int64_t i = 0; i < n_file_pages; ++i) {
		int64_t page_index = i;

		// Timing a flush: in cache
		clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);
		posix_fadvise(fd, page_index * page_size, page_size,
					  POSIX_FADV_DONTNEED);
		clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

		times[i].tv_sec = end_time.tv_sec - times[i].tv_sec;
		times[i].tv_nsec = end_time.tv_nsec - times[i].tv_nsec;

		if (times[i].tv_nsec < 0) {
			times[i].tv_sec--;
			times[i].tv_nsec += 1e9;
		}

		double page_time =
			(double)times[i].tv_sec + (double)times[i].tv_nsec / 1e9;

		time_in_cache += page_time;
	}

	double avg_in_cache = time_in_cache / n_file_pages;
	double avg_not_in_cache = time_not_cache / n_file_pages;

	fprintf(stdout, "In Cache: %.9f\n", avg_in_cache);
	fprintf(stdout, "Not in Cache: %.9f\n", avg_not_in_cache);

	fprintf(stdout, "\nFLUSH_THRESHOLD := %.9f\n",
			(avg_in_cache + avg_not_in_cache) / 2);

	return EXIT_SUCCESS;
}
