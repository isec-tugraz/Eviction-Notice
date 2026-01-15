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
#ifndef READ_THRESHOLD
	#define READ_THRESHOLD 0.000030000
#endif
// clang-format on

int comms_fd, sync_fd;
struct stat comms_file_stat, sync_file_stat;
uint64_t comms_filesize, sync_filesize;

static void wait_for_sync(int sync_fd);

// Performs a Reload and checks the time for the reload.
inline static int in_cache(int fd, uint64_t offset);

// Performs a Reload+Flush and checks the time for the reload
inline static int in_cache_flush(int fd, uint64_t offset);

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

	// core loop, reload using backward reading
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
// Performs a Reload and checks the time for the reload.
inline static int in_cache(int fd, uint64_t offset)
{
	volatile char ch;
	struct timespec start_time, end_time;

	lseek(fd, offset, SEEK_SET);

	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
	if (read(fd, &ch, sizeof(ch)) != sizeof(ch)) {
		perror("in_cache failed");
		exit(-1);
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

	if (((double)timespec_difference_ns(&start_time, &end_time) / 1e9) <=
		READ_THRESHOLD)
		return 1;

	return 0;
}

// Performs a Reload+Flush and checks the time for the reload
inline static int in_cache_flush(int fd, uint64_t offset)
{
	int is_in_cache = in_cache(fd, offset);

	posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);

	return is_in_cache;
}

// blocking function
static void wait_for_sync(int sync_fd)
{
	while (1) {
		if (in_cache_flush(sync_fd, sync_filesize - page_size))
			break;

		fprintf(stdout, "not in cache\n");
		fflush(stdout);

		// So it looks like flushing takes some time. When you flush an address
		// and immediately read it, there's a chance that it's still in page
		// cache and we read it instantaneously. We need a chonky amount of
		// delay to ensure that the page is actually flushed.
		usleep(200000);
	}
}