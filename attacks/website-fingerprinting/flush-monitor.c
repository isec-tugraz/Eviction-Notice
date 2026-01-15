#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "PageUtils.h"

uint64_t pages_in_cache(int file_fd, uint64_t n_pages);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s /file/to/monitor\n", argv[0]);
		return EXIT_FAILURE;
	}

	const char *file_path = argv[1];

	int file_fd = open(file_path, O_RDONLY);
	if (file_fd == -1) {
		perror("failed open file");
		exit(EXIT_FAILURE);
	}

	struct stat file_stat;
	if (fstat(file_fd, &file_stat) == -1) {
		perror("failed fstat comms file");
		exit(EXIT_FAILURE);
	}

	uint64_t n_pages = file_stat.st_size / page_size;

	while (1) {
		// Flush the whole file:
		if (posix_fadvise(file_fd, 0, 0, POSIX_FADV_DONTNEED)) {
			perror("receiver: posix_fadvise failed");
			exit(EXIT_FAILURE);
		}

		sleep(1);

		fprintf(stdout, "Num pages cached: [%lu/%lu]\n",
				pages_in_cache(file_fd, n_pages), n_pages);
	}

	close(file_fd);

	return EXIT_SUCCESS;
}

uint64_t pages_in_cache(int file_fd, uint64_t n_pages)
{
	uint64_t n_in_cache = 0;

	char buffer = 0;
	struct iovec iov[1];
	iov[0].iov_base = &buffer;
	iov[0].iov_len = 1;

	for (uint64_t page_num = 0; page_num < n_pages; ++page_num) {

		lseek(file_fd, 0, SEEK_SET);
		ssize_t ret =
			preadv2(file_fd, iov, 1, page_num * page_size, RWF_NOWAIT);

		if (ret < 0) {
			if (errno == EAGAIN) {
				n_in_cache += 0;
			} else {
				perror("preadv2 failed");
			}
		} else {
			n_in_cache += 1;
		}
	}

	return n_in_cache;
}