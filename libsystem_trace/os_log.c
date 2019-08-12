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

	dispatch_queue_t targetq = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	xpc_connection_t connection = xpc_connection_create("com.apple.log", targetq);
	libtrace_assert(connection != NULL, "Could not create connection to com.apple.log Mach service");
	xpc_connection_resume(connection);

	xpc_object_t reply = xpc_connection_send_message_with_reply_sync(connection, message);
	xpc_release(message);

	bool enabled = xpc_dictionary_get_bool(reply, "IsEnabled");
	xpc_release(reply);

	log->enabled_values |= ((enabled == true) << type);
	log->enabled_mask |= bit;

	return enabled;
}

void
_os_log_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size) {
	libtrace_precondition(log != NULL, "os_log_t cannot be NULL");
	if (log->magic == OS_LOG_DISABLED_MAGIC) return;
	libtrace_precondition(log->magic == OS_LOG_DEFAULT_MAGIC || log->magic == OS_LOG_MAGIC, "Invalid os_log_t pointer parameter passed to os_log_type_enabled()");
	libtrace_precondition(type >= OS_LOG_TYPE_DEFAULT && type <= OS_LOG_TYPE_FAULT, "Invalid os_log_type_t parameter passed to os_log_type_enabled()");

	xpc_object_t message = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_string(message, "Subsystem", log->subsystem ?: "");
	xpc_dictionary_set_string(message, "Category", log->category ?: "");
	xpc_dictionary_set_int64(message, "LogType", type);
	xpc_dictionary_set_string(message, "Format", format);
	xpc_dictionary_set_data(message, "ArgumentBuffer", buf, size);

	dispatch_queue_t targetq = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
	xpc_connection_t connection = xpc_connection_create_mach_service("com.apple.log.events", targetq, 0);
	libtrace_assert(connection != NULL, "Could not create connection to com.apple.log.events Mach service");

	xpc_connection_resume(connection);
	(void)xpc_connection_send_message_with_reply_sync(connection, message);

	xpc_release(message);
	xpc_release(connection);
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
