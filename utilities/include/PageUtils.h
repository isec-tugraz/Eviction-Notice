#ifndef PAGEUTILS_H
#define PAGEUTILS_H

#include <inttypes.h>

extern uint64_t page_size;

int8_t load_page_size() __attribute__((constructor));

#endif // PAGEUTILS_H