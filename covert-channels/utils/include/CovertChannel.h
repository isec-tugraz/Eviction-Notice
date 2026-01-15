#ifndef COVERTCHANNEL_H
#define COVERTCHANNEL_H

#include <inttypes.h>
#include <time.h>

long timespec_difference_ns(const struct timespec *start,
							const struct timespec *end);

struct timespec timespec_difference(const struct timespec start,
									const struct timespec end);

void open_files(const char *comms_file, const char *sync_file);

uint64_t receiver_validate(const char *message_file, uint8_t *data,
						   uint64_t n_comms_file_pages);

void print_stats(uint64_t correct, uint64_t n_comms_file_pages,
				 struct timespec start_time, struct timespec end_time);

void gen_message(uint8_t *msg, size_t num_bits);

#endif // COVERTCHANNEL_H