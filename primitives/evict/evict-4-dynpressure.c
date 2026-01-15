/* Creates a thread to dynamically induce pressure, spawning 64MB chunks every
 * 2ms
 *
 * Args: ./ev4-dynthrottle
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "PageUtils.h"

#define MiB 1024 * 1024
#define THREAD_BYTES 64 * MiB
#define USLEEP_INTERVAL 2000
#define MAX_THREADS 1024

////////////////////////////////////////////////////////////////////////////////
// clang-format off
enum {
	ALL_OK = 0,
	MALLOC_FAILED = -1,
	MLOCK_FAILED = -2,
	ALL_NOT_OK = -3
};
// clang-format on

typedef struct {
	int thread_id;
	u_char *map;
	int return_stat; // not really used, but we could engineer it if we wanted
					 // to
} thread_data;

// if there's an error, this thread will free its map. Otherwise, if main kills
// it, then main has to free map. Don't forget it. Or else... I. will. haunt.
// you.
void *thread_loop(void *argument)
{
	int return_value;
	thread_data *t_data = (thread_data *)argument;

	t_data->map = (u_char *)malloc(THREAD_BYTES * sizeof(u_char));

	if (t_data->map == NULL) {
		return_value = MALLOC_FAILED;
		fprintf(stderr, "malloc fail\n");
		goto end;
	}

	t_data->map[0] = 0x1; // freaking compiler :/

	if (mlock(t_data->map, THREAD_BYTES) == -1) {
		return_value = MLOCK_FAILED;
		perror("mlock\n");
		goto end;
	}

	while (1) {
		sched_yield();
	}

end:
	free(t_data->map);
	fprintf(stderr, "error %d: %d!\n", t_data->thread_id, return_value);
	t_data->return_stat = return_value;

	return NULL;
}
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s\n", argv[0]);
		return EXIT_FAILURE;
	}

	//////////////////////////////////// POC ///////////////////////////////////
	// we're going to be making this like a stack
	pthread_t threads[MAX_THREADS];
	thread_data th_data[MAX_THREADS];

	uint32_t current_thread = 0;

	while (1) {
		fprintf(stdout, "[spwn]...");

		// nothing can be done. We want to keep the pressure up.
		if ((current_thread + 1) >= MAX_THREADS)
			goto end_of_forever;

		++current_thread;

		th_data[current_thread].return_stat = ALL_OK;
		th_data[current_thread].thread_id = current_thread;

		if (pthread_create(&threads[current_thread], NULL, thread_loop,
						   (void *)&th_data[current_thread]) != 0) {
			printf("pthread_create");
			exit(-1);
		}

	end_of_forever:
		usleep(USLEEP_INTERVAL);
	}

	return EXIT_SUCCESS;
}
