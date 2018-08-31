#include <os/object_private.h>
#include <os/log.h>
#include "os_log_s.h"

extern const void *_os_log_class(void);
os_log_t os_log_create(const char *subsystem, const char *category) {
	os_log_t value = (os_log_t)_os_object_alloc(_os_log_class(), sizeof(os_log_t));
	value->magic = OS_LOG_MAGIC;
	value->subsystem = strdup(subsystem);
	value->category = strdup(category);
	return value;
}
