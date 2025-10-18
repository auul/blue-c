#include "dict.h"
#include "imm_str.h"
#include "rc.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct bc_dict {
	const bc_imm_str *key;
	const void *value;
	struct bc_dict *left;
	struct bc_dict *right;
	uint32_t priority;
} bc_dict;

const bc_imm_str *dict_node_key(const bc_dict *node)
{
	return node->key;
}

const void *dict_node_value(const bc_dict *node)
{
	return node->value;
}

static void dict_visit(const void *dict_ptr, void (*visitor)(const void *))
{
	const bc_dict *dict = dict_ptr;
	visitor(dict->key);
	visitor(dict->value);
	visitor(dict->left);
	visitor(dict->right);
}

static inline bc_dict *create_node(
	const bc_imm_str *key, const void *value, bc_dict *left, bc_dict *right)
{
	bc_dict *node = rc_alloc(sizeof(bc_dict), dict_visit);
	if (!node) {
		rc_unref(key);
		rc_unref(value);
		rc_unref(left);
		rc_unref(right);
		return NULL;
	}

	node->key = key;
	node->value = value;
	node->left = left;
	node->right = right;
	return node;
}

static inline int compare_keys(const bc_dict *node, const char *key, size_t len)
{
	const bc_imm_str *target_str = node->key;
	const char *target = imm_str_read(target_str);
	size_t target_len = imm_str_len(target_str);

	if (target_len < len) {
		int cmp = memcmp(target, key, target_len);
		if (cmp) {
			return cmp;
		}
		return -1;
	} else if (len < target_len) {
		int cmp = memcmp(target, key, len);
		if (cmp) {
			return cmp;
		}
		return 1;
	}
	return memcmp(target, key, len);
}

const bc_dict *dict_find(const bc_dict *dict, const char *key, size_t len)
{
	while (dict) {
		int cmp = compare_keys(dict, key, len);
		if (cmp > 0) {
			dict = dict->left;
		} else if (cmp < 0) {
			dict = dict->right;
		} else {
			return dict;
		}
	}
	return NULL;
}

static inline bc_dict *split_dict(
	bc_dict **left_p, bc_dict **right_p, const bc_dict *dict, const char *key,
	size_t len)
{
	*left_p = NULL;
	*right_p = NULL;

	while (dict) {
		bc_dict *node = rc_edit(dict);
		if (!node) {
			rc_unref(*left_p);
			*left_p = NULL;
			rc_unref(*right_p);
			*right_p = NULL;
			return NULL;
		}

		int cmp = compare_keys(node, key, len);
		if (cmp > 0) {
			*left_p = node;
			left_p = &node->right;
			node = *left_p;
		} else if (cmp < 0) {
			*right_p = node;
			right_p = &node->left;
			node = *right_p;
		} else {
			*left_p = node->left;
			node->left = NULL;
			*right_p = node->right;
			node->right = NULL;
			return node;
		}
	}

	*left_p = NULL;
	*right_p = NULL;

	return NULL;
}

static inline bc_dict *
leaf_node(const char *key_src, size_t key_len, const void *value)
{
	const bc_imm_str *key = imm_str_create_n(key_src, key_len);
	if (!key) {
		rc_unref(value);
		return NULL;
	}
	return create_node(key, value, NULL, NULL);
}

static inline bc_dict *rejoin_dict(bc_dict *left, bc_dict *right)
{
	bc_dict *dict;
	bc_dict **dest = &dict;

	while (left && right) {
		if (left->priority > right->priority) {
			*dest = left;
			dest = &left->right;
			left = *dest;
		} else {
			*dest = right;
			dest = &right->left;
			right = *dest;
		}
	}

	if (left) {
		*dest = left;
	} else {
		*dest = right;
	}
	return dict;
}

const bc_dict *
dict_define(const bc_dict **dict_p, const char *key, size_t len, void *value)
{
	const bc_dict *dict = *dict_p;
	if (!dict) {
		dict = leaf_node(key, len, value);
		*dict_p = dict;
		return dict;
	}

	bc_dict *left, *right;
	bc_dict *node = split_dict(&left, &right, dict, key, len);
	if (node) {
		rc_unref(node->value);
		node->value = value;
	} else if (left || right) {
		node = leaf_node(key, len, value);
		if (!node) {
			rc_unref(left);
			rc_unref(right);
			*dict_p = NULL;
			return NULL;
		}
	} else {
		rc_unref(value);
		*dict_p = NULL;
		return NULL;
	}

	dict = rejoin_dict(left, rejoin_dict(node, right));
	*dict_p = dict;

	return node;
}

void dict_delete(const bc_dict **dict_p, const char *key, size_t len)
{
	if (!*dict_p) {
		return;
	}

	bc_dict *left, *right;
	bc_dict *node = split_dict(&left, &right, *dict_p, key, len);
	if (node) {
		rc_unref(node);
	}
	*dict_p = rejoin_dict(left, right);
}
