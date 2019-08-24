#include <stdio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <xpc/xpc.h>
#include <CrashReporterClient.h>

#include <memory>
#include <type_traits>

#include "logd_file_format.h"
#include "logd_common.h"

namespace /* anonymous */ {

template<typename T>
class malloc_deleter {
public:
	void operator() (T ptr) {
		free(ptr);
	}
};

using unique_malloc_ptr = std::unique_ptr<uint8_t, malloc_deleter<uint8_t *>>;

template<typename T = xpc_object_t>
class xpc_holder {
private:
	T value;

public:
	xpc_holder(T val) {
		value = val;
	}

	~xpc_holder() {
		if (value != nullptr) xpc_release(value);
	}

	operator T() {
		return value;
	}

	bool null() const {
		return value == nullptr;
	}
};

inline unique_malloc_ptr unique_calloc(size_t byteSize) {
	return unique_malloc_ptr((uint8_t *) calloc(byteSize, 1));
}

void mkdir_p(const char *wholePath) {
	char *workPath = strdup(wholePath);

	char *next = workPath + 1;
	while ((next = strchr(next, '/')) != NULL) {
		*next = 0;

		int res = mkdir(wholePath, 0755);
		if (res != 0 && errno != EEXIST) {
			_setcrashlogmessage("mkdir(\"%s\", 0755) failed: %s", wholePath, strerror(errno));
			__builtin_trap();
		}

		*next++ = '/';
	}

	free(workPath);
}

int current_log_fd = -1;

} // anonymous namespace

extern "C"
void logd_open_current_log(void) {
	mkdir_p("/var/db/logd_storage");
	current_log_fd = open("/var/db/logd_storage/current_log", O_CREAT | O_WRONLY | O_APPEND, 0644);
	if (current_log_fd == -1) {
		_setcrashlogmessage("logd fatal error: open(\"/var/db/logd_storage/current_log\") failed: %s", strerror(errno));
		__builtin_trap();
	}

	struct stat sbuf;
	if (fstat(current_log_fd, &sbuf) == -1) {
		_setcrashlogmessage("logd fatal error: stat(\"/var/db/logd_storage/current_log\") failed: %s", strerror(errno));
		__builtin_trap();
	}

	if (sbuf.st_size == 0) {
		struct logd_file_header file_header;
		file_header.magic = LOGD_FILE_HEADER_MAGIC;
		file_header.version = LOGD_FILE_HEADER_VERSION;
		file_header.pad0 = 0;
		write(current_log_fd, &file_header, sizeof(file_header));
	}
}

extern "C"
void logd_append_log_entry(os_log_type_t type, const char *subsystem, const char *category, const char *format, time_t timestamp, uint8_t *args, uint32_t argsSize) {
	if (current_log_fd == -1) logd_open_current_log();

	size_t bufferSize = sizeof(struct logd_entry_header);
	bufferSize += strlen(subsystem) + 1;
	bufferSize += strlen(category) + 1;
	bufferSize += strlen(format) + 1;
	bufferSize += sizeof(uint32_t);
	bufferSize += argsSize;
	bufferSize += sizeof(struct logd_entry_footer);

	unique_malloc_ptr buffer = unique_calloc(bufferSize);
	uint8_t *ptr = buffer.get();

	{
		struct logd_entry_header *header = (struct logd_entry_header *)ptr;
		header->log_type = type;
		header->timestamp = timestamp;
		ptr += sizeof(struct logd_entry_header);
	}

	{
		char *strPtr = (char *)ptr;
		strPtr = stpcpy(strPtr, subsystem) + 1;
		strPtr = stpcpy(strPtr, category) + 1;
		strPtr = stpcpy(strPtr, format) + 1;
		ptr = (uint8_t *)strPtr;
	}

	*(uint32_t *)ptr = argsSize; ptr += sizeof(uint32_t);
	if (argsSize > 0) {
		if (args == nullptr) {
			// Don't call logd_append_log_entry() here, as we don't want to infinitely recurse.
			_setcrashlogmessage("logd_append_log_entry(): NULL args buffer not valid when argsSize > 0");
			__builtin_trap();
		}

		memcpy(ptr, args, argsSize); ptr += argsSize;
	}

	{
		struct logd_entry_footer *footer = (struct logd_entry_footer *)ptr;
		footer->pad0 = 0;
	}

	write(current_log_fd, buffer.get(), bufferSize);
}

extern "C"
void logd_handle_submission(xpc_object_t submission) {
	const char *subsystem = xpc_dictionary_get_string(submission, "Subsystem");
	const char *category = xpc_dictionary_get_string(submission, "Category");
	const char *format = xpc_dictionary_get_string(submission, "Format");
	xpc_holder argsObject = xpc_dictionary_get_value(submission, "ArgumentBuffer");
	xpc_holder levelObject = xpc_dictionary_get_value(submission, "LogLevel");
	xpc_holder timestampObject = xpc_dictionary_get_value(submission, "Timestamp");

	if (subsystem == NULL || category == NULL || format == NULL || argsObject.null() || levelObject.null() || timestampObject.null()) {
		logd_append_log_entry(OS_LOG_TYPE_ERROR, "com.apple.logd", "ClientError",
							  "Subsystem, Category, Format, ArgumentBuffer, LogLevel, and Timestamp are all required submission keys",
							  time(NULL), nullptr, 0);
		return;
	}

	uint32_t argsSize = (uint32_t)xpc_data_get_length(argsObject);
	unique_malloc_ptr args = unique_calloc(argsSize);
	argsSize = (uint32_t)xpc_data_get_bytes(argsObject, args.get(), 0, argsSize);

	logd_append_log_entry((os_log_type_t) xpc_int64_get_value(levelObject), subsystem, category, format, xpc_int64_get_value(timestampObject), args.get(), argsSize);
}

extern "C" void __attribute__((__format__(__printf__,1,2)))
_setcrashlogmessage(const char *fmt, ...)
{
	char *mess = NULL;
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = vasprintf(&mess, fmt, ap);
	va_end(ap);
	if (res < 0)
		mess = (char *)fmt; /* the format string is better than nothing */
	CRSetCrashLogMessage(mess);
}
