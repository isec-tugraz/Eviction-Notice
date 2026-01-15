#ifndef GENERALUTILS_H
#define GENERALUTILS_H

#include <inttypes.h>

int64_t auto_strtoll(const char *str);
int64_t *parse_page_list(int page_count, char *page_list[], bool *flush_all,
						 int n_file_pages);
int compare_uint64_t(const void *a, const void *b);

#endif // GENERALUTILS_H