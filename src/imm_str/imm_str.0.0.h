#include "error.h"
#include "imm_str.h"
#include "rc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct bc_imm_str {
	size_t len;
	char data[];
} bc_imm_str;

size_t imm_str_len(const bc_imm_str *str)
{
	return str->len;
}

const char *imm_str_read(const bc_imm_str *str)
{
	return str->data;
}

static inline size_t get_tagged_size(size_t len)
{
	static const size_t header_size = offsetof(bc_imm_str, data);
	static const size_t max_len = SIZE_MAX - header_size - 1;
	if (len > max_len) {
		error_msg(
			BC_ERROR_ALLOC_LEVEL,
			"Requested string length %zu exceeds platform maximum %zu", len,
			max_len);
		return 0;
	}
	return header_size + len + 1;
}

static inline bc_imm_str *alloc_str(size_t len)
{
	size_t total = get_tagged_size(len);
	if (!total) {
		return NULL;
	}

	bc_imm_str *str = rc_alloc(total, NULL);
	if (!str) {
		return NULL;
	}

	str->len = len;

	return str;
}

const bc_imm_str *imm_str_create(const char *src)
{
	return imm_str_create_n(src, strlen(src));
}

const bc_imm_str *imm_str_create_n(const char *src, size_t len)
{
	bc_imm_str *dest = alloc_str(len);
	if (!dest) {
		return NULL;
	}

	memcpy(dest->data, src, len);
	dest->data[len] = 0;

	return dest;
}

const bc_imm_str *imm_str_from_file(FILE *f, size_t len)
{
	bc_imm_str *str = alloc_str(len);
	if (!str) {
		return NULL;
	}

	size_t read_len = fread(str->data, 1, len, f);
	if (read_len < len) {
		rc_unref(str);
		return NULL;
	}
	str->data[len] = 0;

	return str;
}

/* Slice Methods */

void imm_str_slice_init(
	bc_imm_str_slice *slice, const bc_imm_str *str, const char *at, size_t len)
{
	slice->str = str;
	slice->at = at;
	slice->len = len;
}

void imm_str_slice_reinit(
	bc_imm_str_slice *slice, const bc_imm_str *str, const char *at, size_t len)
{
	rc_unref(slice->str);
	imm_str_slice_init(slice, str, at, len);
}

void imm_str_slice_clear(bc_imm_str_slice *slice)
{
	imm_str_slice_reinit(slice, NULL, NULL, 0);
}
