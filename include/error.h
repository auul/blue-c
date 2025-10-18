#ifndef BC_ERROR_H
#define BC_ERROR_H

#include <stddef.h>

enum {
	BC_ERROR_ABORT,
	BC_ERROR_NOTE,
	BC_ERROR_WARN,
	BC_ERROR_FATAL,
};

#define BC_ERROR_BUFFER_SIZE 128
#define BC_ERROR_ALLOC_LEVEL BC_ERROR_FATAL

void error_msg(int level, const char *fmt, ...);
void error_sys(int level, int errnum, const char *fmt, ...);
void error_alloc(size_t size);

#endif
