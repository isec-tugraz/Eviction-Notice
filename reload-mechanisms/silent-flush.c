/* Flushes a whole a file */

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

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return EXIT_FAILURE;
	}

	posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

	return EXIT_SUCCESS;
}
