#include "libtrace_assert.h"
#include <stdio.h>

char *__crashreporter_info__;
asm(".desc __crashreporter_info__, 0x10");

void _libtrace_bug(const char *condition, const char *message, const char *file, int line) {
	asprintf(&__crashreporter_info__, "BUG IN LIBTRACE: %s (at %s:%d)", message, file, line);
	__builtin_trap();
}

void _libtrace_client_bug(const char *condition, const char *message, const char *file, int line) {
	asprintf(&__crashreporter_info__, "BUG IN CLIENT OF LIBTRACE: %s (at %s:%d)", message, file, line);
	__builtin_trap();
}
