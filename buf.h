#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define BUF(T) \
	struct { \
		T* ptr; \
		uint64_t len; \
		uint64_t cap; \
		bool ref; \
	}

#define BUF_EMPTY \
	{ .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define BUF_REF(p, sz) \
	{ .ptr = (p), .len = (sz), .cap = (sz), .ref = true }

#define BUF_ARRAY(a) BUF_REF((a), sizeof(a) / sizeof((a)[0]))

#define BUF_OWNER(p, sz) \
	{ .ptr = (p), .len = (sz), .cap = (sz), .ref = false }

#define BUF_NEW \
	{ .ptr = NULL, .len = 0, .cap = 0, .ref = false }

#define BUF_FREE(buf) \
	do { \
		if (!(buf).ref) { \
			free((buf).ptr); \
		} \
	} while (false)

#define BUF_PUSH(buf, val) \
	do { \
		if ((buf)->len == (buf)->cap) { \
			(buf)->cap = (buf)->cap ? (buf)->cap * 2 : 1; \
			(buf)->ptr = realloc((buf)->ptr, (buf)->cap * sizeof(*(buf)->ptr)); \
		} \
		(buf)->ptr[(buf)->len++] = (val); \
	} while (false)

#define BUF_FIND(buf, val, eq, target) \
	do { \
		for (uint64_t i = 0; i < (buf).len; i++) { \
			if (eq((buf).ptr[i], (val))) { \
				*(target) = &(buf).ptr[i]; \
				break; \
			} \
		} \
	} while (false)
