/*-
 * Copyright (c) 2000 Poul-Henning Kamp and Dag-Erling Coïdan Smørgrav
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *      $FreeBSD: /repoman/r/ncvs/src/sys/sys/sbuf.h,v 1.14 2004/07/09 11:35:30 des Exp $
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libsbuf.h"

struct sbuf *sbuf_new_auto(void) {
	struct sbuf *sb = calloc(1, sizeof(*sb));
	sb->s_buf = malloc(1024);
	sb->s_size = 1024;
	sb->s_len = 0;
	return sb;
}

void sbuf_clear(struct sbuf *sb) {
	free(sb->s_buf);
	sb->s_buf = malloc(1024);
	sb->s_size = 1024;
	sb->s_len = 0;
}

int	sbuf_setpos(struct sbuf *sb, int pos) {
	// Unimplemented.
	return sb->s_len;
}

int	sbuf_bcat(struct sbuf *sb, const void *ptr, size_t len) {
	if ((sb->s_len + len) > sb->s_size) {
		sb->s_size *= 2;
		sb->s_buf = realloc(sb->s_buf, sb->s_size);
	}

	memcpy(sb->s_buf + sb->s_len, ptr, len);
	sb->s_len += len;
	return sb->s_len;
}

int sbuf_bcpy(struct sbuf *sb, const void *ptr, size_t len) {
	if (len > sb->s_size) {
		sb->s_size *= 2;
		sb->s_buf = realloc(sb->s_buf, sb->s_size);
	}

	memcpy(sb->s_buf, ptr, len);
	sb->s_len = (int)len;
	return sb->s_len;
}

int sbuf_cat(struct sbuf *sb, const char *str) {
	return sbuf_bcat(sb, str, (int)strlen(str) * sizeof(char));
}

int sbuf_cpy(struct sbuf *sb, const char *str) {
	return sbuf_bcpy(sb, str, (int)strlen(str) * sizeof(char));
}

int sbuf_vprintf(struct sbuf *sb, const char *fmt, va_list ap) {
	char *str; vasprintf(&str, fmt, ap);
	int ret = sbuf_cat(sb, str);
	free(str);
	return ret;
}

int sbuf_printf(struct sbuf *sb, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int ret = sbuf_vprintf(sb, fmt, ap);
	va_end(ap);
	return ret;
}

int sbuf_putc(struct sbuf *sb, int c) {
	return sbuf_bcat(sb, &c, sizeof(int));
}

int sbuf_trim(struct sbuf *sb) {
	// Don't know what this does.
	return 0;
}

int	sbuf_overflowed(struct sbuf *sb) {
	// Don't know what this does.
	return 0;
}

void sbuf_finish(struct sbuf *sb) {
	// Don't know what this does.
}

char *sbuf_data(struct sbuf *sb) {
	bzero(sb->s_buf + sb->s_len, sb->s_size - sb->s_len);
	return sb->s_buf;
}

int	sbuf_len(struct sbuf *sb) {
	return sb->s_len;
}

int sbuf_done(struct sbuf *sb) {
	// Don't know what this does.
	return 0;
}

void sbuf_delete(struct sbuf *sb) {
	free(sb->s_buf);
	free(sb);
}
