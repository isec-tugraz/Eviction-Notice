#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "PageUtils.h"

int8_t page_in_cache(int fd, int64_t page_num)
{
	char buffer = 0;
	struct iovec iov[1];
	iov[0].iov_base = &buffer;
	iov[0].iov_len = 1;

	lseek(fd, 0, SEEK_SET);
	ssize_t ret = preadv2(fd, iov, 1, page_num * page_size, RWF_NOWAIT);

	int8_t in_cache = 2;

	if (ret < 0) {
		if (errno == EAGAIN) {
			// not in cache
			in_cache = 0;
		} else {
			perror("preadv2 failed");
		}
	} else if (ret == 1) {
		// We asked to read one byte, one byte was read
		in_cache = 1;
	} else {
		// something went wrong
		in_cache = -1;
	}

	return in_cache;
}

void exec_preadv2_mon_pages(int fd, int64_t n_arg_pages, int64_t *pages)
{
	int8_t *data = (int8_t *)malloc(n_arg_pages * sizeof(int8_t));

	for (int64_t i = n_arg_pages - 1; i >= 0; --i) {
		data[i] = page_in_cache(fd, pages[i]);
	}

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_arg_pages; ++i) {
		fprintf(stdout, "%04ld,  %d\n", pages[i], data[i]);
	}
}

void exec_preadv2_mon_total(int fd, int64_t n_file_pages)
{
	int8_t *data = (int8_t *)malloc(n_file_pages * sizeof(int8_t));

	for (int64_t i = n_file_pages - 1; i >= 0; --i) {
		data[i] = page_in_cache(fd, i);
	}

	uint64_t in_cache = 0;

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_file_pages; ++i) {
		fprintf(stdout, "%04ld,   %d\n", i, data[i]);

		in_cache += data[i];
	}

	fprintf(stdout, "\nTotal: %ld\n", n_file_pages);
	fprintf(stdout, "In cache: %ld\n", in_cache);
}