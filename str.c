#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_free(str s)
{
	if (str_is_owner(s)) {
		free((char*)s.ptr);
	}
}

str z_str_ref_str(str s)
{
	return (str) {
		.ptr = s.ptr, .info = z_str_ref_info(str_len(s))
	};
}

str z_str_ref_chars(const char* ptr)
{
	return (str) {
		.ptr = ptr, .info = z_str_ref_info(strlen(ptr))
	};
}

str str_acquire(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return str_empty;
	}

	if (len == 0) {
		free((char*)ptr);
		return str_empty;
	}

	return (str) {
		.ptr = ptr, .info = z_str_owner_info(len)
	};
}

str str_ref_chars(const char* ptr, uint64_t len)
{
	if (ptr == NULL) {
		return str_empty;
	}

	if (len == 0) {
		return str_empty;
	}

	return (str) {
		.ptr = ptr, .info = z_str_ref_info(len)
	};
}

str str_copy(str s)
{
	if (str_is_empty(s)) {
		return str_empty;
	}

	char* ptr = malloc(str_len(s) + 1);
	if (ptr == NULL) {
		return str_empty;
	}

	memcpy(ptr, str_ptr(s), str_len(s));
	ptr[str_len(s)] = '\0';

	return (str) {
		.ptr = ptr, .info = z_str_owner_info(str_len(s))
	};
}

StrFindResult str_find(str s, char c)
{
	for (size_t i = 0; i < str_len(s); i++) {
		if (s.ptr[i] == c) {
			return (StrFindResult)JUST(i);
		}
	}
	return (StrFindResult)NOTHING;
}

str str_fmt(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	str s = str_fmt_va(fmt, args);
	va_end(args);
	return s;
}

str str_fmt_va(const char* fmt, va_list args)
{
	va_list args_copy;
	va_copy(args_copy, args);

	int len = vsnprintf(NULL, 0, fmt, args_copy);
	if (len < 0) {
		return str_empty;
	}

	char* ptr = malloc((size_t)len + 1);
	if (ptr == NULL) {
		return str_empty;
	}

	(void)vsnprintf(ptr, (size_t)len + 1, fmt, args);

	return (str) {
		.ptr = ptr,
		.info = z_str_owner_info(len),
	};
}

bool str_eq(str a, str b)
{
	return str_len(a) == str_len(b)
	       && memcmp(a.ptr, b.ptr, str_len(a)) == 0;
}

str str_join(str sep, uint64_t n, const str* strs)
{
	if (n == 0) {
		return str_empty;
	}

	uint64_t totalLen = (n - 1) * str_len(sep);
	for (uint64_t i = 0; i < n; i++) {
		totalLen += str_len(strs[i]);
	}

	char* ptr = malloc(totalLen + 1);
	if (ptr == NULL) {
		return str_empty;
	}

	char* dest = ptr;
	for (uint64_t i = 0; i < n; i++) {
		str s = strs[i];
		memcpy(dest, s.ptr, str_len(s));
		dest += str_len(s);
		if (i < n - 1) {
			memcpy(dest, sep.ptr, str_len(sep));
			dest += str_len(sep);
		}
	}
	*dest = '\0';

	return (str) {
		.ptr = ptr,
		.info = z_str_owner_info(totalLen),
	};
}
