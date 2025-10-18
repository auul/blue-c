#ifndef BC_SRC_FILE_H
#define BC_SRC_FILE_H

#include <stddef.h>

typedef struct bc_src_file bc_src_file;
typedef struct bc_imm_str bc_imm_str;

const bc_imm_str *src_file_path(const bc_src_file *file);
const bc_imm_str *src_file_text(const bc_src_file *file);

const bc_src_file *src_file_load(const char *path_src);
const bc_src_file *src_file_load_n(const char *path_src, size_t path_len);

#endif
