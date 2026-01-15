/* Reports page presence in cache.
 *
 * Args:
 * 		- /path/to/file
 * 		- pages of file (separated by space): -1 represents the whole file
 */

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
#include <unistd.h>

#include "CacheStat.h"
#include "GeneralUtils.h"
#include "PageUtils.h"

// clang-format off
#ifndef MON_CACHESTAT
	#ifndef MON_PREADV2
		#define MON_PREADV2
	#endif
#endif
// clang-format on

void exec_cachestat_pages(int fd, int64_t n_arg_pages, int64_t *pages);
void exec_cachestat_total(int fd, int64_t n_file_pages);
void exec_preadv2_mon_pages(int fd, int64_t n_arg_pages, int64_t *pages);
void exec_preadv2_mon_total(int fd, int64_t n_file_pages);

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

	bool monitor_all = false;
	int64_t *pages =
		parse_page_list(n_arg_pages, &argv[2], &monitor_all, n_file_pages);

	// We do things a little bit efficiently
	qsort(pages, n_arg_pages, sizeof(uint64_t), compare_uint64_t);

	int fd;
	fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(pages);
		return EXIT_FAILURE;
	}

	/////////////////////////////////// POC ////////////////////////////////////
#ifdef MON_CACHESTAT
	if (monitor_all) {
		exec_cachestat_total(fd, n_file_pages);
	} else {
		exec_cachestat_pages(fd, n_arg_pages, pages);
	}

#elif MON_PREADV2
	if (monitor_all) {
		exec_preadv2_mon_total(fd, n_file_pages);
	} else {
		exec_preadv2_mon_pages(fd, n_arg_pages, pages);
	}
#endif

	return EXIT_SUCCESS;
}