#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ARG_BUF(T) \
	struct { \
		T* ptr; \
		uint64_t len; \
		uint64_t cap; \
		bool ref; \
	}

#define ARG_BUF_EMPTY \
	{ .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define ARG_BUF_REF(p, sz) \
	{ .ptr = (p), .len = (sz), .cap = (sz), .ref = true }

#define ARG_BUF_ARRAY(a) BUF_REF((a), sizeof(a) / sizeof((a)[0]))

#define ARG_BUF_OWNER(p, sz) \
	{ .ptr = (p), .len = (sz), .cap = (sz), .ref = false }

#define ARG_BUF_NEW \
	{ .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define ARG_BUF_FREE(buf) \
	do { \
		if (!(buf).ref) { \
			free((buf).ptr); \
		} \
	} while (false)

#define ARG_BUF_PUSH(buf, val) \
	do { \
		if ((buf)->len == (buf)->cap) { \
			(buf)->cap = (buf)->cap ? (buf)->cap * 2 : 1; \
			(buf)->ptr = realloc((buf)->ptr, (buf)->cap * sizeof(*(buf)->ptr)); \
		} \
		(buf)->ptr[(buf)->len++] = (val); \
	} while (false)

#define ARG_BUF_FIND(buf, val, eq, target) \
	do { \
		for (uint64_t i = 0; i < (buf).len; i++) { \
			if (eq((buf).ptr[i], (val))) { \
				*(target) = &(buf).ptr[i]; \
				break; \
			} \
		} \
	} while (false)
