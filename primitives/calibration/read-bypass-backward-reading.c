/* Reads a page into the cache using the readahead syscall to bypass the
 * read-ahead mechanism.
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
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
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

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return EXIT_FAILURE;
	}

	struct timespec *times = malloc(n_file_pages * sizeof(struct timespec));
	memset(times, 0x0, n_file_pages * sizeof(struct timespec));
	struct timespec end_time;

	char ch = 0x01;
	volatile char bufferfly = 0x24;

	// Flushed the whole file
	posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

	double time_not_cache = 0.0;

	for (int64_t i = n_file_pages - 1; i >= 0; --i) {
		int64_t page_index = i;

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

		double page_time =
			(double)times[i].tv_sec + (double)times[i].tv_nsec / 1e9;

		time_not_cache += page_time;

		bufferfly ^= ch; // I've noticed that the compiler sometimes
						 // optimizes the read out if it realizes that ch
						 // isn't being used.
	}

	double time_in_cache = 0.0;

	for (int64_t i = n_file_pages - 1; i >= 0; --i) {
		int64_t page_index = i;

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

		double page_time =
			(double)times[i].tv_sec + (double)times[i].tv_nsec / 1e9;

		time_in_cache += page_time;

		bufferfly ^= ch; // I've noticed that the compiler sometimes
						 // optimizes the read out if it realizes that ch
						 // isn't being used.
	}

	double avg_in_cache = time_in_cache / n_file_pages;
	double avg_not_in_cache = time_not_cache / n_file_pages;

	fprintf(stdout, "In Cache: %.9f\n", avg_in_cache);
	fprintf(stdout, "Not in Cache: %.9f\n", avg_not_in_cache);

	fprintf(stdout, "\nREAD_THRESHOLD := %.9f\n",
			(avg_in_cache + avg_not_in_cache) / 2);

	return EXIT_SUCCESS;
}
