#pragma once

#include <stdbool.h>

#define ARG_MAYBE(T) \
	struct { \
		bool present; \
		T value; \
	}

#define ARG_JUST(v) \
	{ .present = true, .value = v, }

#define ARG_NOTHING \
	{ .present = false, }

#define ARG_RESULT(T, E) \
	struct { \
		bool ok; \
		union { \
			T value; \
			E error; \
		} get; \
	}

#define ARG_OK(v) \
	{ .ok = true, .get.value = v, }

#define ARG_ERR(e) \
	{ .ok = false, .get.error = e, }
