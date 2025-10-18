#include "error.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	BC_ERROR_USE_FMT_ANSI,
};

static int g_use_fmt = BC_ERROR_USE_FMT_ANSI;

static inline const char *get_level_fmt_ansi(int level)
{
	switch (level) {
	case BC_ERROR_NOTE:
		return "\033[1;36m";
	case BC_ERROR_WARN:
		return "\033[1;35m";
	case BC_ERROR_FATAL:
		return "\033[1;91m";
	default:
		return "\033[1;31m";
	}
}

static inline const char *get_fmt_off(void)
{
	switch (g_use_fmt) {
	case BC_ERROR_USE_FMT_ANSI:
		return "\033[0m";
	default:
		return "";
	}
}

static inline const char *get_level_fmt(int level)
{
	switch (g_use_fmt) {
	case BC_ERROR_USE_FMT_ANSI:
		return get_level_fmt_ansi(level);
	default:
		return "";
	}
}

static inline const char *get_level_text(int level)
{
	switch (level) {
	case BC_ERROR_NOTE:
		return "note";
	case BC_ERROR_WARN:
		return "warning";
	case BC_ERROR_FATAL:
		return "fatal";
	default:
		return "error";
	}
}

static inline void begin_msg_v(int level, const char *fmt, va_list args)
{
	fprintf(
		stderr, "%s%s:%s ", get_level_fmt(level), get_level_text(level),
		get_fmt_off());
	vfprintf(stderr, fmt, args);
}

static inline void end_msg(int level)
{
	fprintf(stderr, "\n");
	if (level == BC_ERROR_FATAL) {
		exit(EXIT_FAILURE);
	}
}

void error_msg(int level, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	begin_msg_v(level, fmt, args);
	va_end(args);
	end_msg(level);
}

void error_sys(int level, int errnum, const char *fmt, ...)
{
	char buf[BC_ERROR_BUFFER_SIZE];
#ifdef _GNU_SOURCE
	const char *msg = strerror_r(errnum, buf, sizeof(buf));
#else
	switch (strerror_r(errnum, buf, sizeof(buf))) {
	case EINVAL:
		snprintf(buf, sizeof(buf), "strerror_r: Unknown errno %d", errnum);
		break;
	case ERANGE:
		snprintf(
			buf, sizeof(buf), "strerror_r: Insufficient error buffer size %zu",
			sizeof(buf));
		break;
	default:
		break;
	}
	const char *msg = buf;
#endif
	fprintf(stderr, "%s", msg);
}

void error_alloc(size_t size)
{
	error_sys(
		BC_ERROR_ALLOC_LEVEL, ENOMEM, "Failed to allocate %zu bytes", size);
}
