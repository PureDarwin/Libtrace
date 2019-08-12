#import <objc/objc.h>
#include "os_log_s.h"

#if _OS_OBJECT_OBJC_ARC
#error This file cannot be compiled with ARC.
#endif

@interface OS_OBJECT_CLASS(os_log) : OS_OBJECT_CLASS(object)
@end

@implementation OS_OBJECT_CLASS(os_log)

+ (void)load {
	_os_log_disabled.isa = (const void *)[self class];
	_os_log_default.isa = (const void *)[self class];
}

- (void)_dispose {
	struct os_log_s *value = (struct os_log_s *)self;
	if (value->subsystem != NULL) free((void *)value->subsystem);
	if (value->category != NULL) free((void *)value->category);
}

@end

#pragma mark -

__XNU_PRIVATE_EXTERN const void *_os_log_class(void) {
	return (const void *)[OS_os_log class];
}

struct os_log_s _os_log_disabled = {
	.isa = NULL,
	.ref_cnt = 0xFFFFFFFF,
	.xref_cnt = 0xFFFFFFFF,
	.magic = OS_LOG_DEFAULT_MAGIC,
	.subsystem = "",
	.category = ""
};

struct os_log_s _os_log_default = {
	.isa = NULL,
	.ref_cnt = 0xFFFFFFFF,
	.xref_cnt = 0xFFFFFFFF,
	.magic = OS_LOG_DEFAULT_MAGIC,
	.subsystem = "",
	.category = ""
};
