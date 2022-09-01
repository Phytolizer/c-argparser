#pragma once

#include "sum.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	const char* ptr;
	uint64_t info;
} arg_str;

#define ARG_STR_FMT "%.*s"
#define ARG_STR_ARG(s) (int)arg_str_len(s), (char*)(s).ptr

#define arg_str_ref_info_(x) ((x) << 1U)
#define arg_str_owner_info_(x) (arg_str_ref_info_(x) | UINT64_C(1))

#define arg_str_empty ((arg_str){NULL, arg_str_ref_info_(0)})
#define arg_str_lit(s) ((arg_str){.ptr = (s), .info = arg_str_ref_info_(sizeof(s) - 1)})

static inline uint64_t arg_str_len(arg_str s)
{
	return s.info >> 1U;
}

static inline const char* arg_str_ptr(arg_str s)
{
	if (s.ptr != NULL) {
		return s.ptr;
	}
	return "";
}

static inline const char* arg_str_end(arg_str s)
{
	return arg_str_ptr(s) + arg_str_len(s);
}

static inline bool arg_str_is_empty(arg_str s)
{
	return arg_str_len(s) == 0;
}

static inline bool arg_str_is_owner(arg_str s)
{
	return (s.info & 1U) != 0;
}

static inline bool arg_str_is_ref(arg_str s)
{
	return !arg_str_is_owner(s);
}

void arg_str_free(arg_str s);

#define arg_str_ref(s) \
	_Generic( \
	          (s), \
	          arg_str : arg_str_ref_str_, \
	          const char* : arg_str_ref_chars_, char* : arg_str_ref_chars_ \
	        )(s)

arg_str arg_str_ref_str_(arg_str s);
arg_str arg_str_ref_chars_(const char* ptr);
arg_str arg_str_acquire(const char* ptr, uint64_t len);
arg_str arg_str_ref_chars(const char* ptr, uint64_t len);
arg_str arg_str_copy(arg_str s);

static inline arg_str arg_str_shifted(arg_str s, uint64_t n)
{
	if (n > arg_str_len(s)) {
		return arg_str_empty;
	}
	return arg_str_ref_chars(s.ptr + n, arg_str_len(s) - n);
}

typedef ARG_MAYBE(uint64_t) StrFindResult;

StrFindResult arg_str_find(arg_str s, char c);

arg_str arg_str_fmt(const char* fmt, ...);
arg_str arg_str_fmt_va(const char* fmt, va_list args);

bool arg_str_eq(arg_str a, arg_str b);

arg_str arg_str_join(arg_str sep, uint64_t n, const arg_str* strs);
