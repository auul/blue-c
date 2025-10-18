#ifndef BC_RC_H
#define BC_RC_H

#include <stddef.h>

void *
rc_alloc(size_t size, void (*visit)(const void *, void (*)(const void *)));
size_t rc_size(const void *ptr);
const void *rc_ref(const void *ptr);
void rc_unref(const void *ptr);
void *rc_edit(const void *src);
void *rc_resize(const void *src, size_t size);

#endif
