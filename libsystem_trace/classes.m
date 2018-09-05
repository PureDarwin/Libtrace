#import <objc/objc.h>
#import <os/availability.h>
#import <os/trace_base.h>
#import <os/object.h>
#import <os/object_private.h>
#import <os/log.h>
#include "os_log_s.h"

@interface OS_os_log : OS_object
@end

@implementation OS_os_log

+ (void)load {
	_os_log_disabled.isa = (__bridge const void *)[self class];
	_os_log_default.isa = (__bridge const void *)[self class];
}

- (void)_dispose {
	struct os_log_s *value = (__bridge struct os_log_s *)self;
	if (value->subsystem != NULL) free((void *)value->subsystem);
	if (value->category != NULL) free((void *)value->category);
}

@end

#pragma mark -

__XNU_PRIVATE_EXTERN const void *_os_log_class(void) {
	return (__bridge const void *)[OS_os_log class];
}

struct os_log_s _os_log_disabled = {
	.isa = NULL,
	.ref_cnt = 0xFFFFFFFF,
	.xref_cnt = 0xFFFFFFFF,
	.magic = OS_LOG_DEFAULT_MAGIC,
	.subsystem = NULL,
	.category = NULL
};

struct os_log_s _os_log_default = {
	.isa = NULL,
	.ref_cnt = 0xFFFFFFFF,
	.xref_cnt = 0xFFFFFFFF,
	.magic = OS_LOG_DEFAULT_MAGIC,
	.subsystem = NULL,
	.category = NULL
};
