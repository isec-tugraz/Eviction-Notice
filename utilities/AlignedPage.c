#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "PageUtils.h"

uint64_t page_size = -1;

int8_t load_page_size()
{
	int64_t ps = sysconf(_SC_PAGE_SIZE);

	if (ps == -1) {
		perror("load_page_size failed");
		page_size = (uint64_t)-1;
		return -1;
	}

	page_size = (uint64_t)ps;

	return 0;
}

/* allocate number_pages of OS_PAGESIZE at array
 * use memset(array, 'A', number_pages * page_size);
 */
uint8_t *get_aligned_pages(uint32_t number_pages)
{
	if (page_size == (uint64_t)-1) {
		return (uint8_t *)-1;
	}

	void *ptr;

	if (posix_memalign(&ptr, (size_t)page_size,
					   (size_t)number_pages * page_size) != 0) {
		perror("posix_memalign failed");
		return (uint8_t *)-1;
	}

	uint8_t *array = (uint8_t *)ptr;

	return array;
}