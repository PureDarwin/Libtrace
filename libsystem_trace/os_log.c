#include <xpc/xpc.h>
#include "os_log_s.h"
#include "libtrace_assert.h"

extern const void *_os_log_class(void);
os_log_t os_log_create(const char *subsystem, const char *category) {
	os_log_t value = (os_log_t)_os_object_alloc(_os_log_class(), sizeof(os_log_t));
	value->magic = OS_LOG_MAGIC;
	value->subsystem = strdup(subsystem);
	value->category = strdup(category);
	return value;
}

bool os_log_type_enabled(os_log_t log, os_log_type_t type) {
	if (log == NULL) return false;
	if (log->magic == OS_LOG_DISABLED_MAGIC) return false;

	libtrace_precondition(log->magic == OS_LOG_DEFAULT_MAGIC || log->magic == OS_LOG_MAGIC, "Invalid os_log_t pointer parameter passed to os_log_type_enabled()");
	libtrace_precondition(type >= OS_LOG_TYPE_DEFAULT && type <= OS_LOG_TYPE_FAULT, "Invalid os_log_type_t parameter passed to os_log_type_enabled()");

	int bit = 1 << type;
	if ((log->enabled_mask & bit) == bit)
		return (log->enabled_values & bit) != 0;

	xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_string(message, "MessageId", "IsTypeEnabled");
	xpc_dictionary_set_string(message, "Subsystem", log->subsystem ?: "");
	xpc_dictionary_set_string(message, "Category", log->category ?: "");
	xpc_dictionary_set_int64(message, "LogType", type);

	dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
	__block bool enabled = false;

	xpc_connection_t connection = xpc_connection_create("com.apple.log.events", NULL);
	xpc_connection_send_message_with_reply(connection, message, dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^(xpc_object_t  _Nonnull object) {
		enabled = xpc_dictionary_get_bool(object, "IsEnabled");
		dispatch_semaphore_signal(semaphore);
	});

	dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
	dispatch_release(semaphore);
	xpc_release(message);

	log->enabled_values |= ((enabled == true) << type);
	log->enabled_mask |= bit;

	return enabled;
}

#pragma mark Legacy Functions

os_log_t _os_log_create(void *dso __unused, const char *subsystem, const char *category) {
	return os_log_create(subsystem, category);
}

bool os_log_is_enabled(os_log_t log) {
	return true;
}

bool os_log_is_debug_enabled(os_log_t log) {
	return os_log_debug_enabled(log);
}

#pragma mark Stub Functions

void
_os_log_internal(void *dso, os_log_t log, os_log_type_t type, const char *message, ...)
{
	_libtrace_assert_fail("_os_log_internal: Function unimplemented");
}
