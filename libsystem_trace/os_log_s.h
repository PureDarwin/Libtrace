#pragma once

#include <os/availability.h>
#include <os/trace_base.h>
#include <os/object.h>
#include <os/object_private.h>
#include <os/log.h>

struct os_log_s {
	_OS_OBJECT_HEADER(const void *isa, ref_cnt, xref_cnt);
	uint32_t magic;
	const char *subsystem;
	const char *category;
};

#define OS_LOG_MAGIC			0x584F0001
#define OS_LOG_DEFAULT_MAGIC	0x584F0002
#define OS_LOG_DISABLED_MAGIC	0x584FFFFF
