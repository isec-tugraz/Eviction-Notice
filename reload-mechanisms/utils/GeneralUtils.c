#include <stdio.h>
#include <time.h>

#include "GeneralUtils.h"

void print_timespec_diff(struct timespec a, struct timespec b)
{
	long seconds = b.tv_sec - a.tv_sec;
	long nanoseconds = b.tv_nsec - a.tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1e9;
	}

	fprintf(stdout, "Time difference: %ld.%09ld seconds\n", seconds,
			nanoseconds);
}