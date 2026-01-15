#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "CovertChannel.h"

extern int comms_fd, sync_fd;
extern struct stat comms_file_stat, sync_file_stat;
extern uint64_t comms_filesize, sync_filesize;

long timespec_difference_ns(const struct timespec *start,
							const struct timespec *end)
{
	long seconds = end->tv_sec - start->tv_sec;
	long nanoseconds = end->tv_nsec - start->tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1000000000;
	}

	long retval = seconds * 1000000000 + nanoseconds;

	return retval;
}

struct timespec timespec_difference(const struct timespec start,
									const struct timespec end)
{
	struct timespec result;

	long seconds = end.tv_sec - start.tv_sec;
	long nanoseconds = end.tv_nsec - start.tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1e9;
	}

	result.tv_sec = seconds;
	result.tv_nsec = nanoseconds;

	return result;
}

void open_files(const char *comms_file, const char *sync_file)
{
	comms_fd = open(comms_file, O_RDONLY);
	if (comms_fd == -1) {
		perror("failed open comms file");
		exit(EXIT_FAILURE);
	}

	if (fstat(comms_fd, &comms_file_stat) == -1) {
		perror("failed fstat comms file");
		exit(EXIT_FAILURE);
	}

	comms_filesize = comms_file_stat.st_size;

	sync_fd = open(sync_file, O_RDONLY);
	if (sync_fd == -1) {
		perror("failed open sync file");
		exit(EXIT_FAILURE);
	}

	if (fstat(sync_fd, &sync_file_stat) == -1) {
		perror("failed fstat sync file");
		exit(EXIT_FAILURE);
	}

	sync_filesize = sync_file_stat.st_size;
}

uint64_t receiver_validate(const char *message_file, uint8_t *data,
						   uint64_t n_comms_file_pages)
{
	uint64_t correct = 0;

	FILE *msg_file = fopen(message_file, "r");
	if (msg_file == NULL) {
		perror("Failed to open msg_file");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < n_comms_file_pages; i++) {
		int ch = fgetc(msg_file);
		if (ch == EOF) {
			break;
		}
		if ((ch == '0') && (data[i] == 0)) {
			correct += 1;
		} else if ((ch == '1') && (data[i] == 1)) {
			correct += 1;
		} else if (!((ch == '0') || (ch == '1'))) {
			fprintf(stderr, "Invalid character in file: %c\n", ch);
			exit(EXIT_FAILURE);
		}
	}

	return correct;
}

void print_stats(uint64_t correct, uint64_t n_comms_file_pages,
				 struct timespec start_time, struct timespec end_time)
{
	fprintf(stdout, "[receiver] Transmission Length: %lu\n",
			n_comms_file_pages);
	fprintf(stdout, "[receiver] Raw Correct: %lu\n", correct);

	double percCorrect = (double)correct / (double)n_comms_file_pages;
	fprintf(stdout, "[receiver] Percentage Correct: %f\n", percCorrect);

	struct timespec res = timespec_difference(start_time, end_time);

	fprintf(stdout, "[receiver] Transmission Time: %ld.%09ld seconds\n",
			res.tv_sec, res.tv_nsec);
}

// Generates a random message of num_bits size into msg; randomness is sourced
// from urandom
void gen_message(uint8_t *msg, size_t num_bits)
{
	int retval = 0;

	// 1. Get data from urandom
	int urandom_fd = open("/dev/urandom", O_RDONLY);
	if (urandom_fd < 0) {
		perror("Failed to open /dev/urandom");
		retval = -1;
		goto end;
	}

	size_t num_bytes = (num_bits + 7) / 8;
	uint8_t *rnd_buffer = malloc(num_bytes);
	if (rnd_buffer == NULL) {
		perror("Failed to allocate memory");
		retval = -1;
		goto closefd;
	}

	int64_t bytes_read = read(urandom_fd, rnd_buffer, num_bytes);
	if (bytes_read < 0) {
		perror("Failed to read from /dev/urandom");
		retval = -1;
		goto freebuf;
	}

	for (size_t i = 0; i < num_bits; i++) {
		msg[i] = (rnd_buffer[i / 8] >> (7 - (i % 8))) & 1;
	}

freebuf:
	free(rnd_buffer);

closefd:
	close(urandom_fd);

end:
	if (retval == -1)
		exit(EXIT_FAILURE);

	return;
}