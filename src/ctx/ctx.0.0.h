#include "ctx.h"
#include "rc.h"
#include "src_file.h"

#include <stddef.h>

typedef struct bc_ctx {
	const bc_src_file *file;
	const char *at;
	size_t len;
	size_t start_line;
	size_t start_col;
	size_t end_line;
	size_t end_col;
} bc_ctx;

const bc_src_file *ctx_file(const bc_ctx *ctx)
{
	return ctx->file;
}

const char *ctx_at(const bc_ctx *ctx)
{
	return ctx->at;
}

size_t ctx_len(const bc_ctx *ctx)
{
	return ctx->len;
}

const char *
ctx_get_start(size_t *line_dest, size_t *col_dest, const bc_ctx *ctx)
{
	*line_dest = ctx->start_line;
	*col_dest = ctx->start_col;
	return ctx->at;
}

const char *ctx_get_end(size_t *line_dest, size_t *col_dest, const bc_ctx *ctx)
{
	*line_dest = ctx->end_line;
	*col_dest = ctx->end_col;
	return ctx->at + ctx->len;
}

void ctx_init(bc_ctx *ctx, const bc_src_file *file, const char *at, size_t len)
{
	ctx->file = file;
	ctx->at = at;
	ctx->len = len;
}

void ctx_reinit(
	bc_ctx *ctx, const bc_src_file *file, const char *at, size_t len)
{
	rc_unref(ctx->file);
	ctx_init(ctx, file, at, len);
}

void ctx_clear(bc_ctx *ctx)
{
	ctx_reinit(ctx, NULL, NULL, 0);
}
