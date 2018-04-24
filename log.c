#include "log.h"

void log_info(const char* TAG, const char* fmt, ...) {
	va_list arglist;

	printf("%s[%s/I] ", COLOR_ORANGE, TAG);
	va_start(arglist, fmt);
	vprintf(fmt, arglist);
	va_end(arglist);
	printf(COLOR_NC);
	printf("\n");
}

void log_error(const char* TAG, const char* fmt, ...) {
	va_list arglist;

	printf("%s[%s/E] ", COLOR_RED, TAG);
	va_start(arglist, fmt);
	vprintf(fmt, arglist);
	va_end(arglist);
	printf(COLOR_NC);
	printf("\n");
}

void log_success(const char* TAG, const char* fmt, ...) {
	va_list arglist;

	printf("%s[%s/S] ", COLOR_GREEN, TAG);
	va_start(arglist, fmt);
	vprintf(fmt, arglist);
	va_end(arglist);
	printf(COLOR_NC);
	printf("\n");
}
