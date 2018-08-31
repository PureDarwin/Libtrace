#pragma once

#include <stdint.h>
#include <os/base.h>

__XNU_PRIVATE_EXTERN OS_NOINLINE OS_NORETURN
void _libtrace_bug(const char *condition, const char *message, const char *file, int line);
#define libtrace_assert(cond, message) \
	((cond) ? (void)0 : _libtrace_bug(#cond, message, __FILE__, __LINE__) )

__XNU_PRIVATE_EXTERN OS_NOINLINE OS_NORETURN
void _libtrace_client_bug(const char *condition, const char *message, const char *file, int line);
#define libtrace_precondition(cond, message) \
	((cond) ? (void)0 : _libtrace_client_bug(#cond, message, __FILE__, __LINE__) )
