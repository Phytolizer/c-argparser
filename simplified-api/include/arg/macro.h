#pragma once

#ifdef _WIN32
#include <stdbool.h>

#define ARG_UNREACHABLE() __assume(false)
#else
#define ARG_UNREACHABLE() __builtin_unreachable()
#endif
