/* Takes n+ arguments, each a path to a shared library (essentially a file). Use
 * the adjacent shell script: ./list-all-shlibs.sh.
 *
 * Run as:
 *      ./evict-1-shlibs $(./utils/list-all-shlibs.sh)
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "PageUtils.h"

#define USLEEP_TIME 2000 // 2 milliseconds

int main(int argc, char *argv[])
{
	int return_value = 0;

	if (argc == 1) {
		printf("argument(s) needed: path to file(s) for reading constantly");
		errno = EINVAL;
		return_value = -1;
		goto end;
	}

	////////////////////////////////////////////////////////////////////////////
	// So! Sometimes, and IDK WHY this happens, we get shared libraries
	// reported, but they don't exist at the path... so we're gonna manually
	// trim them down.
	// This is thanks to snap btw :D
	uint32_t n_init_files = argc - 1;
	uint32_t n_files = n_init_files;

	fprintf(stdout, "Iterating over %d files...\n", n_init_files);

	// Okay, bear with me but the logic is messy.
	// All file descriptors
	int32_t *file_descriptors =
		(int32_t *)malloc(n_init_files * sizeof(int32_t));

	// Each file's stat
	struct stat *fs_stats =
		(struct stat *)malloc(n_init_files * sizeof(struct stat));

	// Number of pages in each file
	uint32_t *fd_n_pages = (uint32_t *)malloc(n_init_files * sizeof(uint32_t));

	// Each file's mapping
	char **mappings = (char **)malloc(n_init_files * sizeof(char *));

	memset(file_descriptors, 0, n_init_files * sizeof(int));

	// We use this complicated goback number thing when a file fails to open, or
	// stat, or mmap. We essentially skip that file.
	size_t fd_goback_number = 0;

	for (uint32_t i = 0; i < n_init_files; ++i) {
		uint32_t fd_index = i - fd_goback_number;
		// argv[i + 1]! Take note!
		file_descriptors[fd_index] = open(argv[i + 1], O_RDONLY);

		// Open the file
		if (file_descriptors[fd_index] == -1) {
			fprintf(stderr, "fail to open: %s\n", argv[i + 1]);

			--n_files;
			++fd_goback_number;
			continue;
		}

		// Get the file size
		if (fstat(file_descriptors[fd_index], &fs_stats[fd_index]) == -1) {
			fprintf(stderr, "fail to stat: %s\n", argv[i + 1]);

			--n_files;
			++fd_goback_number;
			continue;
		}

		// Get the number of pages for this file.
		fd_n_pages[fd_index] =
			(uint32_t)(fs_stats[fd_index].st_size / page_size);

		// according to union buster (Boskov et al.): pages using mmap with read
		// and execute permissions (such pages have a higher priority in the
		// page cache, and thus are more likely to stay there);
		mappings[fd_index] =
			mmap(NULL, fs_stats[fd_index].st_size, PROT_READ | PROT_EXEC,
				 MAP_PRIVATE, file_descriptors[fd_index], 0);

		if (mappings[fd_index] == MAP_FAILED) {
			fprintf(stderr, "mmap failed\n");
			--n_files; // yeah yeah, i know i'm overwriting stuff.
			++fd_goback_number;
			continue;
		}
	}

	fprintf(stdout, "\nNumber of files opened: %d\n", n_files);
	////////////////////////////////////////////////////////////////////////////
	volatile char bufferfly =
		0x12; // prevent the compiler from optimizing the reads

	while (1) {
		for (uint32_t fd_index = 0; fd_index < n_files; ++fd_index) {
			// Each file is marked with a MADV_WILLNEED
			if (madvise(mappings[fd_index], fs_stats[fd_index].st_size,
						MADV_WILLNEED) != 0) {
				perror("madvise");
			}

			// Furthermore, we read a page in each file. Readahead mechanism
			// makes this pretty fast.
			for (uint32_t page_num = 0; page_num < fd_n_pages[fd_index];
				 ++page_num) {
				bufferfly = mappings[fd_index][page_num * page_size];

				bufferfly ^= bufferfly;

				sched_yield(); // keep CPU usage on the down low
			}

			usleep(USLEEP_TIME);
		}
	}
	////////////////////////////////////////////////////////////////////////////

	// Doesn't really matter because the prev loop is infinite
	for (uint32_t fd_index = 0; fd_index < n_files; ++fd_index) {
		if (file_descriptors[fd_index] != 0) {
			close(file_descriptors[fd_index]);
		}
	}

	free(file_descriptors);
	free(fd_n_pages);

end:
	return return_value;
}