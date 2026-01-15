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

// clang-format off
#ifndef USE_CACHESTAT
	#ifndef USE_PREADV2
		#define USE_PREADV2
	#endif
#endif
// clang-format on

#ifdef USE_CACHESTAT
#include "CacheStat.h"
#endif

#include "CovertChannel.h"
#include "PageUtils.h"

static void wait_for_sync(int sync_fd);
static uint8_t page_in_cache(int comms_fd, int64_t page_num);

int comms_fd, sync_fd;
struct stat comms_file_stat, sync_file_stat;
uint64_t comms_filesize, sync_filesize;

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

	// core loop
	for (uint64_t page_num = 0; page_num < n_comms_file_pages; ++page_num) {
		data[page_num] = page_in_cache(comms_fd, page_num);
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

// Uses monitor to wait for sync to be read [BLOCKING FUNCTION]
static void wait_for_sync(int sync_fd)
{
#ifdef USE_CACHESTAT
	struct cachestat cs = {0};
	struct cachestat_range cs_range = {.off = sync_filesize - 1, .len = 1};

	while (1) {
		syscall(SYSCALL_CACHESTAT, sync_fd, &cs_range, &cs, 0);

		if (cs.nr_cache >= 1) {
			break;
		}

		sched_yield();
	}

#elif USE_PREADV2
	char buffer;
	struct iovec iov[1];
	iov[0].iov_base = &buffer;
	iov[0].iov_len = 1;

	while (1) {
		lseek(sync_fd, -1, SEEK_END);

		ssize_t ret = preadv2(sync_fd, iov, 1, 0, RWF_NOWAIT);

		if (ret < 0) {
			if (errno == EAGAIN) {
				// Not in cache
				sched_yield();
				continue;
			} else {
				perror("preadv2 failed");
			}
		} else {
			// In cache
			break;
		}
	}
#endif

	return;
}

static uint8_t page_in_cache(int comms_fd, int64_t page_num)
{
	uint8_t in_cache = 2;

#ifdef USE_CACHESTAT
	struct cachestat cs = {0};
	struct cachestat_range cs_range = {.off = page_num * page_size,
									   .len = page_size};

	syscall(SYSCALL_CACHESTAT, comms_fd, &cs_range, &cs, 0);

	in_cache = cs.nr_cache;

#elif USE_PREADV2
	char buffer = 0;
	struct iovec iov[1];
	iov[0].iov_base = &buffer;
	iov[0].iov_len = 1;

	lseek(comms_fd, 0, SEEK_SET);
	ssize_t ret = preadv2(comms_fd, iov, 1, page_num * page_size, RWF_NOWAIT);

	if (ret < 0) {
		if (errno == EAGAIN) {
			in_cache = 0;
		} else {
			perror("preadv2 failed");
		}
	} else if (ret == 0) {
		in_cache = 2;
	} else {
		in_cache = 1;
	}
#endif

	return in_cache;
}