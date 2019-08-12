#ifndef _LOGD_FILE_FORMAT_H_
#define _LOGD_FILE_FORMAT_H_

#include <os/log.h>

// File format is as follows:
// struct logd_file_header at very beginning of file.
//
// After that, records are appended back-to-back against
// each other. Each record has the following format:
//
// struct logd_entry_header header;
// NUL_terminated_string subsystem;
// NUL_terminated_string category;
// NUL_terminated_string format;
// uint32_t buffer_length;
// uint8_t buffer[buffer_length];
// struct logd_entry_footer footer;

struct logd_file_header {
	uint16_t magic;
	uint16_t version;
	uint32_t pad0; // must be zeros
};
#define LOGD_FILE_HEADER_MAGIC    0x584F
#define LOGD_FILE_HEADER_VERSION  0x0100

struct logd_entry_header {
	os_log_type_t log_type;
};
struct logd_entry_footer {
	uint32_t pad0; // must be zeros
};

#endif
