#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void arg_str_free(arg_str s)
{
	if (arg_str_is_owner(s)) {
		free((char*)s.ptr);
	}
}

arg_str arg_str_ref_str_(arg_str s)
{
	return (arg_str) {
		.ptr = s.ptr, .info = arg_str_ref_info_(arg_str_len(s))
	};
}

arg_str arg_str_ref_chars_(const char* ptr)
{
	return (arg_str) {
		.ptr = ptr, .info = arg_str_ref_info_(strlen(ptr))
	};
}

arg_str arg_str_acquire(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return arg_str_empty;
	}

	if (len == 0) {
		free((char*)ptr);
		return arg_str_empty;
	}

	return (arg_str) {
		.ptr = ptr, .info = arg_str_owner_info_(len)
	};
}

arg_str arg_str_ref_chars(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return arg_str_empty;
	}

	if (len == 0) {
		return arg_str_empty;
	}

	return (arg_str) {
		.ptr = ptr, .info = arg_str_ref_info_(len)
	};
}

arg_str arg_str_copy(arg_str s)
{
	if (arg_str_is_empty(s)) {
		return arg_str_empty;
	}

	char* ptr = malloc(arg_str_len(s) + 1);
	if (ptr == NULL) {
		return arg_str_empty;
	}

	memcpy(ptr, arg_str_ptr(s), arg_str_len(s));
	ptr[arg_str_len(s)] = '\0';

	return (arg_str) {
		.ptr = ptr, .info = arg_str_owner_info_(arg_str_len(s))
	};
}

StrFindResult arg_str_find(arg_str s, char c)
{
	for (size_t i = 0; i < arg_str_len(s); i++) {
		if (s.ptr[i] == c) {
			return (StrFindResult)ARG_JUST(i);
		}
	}
	return (StrFindResult)ARG_NOTHING;
}

arg_str arg_str_fmt(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	arg_str s = arg_str_fmt_va(fmt, args);
	va_end(args);
	return s;
}

arg_str arg_str_fmt_va(const char* fmt, va_list args)
{
	va_list args_copy;
	va_copy(args_copy, args);

	int len = vsnprintf(NULL, 0, fmt, args_copy);
	if (len < 0) {
		return arg_str_empty;
	}

	char* ptr = malloc((size_t)len + 1);
	if (ptr == NULL) {
		return arg_str_empty;
	}

	(void)vsnprintf(ptr, (size_t)len + 1, fmt, args);

	return (arg_str) {
		.ptr = ptr,
		.info = arg_str_owner_info_(len),
	};
}

bool arg_str_eq(arg_str a, arg_str b)
{
	return arg_str_len(a) == arg_str_len(b)
	       && memcmp(a.ptr, b.ptr, arg_str_len(a)) == 0;
}

arg_str arg_str_join(arg_str sep, uint64_t n, const arg_str* strs)
{
	if (n == 0) {
		return arg_str_empty;
	}

	uint64_t totalLen = (n - 1) * arg_str_len(sep);
	for (uint64_t i = 0; i < n; i++) {
		totalLen += arg_str_len(strs[i]);
	}

	char* ptr = malloc(totalLen + 1);
	if (ptr == NULL) {
		return arg_str_empty;
	}

	char* dest = ptr;
	for (uint64_t i = 0; i < n; i++) {
		arg_str s = strs[i];
		memcpy(dest, s.ptr, arg_str_len(s));
		dest += arg_str_len(s);
		if (i < n - 1) {
			memcpy(dest, sep.ptr, arg_str_len(sep));
			dest += arg_str_len(sep);
		}
	}
	*dest = '\0';

	return (arg_str) {
		.ptr = ptr,
		.info = arg_str_owner_info_(totalLen),
	};
}
