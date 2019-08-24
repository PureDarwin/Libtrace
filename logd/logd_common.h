#ifndef _LOGD_COMMON_H_
#define _LOGD_COMMON_H_

#include <sys/cdefs.h>
#include <os/log.h>

__BEGIN_DECLS

extern void logd_append_log_entry(os_log_type_t type, const char *subsystem, const char *category, const char *format, time_t timestamp, uint8_t *args, uint32_t argsSize);
extern void _setcrashlogmessage(const char *fmt, ...) __attribute__((__format__(__printf__,1,2)));

__END_DECLS

#endif /* _LOGD_COMMON_H_ */
