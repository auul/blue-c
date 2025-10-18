#include "error.h"
#include "imm_str.h"
#include "rc.h"
#include "src_file.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct bc_src_file {
	const bc_imm_str *path;
	const bc_imm_str *text;
} bc_src_file;

const bc_imm_str *src_file_path(const bc_src_file *file)
{
	return file->path;
}

const bc_imm_str *src_file_text(const bc_src_file *file)
{
	return file->text;
}

static void
src_file_visit(const void *src_file_ptr, void (*visitor)(const void *))
{
	const bc_src_file *file = src_file_ptr;
	visitor(file->path);
	visitor(file->text);
}

static inline bool get_file_len(size_t *dest, FILE *f)
{
	long retval = fseek(f, 0, SEEK_END);
	if (retval) {
		errno = (int)retval;
		return false;
	}

	retval = ftell(f);
	if (retval < 0) {
		return false;
	}
	*dest = (size_t)retval;

	retval = fseek(f, 0, SEEK_SET);
	if (retval) {
		errno = (int)retval;
		return false;
	}

	return true;
}

const bc_src_file *src_file_load(const char *path_src)
{
	return src_file_load_n(path_src, strlen(path_src));
}

const bc_src_file *src_file_load_n(const char *path_src, size_t path_len)
{
	const bc_imm_str *path = imm_str_create_n(path_src, path_len);
	if (!path) {
		return NULL;
	}

	const char *path_read = imm_str_read(path);
	FILE *f = fopen(path_read, "rb");
	if (!f) {
		error_sys(BC_ERROR_ABORT, errno, "Failed to open file '%s'", path_read);
		rc_unref(path);
		return NULL;
	}

	bc_src_file *file = rc_alloc(sizeof(bc_src_file), src_file_visit);
	if (!file) {
		error_sys(BC_ERROR_ABORT, errno, "Failed to open file '%s'", path_read);
		rc_unref(path);
		fclose(f);
		return NULL;
	}

	memset(file, 0, sizeof(*file));
	file->path = path;

	size_t text_len;
	if (!get_file_len(&text_len, f)) {
		error_sys(BC_ERROR_ABORT, errno, "Failed to open file '%s'", path_read);
		rc_unref(file);
		fclose(f);
		return NULL;
	}

	file->text = imm_str_from_file(f, text_len);
	if (!file->text) {
		if (ferror(f)) {
			error_msg(
				BC_ERROR_ABORT,
				"Failed to open file '%s': fread encountered an error");
		} else {
			error_msg(BC_ERROR_ABORT, "Failed to open file '%s'", path_read);
		}
		rc_unref(file);
		fclose(f);
		return NULL;
	}

	fclose(f);

	return file;
}
