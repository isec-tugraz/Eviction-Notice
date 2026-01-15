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
#include <unistd.h>

#include "CovertChannel.h"
#include "PageUtils.h"

// clang-format off
#ifndef FLUSH_THRESHOLD
	#define FLUSH_THRESHOLD 0.000000900
#endif
// clang-format on

int comms_fd, sync_fd;
struct stat comms_file_stat, sync_file_stat;
uint64_t comms_filesize, sync_filesize;

static void wait_for_sync(int sync_fd);

// Performs a Reload and checks the time for the reload.
inline static int in_cache(int fd, uint64_t offset);
inline static int in_cache_with_print(int fd, uint64_t offset, int print);

long timespec_difference_ns(const struct timespec *start,
							const struct timespec *end);

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	if (argc != 4) {
		fprintf(stderr, "Usage: %s comms_file sync_file message_file\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	////////////////////////// Step 1: Opening files ///////////////////////////
	// comms = the file over which the signal is sent
	// sync  = the file used to synchronize the start of transmission
	const char *comms_file = argv[1];
	const char *sync_file = argv[2];
	const char *message_file = argv[3];

	open_files(comms_file, sync_file);

	///////////////////////// Step 2: Setup Both Files /////////////////////////
	// Flush comms (Sender reads it all back in)
	if (posix_fadvise(comms_fd, 0, 0, POSIX_FADV_DONTNEED)) {
		perror("receiver: posix_fadvise failed");
		exit(EXIT_FAILURE);
	}

	// Flush sync (sender reads when finished)
	if (posix_fadvise(sync_fd, 0, 0, POSIX_FADV_DONTNEED)) {
		perror("receiver: posix_fadvise failed");
		exit(EXIT_FAILURE);
	}

	struct timespec start_time, end_time;

	uint64_t n_comms_file_pages = comms_filesize / page_size;

	uint8_t *data = (uint8_t *)malloc(n_comms_file_pages * sizeof(uint8_t));

	for (int i = 0; i < 32; ++i) {
		sched_yield();
	}

	///////////////////////// Step 3: Start Receiving //////////////////////////
	wait_for_sync(sync_fd); // blocking function
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

	// core loop, time the flush operation
	for (int64_t page_num = n_comms_file_pages - 1; page_num >= 0;
		 page_num -= 1) {
		data[page_num] = in_cache(comms_fd, page_num * page_size);
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

	//////////////////////////// Step 4: Validation ////////////////////////////
	uint64_t correct =
		receiver_validate(message_file, data, n_comms_file_pages);

	///////////////////////////// Step 5: Stats ////////////////////////////////
	print_stats(correct, n_comms_file_pages, start_time, end_time);

	free(data);
	close(sync_fd);
	close(comms_fd);

	return EXIT_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////
inline static int in_cache(int fd, uint64_t offset)
{
	return in_cache_with_print(fd, offset, 0);
}

// Performs a Flush and checks the time for a flush
inline static int in_cache_with_print(int fd, uint64_t offset, int print)
{
	struct timespec start_time, end_time;

	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	posix_fadvise(fd, offset, page_size, POSIX_FADV_DONTNEED);
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

	double time_diff =
		(double)timespec_difference_ns(&start_time, &end_time) / 1e9;

	if (print)
		fprintf(stdout, "flush time:%0.12f", time_diff);
	if (time_diff >= FLUSH_THRESHOLD) {
		if (print)
			fprintf(stdout, " [in]\n");
		return 1;
	}

	if (print)
		fprintf(stdout, " [out]\n");
	return 0;
}

// blocking function
static void wait_for_sync(int sync_fd)
{
	while (1) {
		uint64_t n_notices = 0;

		for (uint64_t i = 0; i < (sync_filesize / page_size); ++i) {
			if (in_cache_with_print(sync_fd, i * page_size, 1)) {
				n_notices += 1;
				fprintf(stdout, "==== inc: %lu\n", n_notices);
				sched_yield();
			}
		}

		if (n_notices == (sync_filesize / page_size))
			break;

		usleep(100000);
	}
}