#ifndef BC_CTX_H
#define BC_CTX_H

#include <stddef.h>

typedef struct bc_ctx bc_ctx;
typedef struct bc_src_file bc_src_file;

const bc_src_file *ctx_file(const bc_ctx *ctx);
const char *ctx_at(const bc_ctx *ctx);
size_t ctx_len(const bc_ctx *ctx);
const char *
ctx_get_start(size_t *line_dest, size_t *col_dest, const bc_ctx *ctx);
const char *ctx_get_end(size_t *line_dest, size_t *col_dest, const bc_ctx *ctx);

void ctx_init(bc_ctx *ctx, const bc_src_file *file, const char *at, size_t len);
void ctx_reinit(
	bc_ctx *ctx, const bc_src_file *file, const char *at, size_t len);
void ctx_clear(bc_ctx *ctx);

#endif
