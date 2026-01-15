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
#include <time.h>
#include <unistd.h>

#include "CovertChannel.h"
#include "PageUtils.h"

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

	//////////////////////// Step 2: Generating Message ////////////////////////
	uint64_t n_comms_file_pages = comms_filesize / page_size;
	uint8_t *message = (uint8_t *)malloc(n_comms_file_pages * sizeof(uint8_t));

	gen_message(message, n_comms_file_pages);

	// We store the message to a file, so that the receiver can validate
	// transmission speed, error rate...
	FILE *msg_file = fopen(message_file, "w");
	if (msg_file == NULL) {
		perror("Failed to open message_file");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < n_comms_file_pages; ++i) {
		fprintf(msg_file, "%u", message[i]);
	}

	fclose(msg_file);

	///////////////////////// Step 3: Transmitting Data ////////////////////////
	char ch;
	volatile u_char bufferfly = 0x12;

	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

	// 1. Read the whole file so that we can flush the pages
	for (uint64_t i = 0; i < n_comms_file_pages; i += 1) {
		lseek(comms_fd, i * page_size, SEEK_SET);
		if (read(comms_fd, &ch, sizeof(ch)) != 1) {
			perror("read comms_fd");
			exit(EXIT_FAILURE);
		}

		// otherwise the compiler optimizes the read out sometimes
		bufferfly ^= ch;
	}

	// 2. Transmit
	for (uint64_t page_num = 0; page_num < n_comms_file_pages; ++page_num) {
		// if 0, flush; if 1, skip
		if (message[page_num] == 0) {
			posix_fadvise(comms_fd, page_num * page_size, page_size,
						  POSIX_FADV_DONTNEED);
		}
	}

	//////////////////////////// Step 4: Signalling ////////////////////////////
	// We signal through the sync file that we're done sending
	for (uint64_t pagenum = 0; pagenum < (sync_filesize / page_size);
		 ++pagenum) {

		lseek(sync_fd, pagenum * page_size, SEEK_SET);
		if (read(sync_fd, &ch, sizeof(ch)) != sizeof(ch)) {
			perror("sender mark complete: read failed");
			exit(EXIT_FAILURE);
		}

		// otherwise the compiler optimizes the read out sometimes
		bufferfly ^= ch;
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

	struct timespec res = timespec_difference(start_time, end_time);

	fprintf(stdout, "Transmission Time: %ld.%09ld seconds\n", res.tv_sec,
			res.tv_nsec);

	close(sync_fd);
	close(comms_fd);
	free(message);

	return EXIT_SUCCESS;

	// otherwise the compiler will optimize all the reads
	fprintf(stderr, "bufferfly: %d\n", bufferfly);
}