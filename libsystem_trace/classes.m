/*
 * Copyright (c) 2019 PureDarwin Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
	.magic = OS_LOG_DISABLED_MAGIC,
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
