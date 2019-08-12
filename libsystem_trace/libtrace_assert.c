#include "libtrace_assert.h"
#include <CrashReporterClient.h>
#include <sys/reason.h>
#include <stdio.h>
#include <stdarg.h>

void _libtrace_assert_fail(const char *message, ...) {
	char *msg;
	va_list ap;

	va_start(ap, message);
	vasprintf(&msg, message, ap);
	va_end(ap);

	CRSetCrashLogMessage(msg);
	abort_with_reason(OS_REASON_LIBSYSTEM, 1, msg, OS_REASON_FLAG_GENERATE_CRASH_REPORT);
}
