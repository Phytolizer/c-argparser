#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct {
	size_t length;
	size_t capacity;
} ArgGbufHeader;

#define ARG_GBUF_HEADER(buf) ((ArgGbufHeader*)(buf) - 1)

#define ARG_GBUF_FREE(buf) \
	do { \
		if (buf) { \
			free(ARG_GBUF_HEADER(buf)); \
		} \
	} while (false)

#define ARG_GBUF_LEN(buf) ((buf) ? ARG_GBUF_HEADER(buf)->length : 0)

#define ARG_GBUF_MAYBE_GROW(buf, added_count) \
	do { \
		if ((buf) == NULL || ARG_GBUF_HEADER(buf)->capacity < ARG_GBUF_HEADER(buf)->length + (added_count)) { \
			ArgGbufHeader* arg_gbuf_header = (buf) == NULL ? NULL : ARG_GBUF_HEADER(buf); \
			size_t arg_gbuf_new_cap = (arg_gbuf_header != NULL && arg_gbuf_header->capacity > 0) ? arg_gbuf_header->capacity * 2 : 1; \
			arg_gbuf_header = realloc(arg_gbuf_header, arg_gbuf_new_cap * sizeof(*(buf)) + sizeof(ArgGbufHeader)); \
			if ((buf) == NULL) arg_gbuf_header->length = 0; \
			arg_gbuf_header->capacity = arg_gbuf_new_cap; \
			(buf) = (void*)(arg_gbuf_header + 1); \
		} \
	} while (false)

#define ARG_GBUF_PUSH(buf, val) \
	do { \
		ARG_GBUF_MAYBE_GROW(buf, 1); \
		buf[ARG_GBUF_HEADER(buf)->length++] = (val); \
	} while (false)
