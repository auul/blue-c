#ifndef BC_DICT_H
#define BC_DICT_H

#include <stddef.h>

typedef struct bc_imm_str bc_imm_str;
typedef struct bc_dict bc_dict;

const bc_imm_str *dict_node_key(const bc_dict *dict);
const void *dict_node_value(const bc_dict *dict);

const bc_dict *dict_find(const bc_dict *dict, const char *key, size_t len);
const bc_dict *
dict_define(const bc_dict **dict_p, const char *key, size_t len, void *value);
void dict_delete(const bc_dict **dict_p, const char *key, size_t len);

#endif
