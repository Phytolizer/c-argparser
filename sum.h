#pragma once

#include <stdbool.h>

#define MAYBE(T) \
	struct { \
		bool present; \
		T value; \
	}

#define JUST(v) \
	{ .present = true, .value = v, }

#define NOTHING \
	{ .present = false, }

#define RESULT(T, E) \
	struct { \
		bool ok; \
		union { \
			T value; \
			E error; \
		} get; \
	}

#define OK(v) \
	{ .ok = true, .get.value = v, }

#define ERR(e) \
	{ .ok = false, .get.error = e, }
