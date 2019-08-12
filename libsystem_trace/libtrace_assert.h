#pragma once

#include <stdint.h>
#include <os/base.h>

__XNU_PRIVATE_EXTERN OS_NOINLINE OS_NORETURN
void _libtrace_assert_fail(const char *message, ...);

#define libtrace_assert(cond, message, ...) \
	do { \
		if (__builtin_expect(!(cond), 0)) \
			_libtrace_assert_fail("BUG IN LIBTRACE: " message, ##__VA_ARGS__); \
	} while (0)
#define libtrace_precondition(cond, message, ...) \
	do { \
		if (__builtin_expect(!(cond), 0)) \
			_libtrace_assert_fail("BUG IN CLIENT OF LIBTRACE: " message, ##__VA_ARGS__); \
	} while (0)
