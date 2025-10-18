#ifndef BC_IMM_STR_H
#define BC_IMM_STR_H

#include <stddef.h>
#include <stdio.h>

typedef struct bc_imm_str bc_imm_str;

size_t imm_str_len(const bc_imm_str *str);
const char *imm_str_read(const bc_imm_str *str);

const bc_imm_str *imm_str_create(const char *src);
const bc_imm_str *imm_str_create_n(const char *src, size_t len);
const bc_imm_str *imm_str_from_file(FILE *f, size_t len);

typedef struct bc_imm_str_slice {
	const bc_imm_str *str;
	const char *at;
	size_t len;
} bc_imm_str_slice;

void imm_str_slice_init(
	bc_imm_str_slice *slice, const bc_imm_str *str, const char *at, size_t len);
void imm_str_slice_reinit(
	bc_imm_str_slice *slice, const bc_imm_str *str, const char *at, size_t len);
void imm_str_slice_clear(bc_imm_str_slice *slice);

#endif
