/* Takes no arguments. Will begin reading every (readable) file on the system
 * starting from /; we skip /proc and /sys
 *
 * Run as: ./evict-2-randaccess
 */

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "PageUtils.h"

#define USLEEP_TIME 2000 // 2 milliseconds

#define MAX_PATH 4096
volatile u_char bufferfly;
uint64_t total_pages_acc = 0;
uint64_t total_bytes = 0;

void go_down_dir(const char *dir_path);

int main(int argc, char *argv[])
{
	srand(time(NULL));

	bufferfly = 0x24;
	const char *root = "/";

	// We read this only once, but can infinite while it. It keeps running for a
	// LONG LONG TIMEEEE... usually.
	go_down_dir(root);

	return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
void shuffle(uint64_t *array, size_t n)
{
	if (n == 0)
		return;

	for (size_t i = n - 1; i > 0; i--) {
		size_t j = rand() % (i + 1);
		uint64_t temp = array[i];
		array[i] = array[j];
		array[j] = temp;
	}
}

void go_down_dir(const char *dir_path)
{
	char path[MAX_PATH];

	struct dirent *entry;
	struct stat file_stat;

	DIR *dp = opendir(dir_path);

	if (dp == NULL) {
		perror("opendir");
		return;
	}

	fprintf(stdout,
			"\n====================================\n"
			"> [%s] Pages: %lu, Bytes: %lu\n",
			dir_path, total_pages_acc, total_bytes);

	while ((entry = readdir(dp)) != NULL) {
		// Skip the special entries "." and ".."
		if (strcmp(entry->d_name, ".") == 0 ||
			strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		// no proc's and sys'
		if ((strcmp(entry->d_name, "proc") == 0) ||
			(strcmp(entry->d_name, "sys") == 0)) {
			continue;
		}

		snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
		if (stat(path, &file_stat) == -1) {
			// perror("stat"); // too many<s symbolic links.
			continue;
		}

		if (S_ISDIR(file_stat.st_mode)) {
			// Recursively go down dir
			go_down_dir(path);

		} else {
			// We hit a file
			if ((file_stat.st_mode & S_IRUSR) &&
				(file_stat.st_mode & S_IXUSR)) {

				// ".so" will be taken care of evict-1-shlibs.c
				if (strstr(entry->d_name, ".so") == NULL) {
					fprintf(stdout, "\t%s\n", path);

					int fd = open(path, O_RDONLY);
					if (fd == -1) {
						perror("open");
						continue;
					}

					size_t file_size = file_stat.st_size;
					uint64_t n_pages = file_size / page_size;
					total_bytes += file_size;

					char *map = mmap(NULL, file_size, PROT_READ | PROT_EXEC,
									 MAP_PRIVATE, fd, 0);
					if (map == MAP_FAILED) {
						perror("mmap");
						close(fd);
						continue;
					}

					if (madvise(map, file_size, MADV_WILLNEED) != 0) {
						perror("madvise");
					}

					// To randomly access the pages
					uint64_t *page_indices = malloc(n_pages * sizeof(uint64_t));
					for (uint64_t i = 0; i < n_pages; ++i) {
						page_indices[i] = i;
					}

					// To randomly access the pages
					shuffle(page_indices, n_pages);

					for (uint64_t i = 0; i < n_pages; ++i) {
						uint64_t pagenum = page_indices[i];
						bufferfly ^= map[pagenum * page_size];

						sched_yield();
					}

					total_pages_acc += n_pages;

					free(page_indices);
					munmap(map, file_size);
					close(fd);

					usleep(USLEEP_TIME);
				}
			}
		}
	}

	closedir(dp);
}