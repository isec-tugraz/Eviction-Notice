#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "CacheStat.h"
#include "PageUtils.h"

void exec_cachestat_pages(int fd, int64_t n_arg_pages, int64_t *pages)
{
	struct cachestat *cs =
		(struct cachestat *)malloc(n_arg_pages * sizeof(struct cachestat));

	for (int64_t i = 0; i < n_arg_pages; ++i) {
		struct cachestat_range cs_range = {.off = pages[i] * page_size,
										   .len = page_size};

		if (syscall(SYSCALL_CACHESTAT, fd, &cs_range, &cs[i], 0) == -1) {
			perror("cachestat failed :(");
		}
	}

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_arg_pages; ++i) {
		fprintf(stdout, "%04ld,   %lu\n", pages[i], cs[i].nr_cache);
	}
}

void exec_cachestat_total(int fd, int64_t n_file_pages)
{
	struct cachestat *cs =
		(struct cachestat *)malloc(n_file_pages * sizeof(struct cachestat));

	// going page-by-page. you can do off 0, len 0 to get whole file stats,
	// but we'd like to see the page-by-page stats.
	for (int64_t i = 0; i < n_file_pages; ++i) {
		struct cachestat_range cs_range = {.off = i * page_size,
										   .len = page_size};

		if (syscall(SYSCALL_CACHESTAT, fd, &cs_range, &cs[i], 0) == -1) {
			perror("cachestat failed :(");
		}
	}

	uint64_t in_cache = 0;

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_file_pages; ++i) {
		fprintf(stdout, "%04ld,   %lu\n", i, cs[i].nr_cache);

		in_cache += cs[i].nr_cache;
	}

	fprintf(stdout, "\nTotal: %ld\n", n_file_pages);
	fprintf(stdout, "In cache: %ld\n", in_cache);
}