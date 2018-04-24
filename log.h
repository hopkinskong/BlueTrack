#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>

// Color defines
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_ORANGE "\033[0;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_PURPLE "\033[0;35m"
#define COLOR_CYAN "\033[3;36m"
#define COLOR_LIGHT_GREY "\033[0;37m"
#define COLOR_DARK_GREY "\033[1;30m"
#define COLOR_LIGHT_RED "\033[1;31m"
#define COLOR_LIGHT_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_NC "\033[0m"

#ifdef __cplusplus
extern "C" {
	void log_info(const char* TAG, const char* fmt, ...);
	void log_error(const char* TAG, const char* fmt, ...);
	void log_success(const char* TAG, const char* fmt, ...);
}
#endif /* __cplusplus */

void log_info(const char* TAG, const char* fmt, ...);
void log_error(const char* TAG, const char* fmt, ...);
void log_success(const char* TAG, const char* fmt, ...);

#endif /* LOG_H_ */
