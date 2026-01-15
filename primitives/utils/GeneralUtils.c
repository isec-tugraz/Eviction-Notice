#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "GeneralUtils.h"

int64_t auto_strtoll(const char *str)
{
	char *endptr;

	// Base 0 does automatic base detection POGCHAMP + HELL YEAH
	int64_t value = strtoll(str, &endptr, 0);

	if (errno == ERANGE) {
		perror("Value out of range");
		return 0;
	}
	if (errno == EINVAL) {
		fprintf(stderr, "No digits or weird base???\n");
		return 0;
	}

	return value;
}

int64_t *parse_page_list(int page_count, char *page_list[], bool *perf_all,
						 int n_file_pages)
{
	int64_t *pages = malloc(page_count * sizeof(int64_t));
	*perf_all = false;

	for (int i = 0; i < page_count; ++i) {
		int64_t page_val = auto_strtoll(page_list[i]);

		// negative numbers = perform action on the whole file
		if (page_val < 0) {
			*perf_all = true;
		}

		// anything greater than n_file_pages gets clipped
		if (page_val > n_file_pages - 1) {
			page_val = n_file_pages - 1;
		}

		pages[i] = page_val;
	}

	return pages;
}

int compare_uint64_t(const void *a, const void *b)
{
	uint64_t num1 = *(uint64_t *)a;
	uint64_t num2 = *(uint64_t *)b;

	if (num1 < num2)
		return -1;
	if (num1 > num2)
		return 1;
	return 0;
}