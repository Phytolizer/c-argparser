#pragma once

#include "sum.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	const char* ptr;
	uint64_t info;
} ArgStr;

#define ARG_STR_FMT "%.*s"
#define ARG_STR_ARG(s) (int)arg_str_len(s), (char*)(s).ptr

#define arg_str_ref_info_(x) ((x) << 1U)
#define arg_str_owner_info_(x) (arg_str_ref_info_(x) | UINT64_C(1))

#define arg_str_empty ((ArgStr){.ptr = NULL, .info = arg_str_ref_info_(0)})
#define arg_str_lit(s) ((ArgStr){.ptr = (s), .info = arg_str_ref_info_(sizeof(s) - 1)})

static inline uint64_t arg_str_len(ArgStr s)
{
	return s.info >> 1U;
}

static inline const char* arg_str_ptr(ArgStr s)
{
	if (s.ptr != NULL) {
		return s.ptr;
	}
	return "";
}

static inline const char* arg_str_end(ArgStr s)
{
	return arg_str_ptr(s) + arg_str_len(s);
}

static inline bool arg_str_is_empty(ArgStr s)
{
	return arg_str_len(s) == 0;
}

static inline bool arg_str_is_owner(ArgStr s)
{
	return (s.info & 1U) != 0;
}

static inline bool arg_str_is_ref(ArgStr s)
{
	return !arg_str_is_owner(s);
}

void arg_str_free(ArgStr s);

#define arg_str_ref(s) \
	_Generic( \
	          (s), \
	          ArgStr : arg_str_ref_str_, \
	          const char* : arg_str_ref_chars_, char* : arg_str_ref_chars_ \
	        )(s)

ArgStr arg_str_ref_str_(ArgStr s);
ArgStr arg_str_ref_chars_(const char* ptr);
ArgStr arg_str_acquire(const char* ptr, uint64_t len);
ArgStr arg_str_ref_chars(const char* ptr, uint64_t len);
ArgStr arg_str_copy(ArgStr s);

static inline ArgStr arg_str_shifted(ArgStr s, uint64_t n)
{
	if (n > arg_str_len(s)) {
		return arg_str_empty;
	}
	return arg_str_ref_chars(s.ptr + n, arg_str_len(s) - n);
}

typedef ARG_MAYBE(uint64_t) StrFindResult;

StrFindResult arg_str_find(ArgStr s, char c);

ArgStr arg_str_fmt(const char* fmt, ...);
ArgStr arg_str_fmt_va(const char* fmt, va_list args);

bool arg_str_eq(ArgStr a, ArgStr b);

ArgStr arg_str_join(ArgStr sep, uint64_t n, const ArgStr* strs);
