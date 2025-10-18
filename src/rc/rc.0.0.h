#include "error.h"
#include "rc.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct bc_rc_tag {
	atomic_size_t ref;
	size_t size;
	void (*visit)(const void *, void (*)(const void *));
	char alignas(max_align_t) data[];
} bc_rc_tag;

static const size_t BC_RC_TAG_SIZE = offsetof(bc_rc_tag, data);

static inline size_t get_tagged_size(size_t size)
{
	static const size_t max_alloc = SIZE_MAX - BC_RC_TAG_SIZE;
	if (size > max_alloc) {
		error_msg(
			BC_ERROR_ALLOC_LEVEL,
			"Requested ref-counted allocation %zu exceeds platform maximum %zu",
			size, max_alloc);
		return 0;
	}
	return BC_RC_TAG_SIZE + size;
}

void *rc_alloc(size_t size, void (*visit)(const void *, void (*)(const void *)))
{
	size_t total = get_tagged_size(size);
	bc_rc_tag *tag = malloc(total);
	if (!tag) {
		error_alloc(total);
		return NULL;
	}

	atomic_init(&tag->ref, 1);
	tag->size = size;
	tag->visit = visit;

	return tag->data;
}

static inline bc_rc_tag *get_tag(const void *ptr)
{
	return (bc_rc_tag *)((char *)ptr - BC_RC_TAG_SIZE);
}

size_t rc_size(const void *ptr)
{
	if (!ptr) {
		return 0;
	}

	const bc_rc_tag *tag = get_tag(ptr);
	return tag->size;
}

static inline void inc_tag_ref(bc_rc_tag *tag)
{
	atomic_fetch_add_explicit(&tag->ref, 1, memory_order_relaxed);
}

static inline size_t dec_tag_ref(bc_rc_tag *tag)
{
	size_t ref = atomic_fetch_sub_explicit(&tag->ref, 1, memory_order_release);
	if (ref > 1) {
		return ref - 1;
	} else if (tag->visit) {
		tag->visit(tag->data, rc_unref);
	}
	free(tag);
	return 0;
}

static inline bool is_tag_unique(const bc_rc_tag *tag)
{
	return atomic_load_explicit(&tag->ref, memory_order_relaxed) == 0 &&
		   atomic_load_explicit(&tag->ref, memory_order_acquire) == 0;
}

const void *rc_ref(const void *ptr)
{
	if (ptr) {
		inc_tag_ref(get_tag(ptr));
	}
	return ptr;
}

void rc_unref(const void *ptr)
{
	if (!ptr) {
		return;
	}
	dec_tag_ref(get_tag(ptr));
}

static void rc_ref_visit(const void *ptr)
{
	rc_ref(ptr);
}

static inline void *clone_data(bc_rc_tag *tag, size_t dest_size)
{
	void *dest = rc_alloc(dest_size, tag->visit);
	if (!dest) {
		dec_tag_ref(tag);
		return NULL;
	}

	memcpy(dest, tag->data, tag->size);
	if (dec_tag_ref(tag) && tag->visit) {
		tag->visit(dest, rc_ref_visit);
	}
	return dest;
}

void *rc_edit(const void *src)
{
	if (!src) {
		return NULL;
	}

	bc_rc_tag *tag = get_tag(src);
	if (is_tag_unique(tag)) {
		return (void *)src;
	}
	return clone_data(tag, tag->size);
}

static inline bc_rc_tag *realloc_tag(bc_rc_tag *tag, size_t size)
{
	size_t total = get_tagged_size(size);
	if (!total) {
		dec_tag_ref(tag);
		return NULL;
	}

	bc_rc_tag *re_tag = realloc(tag, total);
	if (!re_tag) {
		dec_tag_ref(re_tag);
		return NULL;
	}
	tag = re_tag;

	memset(tag->data + tag->size, 0, size - tag->size);
	tag->size = size;

	return tag;
}

void *rc_resize(const void *src, size_t size)
{
	if (!src) {
		return rc_alloc(size, NULL);
	}

	bc_rc_tag *tag = get_tag(src);
	if (is_tag_unique(tag)) {
		tag = realloc_tag(tag, size);
		if (!tag) {
			return NULL;
		}
		return tag->data;
	}
	return clone_data(tag, size);
}
