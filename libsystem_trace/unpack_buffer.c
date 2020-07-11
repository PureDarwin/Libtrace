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

#include <ctype.h>
#include <errno.h>
#include "os_log_s.h"
#include "libtrace_assert.h"
#include "libsbuf.h"

static bool contains_attribute(char *allAttributes, char *desired) {
	if (strcmp(allAttributes, desired) == 0) return true;

	char *needle = strstr(allAttributes, desired);
	if (needle == NULL) return false;

	if (needle != allAttributes) {
		// The "if" check on the above line is to avoid underflowing the buffer,
		// in case the desired attribute is located at the very start of allAttributes.
		char *comma = needle - 1;
		if (*comma != ',') return false;
	}

	char *end = needle + strlen(desired);
	if (*end == '\0' || *end == ',') return true;

	return false;
}

typedef enum {
	privacy_setting_unset = 0,
	privacy_setting_private = 1,
	privacy_setting_public = 2
} privacy_setting_t;

__XNU_PRIVATE_EXTERN
char *os_log_decode_buffer(const char *formatString, uint8_t *buffer, uint32_t bufferSize) {
	uint32_t bufferIndex = 0;
	// uint8_t summaryByte = buffer[bufferIndex]; // not actually used, hence commented out
	bufferIndex++;

	uint8_t argsSeen = 0;
	uint8_t argCount = buffer[bufferIndex];
	bufferIndex++;

	struct sbuf *sbuf = sbuf_new_auto();
	while (formatString[bufferIndex++] != '\0') {
		if (formatString[bufferIndex] != '%') {
			sbuf_putc(sbuf, formatString[bufferIndex]);
			continue;
		}

		libtrace_assert(formatString[bufferIndex] == '%', "next char not % (this shouldn't happen)");
		libtrace_assert(argsSeen < argCount, "Too many format specifiers in os_log string (expected %d)", argCount);

		privacy_setting_t privacy = privacy_setting_unset;
		if (formatString[bufferIndex] == '{') {
			const char *closingBracket = strchr(formatString + bufferIndex, '}');
			size_t brlen = closingBracket - (formatString + bufferIndex);
			char *attribute = (char *)malloc(brlen + 1);

			strlcpy(attribute, formatString + bufferIndex, brlen + 1);
			bufferIndex += strlen(attribute) + 1;

			if (contains_attribute(attribute, "private")) {
				privacy = privacy_setting_private;
			} else if (contains_attribute(attribute, "public")) {
				privacy = privacy_setting_public;
			}

			while (formatString[bufferIndex] == '.' ||
				   formatString[bufferIndex] == '*' ||
				   isnumber(formatString[bufferIndex])) {
				bufferIndex++;
			}

			char *formattedArgument = NULL;
			if (formatString[bufferIndex] == 's') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;
				formattedArgument = calloc(length + 1, sizeof(char));
				memcpy(formattedArgument, buffer + bufferIndex, length);
				bufferIndex += length * sizeof(char);

				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[bufferIndex] == 'S') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				wchar_t *wideArgument = calloc(length + 1, sizeof(wchar_t));
				memcpy(wideArgument, buffer + bufferIndex, length * sizeof(wchar_t));
				bufferIndex += length * sizeof(wchar_t);

				formattedArgument = calloc(length + 1, sizeof(char));
				wcstombs(formattedArgument, wideArgument, length);
				free(wideArgument);

				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[bufferIndex] == 'P') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				struct sbuf *dataHex = sbuf_new_auto();
				sbuf_putc(dataHex, '<');

				for (uint8_t i = 0; i < length; i++) {
					sbuf_printf(dataHex, "%02X", buffer[bufferIndex]);
					bufferIndex++;
				}

				sbuf_putc(dataHex, '>');
				formattedArgument = strdup(sbuf_data(dataHex));
				sbuf_delete(dataHex);

				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[bufferIndex] == '@') {
				// FIXME: Correctly describe Objective-C objects
				formattedArgument = strdup("<ObjC object>");
				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[bufferIndex] == 'm') {
				bufferIndex++; // skip zero "length"
				formattedArgument = strdup(strerror(errno));
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else if (formatString[bufferIndex] == 'd') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				int64_t integer;
				if (length == 1) integer = *(int8_t *)buffer;
				else if (length == 2) integer = *(int16_t *)buffer;
				else if (length == 4) integer = *(int32_t *)buffer;
				else if (length == 8) integer = *(int64_t *)buffer;
				else libtrace_assert(false, "Unexpected integer size %d", length);

				bufferIndex += length;
				asprintf(&formattedArgument, "%lld", integer);
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else if (formatString[bufferIndex] == 'u') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				uint64_t integer;
				if (length == 1) integer = *(uint8_t *)buffer;
				else if (length == 2) integer = *(uint16_t *)buffer;
				else if (length == 4) integer = *(uint32_t *)buffer;
				else if (length == 8) integer = *(uint64_t *)buffer;
				else libtrace_assert(false, "Unexpected integer size %d", length);

				bufferIndex += length;
				asprintf(&formattedArgument, "%llu", integer);
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else if (formatString[bufferIndex] == 'x') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				uint64_t integer;
				if (length == 1) integer = *(uint8_t *)buffer;
				else if (length == 2) integer = *(uint16_t *)buffer;
				else if (length == 4) integer = *(uint32_t *)buffer;
				else if (length == 8) integer = *(uint64_t *)buffer;
				else libtrace_assert(false, "Unexpected integer size %d", length);

				bufferIndex += length;
				asprintf(&formattedArgument, "0x%llx", integer);
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else if (formatString[bufferIndex] == 'X') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				uint64_t integer;
				if (length == 1) integer = *(uint8_t *)buffer;
				else if (length == 2) integer = *(uint16_t *)buffer;
				else if (length == 4) integer = *(uint32_t *)buffer;
				else if (length == 8) integer = *(uint64_t *)buffer;
				else libtrace_assert(false, "Unexpected integer size %d", length);

				bufferIndex += length;
				asprintf(&formattedArgument, "0x%llX", integer);
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else {
				libtrace_assert(false, "Unknown format argument %%%c in os_log() call", formatString[bufferIndex]);
			}

			if (privacy == privacy_setting_public) {
				sbuf_cat(sbuf, formattedArgument);
			} else {
				sbuf_cat(sbuf, "<private>");
			}

			free(formattedArgument);
			free(attribute);

			argsSeen++;
		}
	}

	char *retval = strdup(sbuf_data(sbuf));
	sbuf_delete(sbuf);
	return retval;
}

__XNU_PRIVATE_EXTERN
const char *os_log_buffer_to_hex_string(const uint8_t *buffer, uint32_t buffer_size) {
	struct sbuf *sbuf = sbuf_new_auto();
	while (buffer_size-- != 0) {
		sbuf_printf(sbuf, "%02X", buffer[0]);
		buffer++;
	}

	const char *retval = strdup(sbuf_data(sbuf));
	sbuf_delete(sbuf);
	return retval;
}
