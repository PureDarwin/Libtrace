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
	size_t formatIndex = 0;

	while (formatString[formatIndex] != '\0') {
		if (formatString[formatIndex] != '%') {
			sbuf_putc(sbuf, formatString[formatIndex]);
			formatIndex++;
			continue;
		}

		libtrace_assert(formatString[formatIndex] == '%', "next char not % (this shouldn't happen)");
		formatIndex++;

		libtrace_assert(argsSeen < argCount, "Too many format specifiers in os_log string (expected %d)", argCount);

		if (formatString[formatIndex] == '{') {
			char *closingBracket = strchr(formatString + formatIndex, '}');
			*closingBracket = '\0';

			char *attribute = strdup(formatString + formatIndex);
			*closingBracket = '}';
			formatIndex += strlen(attribute) + 2;

			privacy_setting_t privacy = privacy_setting_unset;
			if (contains_attribute(attribute, "private")) {
				privacy = privacy_setting_private;
			} else if (contains_attribute(attribute, "public")) {
				privacy = privacy_setting_public;
			}

			while (formatString[formatIndex] == '.' ||
				   formatString[formatIndex] == '*' ||
				   isnumber(formatString[formatIndex])) {
				formatIndex++;
			}

			char *formattedArgument = NULL;
			if (formatString[formatIndex] == 's') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;
				formattedArgument = calloc(length + 1, sizeof(char));
				memcpy(formattedArgument, buffer + bufferIndex, length);
				bufferIndex += length * sizeof(char);

				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[formatIndex] == 'S') {
				uint8_t length = buffer[bufferIndex];
				bufferIndex++;

				wchar_t *wideArgument = calloc(length + 1, sizeof(wchar_t));
				memcpy(wideArgument, buffer + bufferIndex, length * sizeof(wchar_t));
				bufferIndex += length * sizeof(wchar_t);

				formattedArgument = calloc(length + 1, sizeof(char));
				wcstombs(formattedArgument, wideArgument, length);
				free(wideArgument);

				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[formatIndex] == 'P') {
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
			} else if (formatString[formatIndex] == '@') {
				// FIXME: Correctly describe Objective-C objects
				formattedArgument = strdup("<ObjC object>");
				if (privacy == privacy_setting_unset) privacy = privacy_setting_private;
			} else if (formatString[formatIndex] == 'm') {
				bufferIndex++; // skip zero "length"
				formattedArgument = strdup(strerror(errno));
				if (privacy == privacy_setting_unset) privacy = privacy_setting_public;
			} else if (formatString[formatIndex] == 'd') {
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
			} else if (formatString[formatIndex] == 'u') {
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
			} else if (formatString[formatIndex] == 'x') {
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
			} else if (formatString[formatIndex] == 'X') {
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
				libtrace_assert(false, "Unknown format argument %%%c in os_log() call", formatString[formatIndex]);
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
