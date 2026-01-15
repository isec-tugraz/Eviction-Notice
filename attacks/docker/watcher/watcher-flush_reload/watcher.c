/* Flush+Reload specific pages of a file
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

// clang-format off
#ifndef READ_THRESHOLD
	#define READ_THRESHOLD 0.000030000
#endif
// clang-format on

uint64_t page_size = -1;

__attribute__((constructor)) int8_t load_page_size()
{
	int64_t ps = sysconf(_SC_PAGE_SIZE);

	if (ps == -1) {
		perror("load_page_size failed");
		page_size = (uint64_t)-1;
		return -1;
	}

	page_size = (uint64_t)ps;

	return 0;
}

int64_t *parse_page_list(int page_count, char *page_list[], bool *perf_all,
						 int n_file_pages);

int compare_uint64_t(const void *a, const void *b);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <file_path> [page_num]+...\n", argv[0]);
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

	int64_t n_arg_pages = argc - 2;

	bool flush_all = false;
	int64_t *pages =
		parse_page_list(n_arg_pages, &argv[2], &flush_all, n_file_pages);

	// We do things a little bit efficiently
	qsort(pages, n_arg_pages, sizeof(uint64_t), compare_uint64_t);

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(pages);
		return EXIT_FAILURE;
	}

	volatile char bufferfly = 0x12;

	int total_pages = flush_all ? n_file_pages : n_arg_pages;

	struct timespec *times = malloc(total_pages * sizeof(struct timespec));
	struct timespec end_time;

	while (1) {
		// FLUSH
		for (int64_t i = 0; i < total_pages; ++i) {
			int64_t page_index = flush_all ? i : pages[i];

			lseek(fd, page_index * page_size, SEEK_SET);

			posix_fadvise(fd, page_index * page_size, page_size,
						  POSIX_FADV_DONTNEED);
		}

		// wait
		sleep(1);

		// RELOAD
		char ch;

		for (int64_t i = total_pages - 1; i >= 0; --i) {
			int64_t page_index = flush_all ? i : pages[i];

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

			bufferfly ^= ch;
		}

		fprintf(stdout, "==================================================\n");
		fprintf(stdout, "page, time [s],  cache\n");
		for (int64_t i = 0; i < total_pages; ++i) {
			double page_time =
				(double)times[i].tv_sec + (double)times[i].tv_nsec / 1e9;

			fprintf(stdout, "%04ld, %ld.%09ld, %s\n", i, times[i].tv_sec,
					times[i].tv_nsec,
					page_time > READ_THRESHOLD ? "\033[0;31m0\033[0m"
											   : "\033[0;32m-> 1\033[0m");
		}
	}

	return EXIT_SUCCESS;
}

int64_t auto_strtoll(const char *str)
{
	char *endptr;

	// Base 0 does automatic base detection POGCHAMP + HELL YEAH
	int64_t value = strtoll(str, &endptr, 0);

	if (errno == ERANGE) {
		perror("Value out of range");
		return 0;
	}
	if (errno == EINVAL) {
		fprintf(stderr, "No digits or weird base???\n");
		return 0;
	}

	return value;
}

int64_t *parse_page_list(int page_count, char *page_list[], bool *perf_all,
						 int n_file_pages)
{
	int64_t *pages = malloc(page_count * sizeof(int64_t));
	*perf_all = false;

	for (int i = 0; i < page_count; ++i) {
		int64_t page_val = auto_strtoll(page_list[i]);

		// negative numbers = perform action on the whole file
		if (page_val < 0) {
			*perf_all = true;
		}

		// anything greater than n_file_pages gets clipped
		if (page_val > n_file_pages - 1) {
			page_val = n_file_pages - 1;
		}

		pages[i] = page_val;
	}

	return pages;
}

int compare_uint64_t(const void *a, const void *b)
{
	uint64_t num1 = *(uint64_t *)a;
	uint64_t num2 = *(uint64_t *)b;

	if (num1 < num2)
		return -1;
	if (num1 > num2)
		return 1;
	return 0;
}