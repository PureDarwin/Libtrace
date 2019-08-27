#include <xpc/xpc.h>
#include "os_log_s.h"
#include "libtrace_assert.h"
#include <asl.h>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern const void *_os_log_class(void);
extern struct os_log_s _os_log_default, _os_log_disabled;

void _libtrace_init(void) {
	_os_log_default.isa = _os_log_class();
	_os_log_disabled.isa = _os_log_class();
}

os_log_t os_log_create(const char *subsystem, const char *category) {
	libtrace_precondition(subsystem != NULL, "subsystem cannot be NULL");
	libtrace_precondition(category != NULL, "category cannot be NULL");

	os_log_t value = (os_log_t)_os_object_alloc(_os_log_class(), sizeof(os_log_t));
	value->magic = OS_LOG_MAGIC;
	value->subsystem = strdup(subsystem);
	value->category = strdup(category);
	return value;
}

bool os_log_type_enabled(os_log_t log, os_log_type_t type) {
	if (log == NULL) return false;
	if (log->magic == OS_LOG_DISABLED_MAGIC) return false;

	return true;
}

__XNU_PRIVATE_EXTERN
char *os_log_decode_buffer(const char *formatString, uint8_t *buffer, uint32_t bufferSize);

void
_os_log_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size) {
	libtrace_precondition(log != NULL, "os_log_t cannot be NULL");
	if (log->magic == OS_LOG_DISABLED_MAGIC) return;
	libtrace_precondition(log->magic == OS_LOG_DEFAULT_MAGIC || log->magic == OS_LOG_MAGIC, "Invalid os_log_t pointer parameter passed to _os_log_impl()");
	libtrace_precondition(type >= OS_LOG_TYPE_DEFAULT && type <= OS_LOG_TYPE_FAULT, "Invalid os_log_type_t parameter passed to _os_log_impl()");

	char *decodedMessage = os_log_decode_buffer(format, buf, size);

	aslmsg message = asl_new(ASL_TYPE_MSG);
	asl_set(message, "os_log(3)", "TRUE");
	asl_set(message, ASL_KEY_MSG, decodedMessage);
	asl_set(message, "Subsystem", log->subsystem);
	asl_set(message, "Category", log->category);

	free(decodedMessage);

	switch (type) {
		case OS_LOG_TYPE_DEBUG:
			asl_set(message, ASL_KEY_LEVEL, ASL_STRING_DEBUG);
			break;

		case OS_LOG_TYPE_INFO:
		case OS_LOG_TYPE_DEFAULT:
			asl_set(message, ASL_KEY_LEVEL, ASL_STRING_INFO);
			break;

		case OS_LOG_TYPE_ERROR:
			asl_set(message, ASL_KEY_LEVEL, ASL_STRING_ERR);
			break;

		case OS_LOG_TYPE_FAULT:
			asl_set(message, ASL_KEY_LEVEL, ASL_STRING_ALERT);
			break;

		default:
			libtrace_assert(false, "Invalid os_log_type_t not caught by precondition");
	}

	asl_send(NULL, message);
	asl_release(message);
}

#pragma mark Legacy Functions

os_log_t _os_log_create(void *dso __unused, const char *subsystem, const char *category) {
	return os_log_create(subsystem, category);
}

bool os_log_is_enabled(os_log_t log) {
	return true;
}

bool os_log_is_debug_enabled(os_log_t log) {
	return os_log_type_enabled(log, OS_LOG_TYPE_DEBUG);
}
