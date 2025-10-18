#ifndef BC_VEC_H
#define BC_VEC_H

#include "error.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Configuration Knobs */

#ifndef BC_VEC_INIT_CAP
#	define BC_VEC_INIT_CAP 16
#endif

#ifndef BC_VEC_GROW_FACTOR
#	define BC_VEC_GROW_FACTOR 2
#endif

#ifndef BC_VEC_SHRINK_THRESHOLD
#	define BC_VEC_SHRINK_THRESHOLD 4
#endif

#ifndef BC_VEC_SHRINK_MIN
#	define BC_VEC_SHRINK_MIN 8
#endif

#ifndef BC_VEC_SHRINK_RESERVE
#	define BC_VEC_SHRINK_RESERVE 2
#endif

#ifndef BC_VEC_HYSTERESIS
#	define BC_VEC_HYSTERESIS 2
#endif

/* Constants */

enum {
	BC_VEC_E_ALLOC = -1,
	BC_VEC_E_BOUNDS = -2,
	BC_VEC_E_UNDERFLOW = -3,
	BC_VEC_E_OVERFLOW = -4,

	BC_VEC_SUCCESS = 0,
};

/* Templates */

#if BC_VEC_HYSTERESIS
#	define BC_VEC_STRUCT(api, type) \
		typedef struct api {         \
			size_t cap;              \
			size_t len;              \
			unsigned grow;           \
			unsigned shrink;         \
			type elem[];             \
		} api;
#else
#	define BC_VEC_STRUCT(api, type) \
		typedef struct api {         \
			size_t cap;              \
			size_t len;              \
			type elem[];             \
		} api;
#endif

#define BC_VEC_HEADER_SIZE(api) offsetof(api, elem)
#define BC_VEC_MAX_CAP(api, type)                         \
	((SIZE_MAX - BC_VEC_HEADER_SIZE(api)) / sizeof(type))

#if BC_VEC_SHRINK_THRESHOLD
#	define BC_VEC_IS_ABOVE_SHRINK_THRESHOLD(api)                    \
		static inline bool is_above_shrink_threshold(const api *vec) \
		{                                                            \
			return vec->len > vec->cap / (BC_VEC_SHRINK_THRESHOLD);  \
		}
#else
#	define BC_VEC_IS_ABOVE_SHRINK_THRESHOLD(api)                    \
		static inline bool is_above_shrink_threshold(const api *vec) \
		{                                                            \
			return vec->len == vec->cap;                             \
		}
#endif

#if BC_VEC_HYSTERESIS
#	define BC_VEC_INC_GROW(api)              \
		static inline void inc_grow(api *vec) \
		{                                     \
			vec->grow++;                      \
			if (vec->grow >= UINT_MAX >> 1) { \
				vec->grow >>= 1;              \
				vec->shrink >>= 1;            \
			}                                 \
		}
#	define BC_VEC_ISNT_SHRINKABLE(api)                           \
		static inline bool isnt_shrinkable(const api *vec)        \
		{                                                         \
			return (vec->cap == BC_VEC_SHRINK_MIN) ||             \
				   is_above_shrink_threshold(vec) ||              \
				   (vec->shrink < vec->grow / BC_VEC_HYSTERESIS); \
		}
#	define BC_VEC_INC_SHRINK(api)              \
		static inline void inc_shrink(api *vec) \
		{                                       \
			vec->shrink++;                      \
		}
#	define BC_VEC_RESET_SHRINK(api)              \
		static inline void reset_shrink(api *vec) \
		{                                         \
			vec->grow >>= 1;                      \
			vec->shrink = 0;                      \
		}
#else
#	define BC_VEC_INC_GROW(api)              \
		static inline void inc_grow(api *vec) \
		{                                     \
			((void)(vec));                    \
		}
#	define BC_VEC_ISNT_SHRINKABLE(api)                    \
		static inline bool isnt_shrinkable(const api *vec) \
		{                                                  \
			return vec->cap == BC_VEC_SHRINK_MIN ||        \
				   is_above_shrink_threshold(vec);         \
		}
#	define BC_VEC_INC_SHRINK(api)              \
		static inline void inc_shrink(api *vec) \
		{                                       \
			((void)(vec));                      \
		}
#	define BC_VEC_RESET_SHRINK(api)              \
		static inline void reset_shrink(api *vec) \
		{                                         \
			((void)(vec));                        \
		}
#endif

#if BC_VEC_GROW_FACTOR
#	define BC_VEC_GROW_CAP(api, type)                                      \
		static inline size_t grow_cap(const api *vec, size_t min)           \
		{                                                                   \
			size_t cap = vec->cap ? vec->cap : 1;                           \
			while (cap < min) {                                             \
				if (cap > BC_VEC_MAX_CAP(api, type) / BC_VEC_GROW_FACTOR) { \
					return BC_VEC_MAX_CAP(api, type);                       \
				}                                                           \
				cap *= BC_VEC_GROW_FACTOR;                                  \
			}                                                               \
			return cap;                                                     \
		}
#else
#	define BC_VEC_GROW_CAP(api, type)                            \
		static inline size_t grow_cap(const api *vec, size_t min) \
		{                                                         \
			return min;                                           \
		}
#endif

#if BC_VEC_SHRINK_RESERVE
#	define BC_VEC_SHRINK_CAP(api)                                             \
		static inline size_t shrink_cap(api *vec)                              \
		{                                                                      \
			return vec->len + ((vec->cap - vec->len) / BC_VEC_SHRINK_RESERVE); \
		}
#else
#	define BC_VEC_SHRINK_CAP(api)                \
		static inline size_t shrink_cap(api *vec) \
		{                                         \
			return vec->len;                      \
		}
#endif

#define BC_VEC_HELPERS(api, type)         \
	BC_VEC_IS_ABOVE_SHRINK_THRESHOLD(api) \
	BC_VEC_INC_GROW(api)                  \
	BC_VEC_ISNT_SHRINKABLE(api)           \
	BC_VEC_INC_SHRINK(api)                \
	BC_VEC_RESET_SHRINK(api)              \
	BC_VEC_GROW_CAP(api, type)            \
	BC_VEC_SHRINK_CAP(api)

#define BC_VEC_UPDATE_DUMMY(api, type)                  \
	static inline void update_unit(type *unit)          \
	{                                                   \
		((void)(unit));                                 \
	}                                                   \
                                                        \
	static inline void update_range(                    \
		api *vec, size_t start_index, size_t end_index) \
	{                                                   \
		((void)(vec));                                  \
		((void)(start_index));                          \
		((void)(end_index));                            \
	}

#define BC_VEC_UPDATE_W_UTOR(api, type, utor)              \
	static inline void update_unit(type *unit)             \
	{                                                      \
		utor(unit);                                        \
	}                                                      \
                                                           \
	static inline void update_range(                       \
		api *vec, size_t start_index, size_t end_index)    \
	{                                                      \
		for (size_t i = start_index; i < end_index; i++) { \
			update_unit(&vec->elem[i]);                    \
		}                                                  \
	}

#define BC_VEC_DESTROY_DUMMY(api, type)                 \
	static inline void destroy_unit(type *unit)         \
	{                                                   \
		((void)(unit));                                 \
	}                                                   \
                                                        \
	static inline void destroy_range(                   \
		api *vec, size_t start_index, size_t end_index) \
	{                                                   \
		((void)(vec));                                  \
		((void)(start_index));                          \
		((void)(end_index));                            \
	}

#define BC_VEC_DESTROY_W_DTOR(api, type, dtor)          \
	static inline void destroy_unit(type *unit)         \
	{                                                   \
		dtor(unit);                                     \
	}                                                   \
                                                        \
	static inline void destroy_range(                   \
		api *vec, size_t start_index, size_t end_index) \
	{                                                   \
		for (size_t i = end_index; i > start_index;) {  \
			i--;                                        \
			destroy_unit(&vec->elem[i]);                \
		}                                               \
	}

#define BC_VEC_IMPLEMENT(api, type) \
	BC_VEC_STRUCT(api, type)        \
	BC_VEC_UPDATE_DUMMY(api, type)  \
	BC_VEC_DESTROY_DUMMY(api, type) \
	BC_VEC_TEMPLATE(api, type)

#define BC_VEC_IMPLEMENT_W_DTOR(api, type, dtor) \
	BC_VEC_STRUCT(api, type)                     \
	BC_VEC_UPDATE_DUMMY(api, type)               \
	BC_VEC_DESTROY_W_DTOR(api, type, dtor)       \
	BC_VEC_TEMPLATE(api, type)

#define BC_VEC_IMPLEMENT_W_UTOR(api, type, utor, dtor) \
	BC_VEC_STRUCT(api, type)                           \
	BC_VEC_UPDATE_W_UTOR(api, type, utor)              \
	BC_VEC_DESTROY_W_DTOR(api, type, dtor)             \
	BC_VEC_TEMPLATE(api, type)

#define BC_VEC_TEMPLATE(api, type)                                            \
	BC_VEC_HELPERS(api, type)                                                 \
                                                                              \
	static inline bool is_cap_too_high(size_t cap)                            \
	{                                                                         \
		if (cap > BC_VEC_MAX_CAP(api, type)) {                                \
			error_msg(                                                        \
				BC_ERROR_ALLOC_LEVEL,                                         \
				"Requested vector cap %zu of type %s exceeds the "            \
				"platform "                                                   \
				"maximum %zu",                                                \
				cap, #type, BC_VEC_MAX_CAP(api, type));                       \
			return true;                                                      \
		}                                                                     \
		return false;                                                         \
	}                                                                         \
                                                                              \
	static inline size_t calc_total_size(size_t cap)                          \
	{                                                                         \
		if (is_cap_too_high(cap)) {                                           \
			return 0;                                                         \
		}                                                                     \
		return BC_VEC_HEADER_SIZE(api) + cap * sizeof(type);                  \
	}                                                                         \
	api *api##_create(size_t cap)                                             \
	{                                                                         \
		size_t total = calc_total_size(cap);                                  \
		if (!total) {                                                         \
			return NULL;                                                      \
		}                                                                     \
                                                                              \
		api *vec = calloc(1, total);                                          \
		if (!vec) {                                                           \
			error_alloc(total);                                               \
			return NULL;                                                      \
		}                                                                     \
                                                                              \
		vec->cap = cap;                                                       \
		vec->len = 0;                                                         \
                                                                              \
		return vec;                                                           \
	}                                                                         \
                                                                              \
	void api##_destroy(api *vec)                                              \
	{                                                                         \
		if (!vec) {                                                           \
			return;                                                           \
		}                                                                     \
		destroy_range(vec, 0, vec->len);                                      \
		free(vec);                                                            \
	}                                                                         \
	static inline int fatal_error(api **vec_p, int status)                    \
	{                                                                         \
		api##_destroy(*vec_p);                                                \
		*vec_p = NULL;                                                        \
		return status;                                                        \
	}                                                                         \
                                                                              \
	int api##_reserve(api **vec_p, size_t min)                                \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		if (vec->cap >= min) {                                                \
			return BC_VEC_SUCCESS;                                            \
		} else if (is_cap_too_high(min)) {                                    \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		size_t cap = grow_cap(vec, min);                                      \
		size_t total = calc_total_size(cap);                                  \
                                                                              \
		vec = realloc(vec, total);                                            \
		if (!vec) {                                                           \
			error_alloc(total);                                               \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		vec->cap = cap;                                                       \
		inc_grow(vec);                                                        \
		*vec_p = vec;                                                         \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline bool is_grow_request_too_high(size_t len, size_t request)   \
	{                                                                         \
		if (BC_VEC_MAX_CAP(api, type) - len < request) {                      \
			error_msg(                                                        \
				BC_ERROR_ALLOC_LEVEL,                                         \
				"Requested %s vector growth by %zu exceeds the platform "     \
				"maximum %zu",                                                \
				#type, request, BC_VEC_MAX_CAP(api, type));                   \
			return true;                                                      \
		}                                                                     \
		return false;                                                         \
	}                                                                         \
                                                                              \
	int api##_grow(api **vec_p, size_t request)                               \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		if (vec->cap - vec->len >= request) {                                 \
			return BC_VEC_SUCCESS;                                            \
		}                                                                     \
                                                                              \
		size_t len = vec->len;                                                \
		if (is_grow_request_too_high(len, request)) {                         \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		return api##_reserve(vec_p, len + request);                           \
	}                                                                         \
                                                                              \
	int api##_shrink(api **vec_p)                                             \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		reset_shrink(vec);                                                    \
                                                                              \
		if (vec->len == vec->cap) {                                           \
			return BC_VEC_SUCCESS;                                            \
		}                                                                     \
                                                                              \
		size_t total = calc_total_size(vec->len);                             \
		vec = realloc(vec, total);                                            \
		if (!vec) {                                                           \
			error_alloc(total);                                               \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		vec->cap = vec->len;                                                  \
		*vec_p = vec;                                                         \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline int request_shrink(api **vec_p)                             \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		inc_shrink(vec);                                                      \
		if (isnt_shrinkable(vec)) {                                           \
			return BC_VEC_SUCCESS;                                            \
		}                                                                     \
		reset_shrink(vec);                                                    \
                                                                              \
		size_t cap = shrink_cap(vec);                                         \
		if (cap < BC_VEC_SHRINK_MIN) {                                        \
			cap = BC_VEC_SHRINK_MIN;                                          \
		}                                                                     \
                                                                              \
		size_t total = calc_total_size(cap);                                  \
		if (!total) {                                                         \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		vec = realloc(vec, total);                                            \
		if (!vec) {                                                           \
			error_alloc(total);                                               \
			return fatal_error(vec_p, BC_VEC_E_ALLOC);                        \
		}                                                                     \
                                                                              \
		vec->cap = cap;                                                       \
		*vec_p = vec;                                                         \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_clear(api *vec)                                                 \
	{                                                                         \
		destroy_range(vec, 0, vec->len);                                      \
		vec->len = 0;                                                         \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_trunc_unsafe(api **vec_p, size_t len)                           \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		size_t tail = vec->len - len;                                         \
		destroy_range(vec, tail, vec->len);                                   \
		vec->len = tail;                                                      \
		return request_shrink(vec_p);                                         \
	}                                                                         \
                                                                              \
	int api##_pop_n_unsafe(type *dest, api **vec_p, size_t n)                 \
	{                                                                         \
		const api *vec = *vec_p;                                              \
		memcpy(dest, &vec->elem[vec->len - n], n * sizeof(*dest));            \
		return api##_trunc_unsafe(vec_p, n);                                  \
	}                                                                         \
                                                                              \
	type api##_pop_unsafe(api **vec_p)                                        \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		type value = vec->elem[vec->len - 1];                                 \
		vec->len--;                                                           \
		request_shrink(vec_p);                                                \
		return value;                                                         \
	}                                                                         \
                                                                              \
	static inline void shift_tail(                                            \
		api *vec, size_t dest_index, size_t src_index)                        \
	{                                                                         \
		memmove(                                                              \
			vec->elem + dest_index, vec->elem + src_index,                    \
			(vec->len - src_index) * sizeof(type));                           \
	}                                                                         \
                                                                              \
	int api##_delete_unsafe(api **vec_p, size_t index, size_t len)            \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		size_t end_index = index + len;                                       \
		destroy_range(vec, index, end_index);                                 \
		shift_tail(vec, index, end_index);                                    \
		vec->len -= len;                                                      \
		return request_shrink(vec_p);                                         \
	}                                                                         \
                                                                              \
	static inline int perform_insert(                                         \
		api *vec, size_t index, const type *src, size_t len)                  \
	{                                                                         \
		shift_tail(vec, index + len, index);                                  \
		memcpy(&vec->elem[index], src, len * sizeof(*src));                   \
		update_range(vec, index, index + len);                                \
		vec->len += len;                                                      \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_insert_unsafe(                                                  \
		api **vec_p, size_t index, const type *src, size_t len)               \
	{                                                                         \
		int retval = api##_grow(vec_p, len);                                  \
		if (retval) {                                                         \
			return retval;                                                    \
		}                                                                     \
		perform_insert(*vec_p, index, src, len);                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_overwrite_unsafe(                                               \
		api **vec_p, size_t index, const type *src, size_t len)               \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		destroy_range(vec, index, index + len);                               \
		memcpy(&vec->elem[index], src, len * sizeof(*src));                   \
		update_range(vec, index, index + len);                                \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline void perform_splice_unsafe(                                 \
		api **vec_p, size_t index, size_t delete_len, const type *src,        \
		size_t insert_len)                                                    \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		size_t delete_end = index + delete_len;                               \
		destroy_range(vec, index, index + delete_len);                        \
		shift_tail(vec, index + insert_len, delete_end);                      \
		memcpy(&vec->elem[index], src, insert_len * sizeof(*src));            \
		update_range(vec, index, index + insert_len);                         \
		vec->len += insert_len - delete_len;                                  \
	}                                                                         \
                                                                              \
	static inline int splice_grow_unsafe(                                     \
		api **vec_p, size_t index, size_t delete_len, const type *src,        \
		size_t insert_len)                                                    \
	{                                                                         \
		int retval = api##_grow(vec_p, insert_len - delete_len);              \
		if (retval) {                                                         \
			return retval;                                                    \
		}                                                                     \
		perform_splice_unsafe(vec_p, index, delete_len, src, insert_len);     \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline int splice_shrink_unsafe(                                   \
		api **vec_p, size_t index, size_t delete_len, const type *src,        \
		size_t insert_len)                                                    \
	{                                                                         \
		perform_splice_unsafe(vec_p, index, delete_len, src, insert_len);     \
		return request_shrink(vec_p);                                         \
	}                                                                         \
                                                                              \
	int api##_splice_unsafe(                                                  \
		api **vec_p, size_t index, size_t delete_len, const type *src,        \
		size_t insert_len)                                                    \
	{                                                                         \
		if (delete_len > insert_len) {                                        \
			return splice_shrink_unsafe(                                      \
				vec_p, index, delete_len, src, insert_len);                   \
		}                                                                     \
		return splice_grow_unsafe(vec_p, index, delete_len, src, insert_len); \
	}                                                                         \
                                                                              \
	int api##_trunc(api **vec_p, size_t len)                                  \
	{                                                                         \
		const api *vec = *vec_p;                                              \
		if (len > vec->len) {                                                 \
			return BC_VEC_E_UNDERFLOW;                                        \
		}                                                                     \
		return api##_trunc_unsafe(vec_p, len);                                \
	}                                                                         \
                                                                              \
	int api##_delete(api **vec_p, size_t index, size_t len)                   \
	{                                                                         \
		const api *vec = *vec_p;                                              \
		if (index > vec->len) {                                               \
			return BC_VEC_E_BOUNDS;                                           \
		} else if (vec->len - index < len) {                                  \
			return BC_VEC_E_UNDERFLOW;                                        \
		}                                                                     \
		return api##_delete_unsafe(vec_p, index, len);                        \
	}                                                                         \
                                                                              \
	int api##_append(api **vec_p, const type *src, size_t len)                \
	{                                                                         \
		int retval = api##_grow(vec_p, len);                                  \
		if (retval) {                                                         \
			return retval;                                                    \
		}                                                                     \
                                                                              \
		api *vec = *vec_p;                                                    \
		memcpy(&vec->elem[vec->len], src, len * sizeof(*src));                \
		update_range(vec, vec->len, vec->len + len);                          \
		vec->len += len;                                                      \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_push(api **vec_p, type value)                                   \
	{                                                                         \
		int retval = api##_grow(vec_p, 1);                                    \
		if (retval) {                                                         \
			destroy_unit(&value);                                             \
			return retval;                                                    \
		}                                                                     \
                                                                              \
		api *vec = *vec_p;                                                    \
		vec->elem[vec->len] = value;                                          \
		vec->len++;                                                           \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline bool is_not_inplace(const api *vec, const type *src)        \
	{                                                                         \
		uintptr_t elem_uptr = (uintptr_t)vec->elem;                           \
		uintptr_t src_uptr = (uintptr_t)src;                                  \
		return src_uptr < elem_uptr ||                                        \
			   elem_uptr + vec->len * sizeof(type) < src_uptr;                \
	}                                                                         \
                                                                              \
	static inline void shift_span(                                            \
		api *vec, size_t dest_index, size_t src_index, size_t len)            \
	{                                                                         \
		memmove(                                                              \
			&vec->elem[dest_index], &vec->elem[src_index],                    \
			len * sizeof(type));                                              \
	}                                                                         \
                                                                              \
	int api##_insert(api **vec_p, size_t index, const type *src, size_t len)  \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		if (index == vec->len) {                                              \
			return api##_append(vec_p, src, len);                             \
		} else if (index > vec->len) {                                        \
			return BC_VEC_E_BOUNDS;                                           \
		} else if (is_not_inplace(vec, src)) {                                \
			return api##_insert_unsafe(vec_p, index, src, len);               \
		}                                                                     \
                                                                              \
		size_t src_index = (size_t)(src - vec->elem);                         \
		int retval = api##_grow(vec_p, len);                                  \
		vec = *vec_p;                                                         \
		if (retval) {                                                         \
			return retval;                                                    \
		} else if (src_index + len < index) {                                 \
			return perform_insert(vec, index, &vec->elem[src_index], len);    \
		}                                                                     \
                                                                              \
		shift_tail(vec, index + len, index);                                  \
		if (src_index >= index) {                                             \
			return perform_insert(                                            \
				vec, index, &vec->elem[src_index + len], len);                \
		}                                                                     \
                                                                              \
		update_range(vec, src_index, index);                                  \
		shift_span(vec, index, src_index, len);                               \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_overwrite(                                                      \
		api **vec_p, size_t index, const type *src, size_t len)               \
	{                                                                         \
		api *vec = *vec_p;                                                    \
		if (index > vec->len) {                                               \
			return BC_VEC_E_BOUNDS;                                           \
		} else if (vec->len - index < len) {                                  \
			return BC_VEC_E_OVERFLOW;                                         \
		} else if (is_not_inplace(vec, src)) {                                \
			return api##_overwrite_unsafe(vec_p, index, src, len);            \
		}                                                                     \
                                                                              \
		size_t src_index = (size_t)(src - vec->elem);                         \
		if (src_index == index) {                                             \
			return BC_VEC_SUCCESS;                                            \
		}                                                                     \
                                                                              \
		size_t src_end = src_index + len;                                     \
		if (src_end <= index) {                                               \
			return api##_overwrite_unsafe(vec_p, index, src, len);            \
		}                                                                     \
                                                                              \
		size_t dest_index = index;                                            \
		size_t dest_end = dest_index + len;                                   \
		if (src_index >= dest_end) {                                          \
			return api##_overwrite_unsafe(vec_p, index, src, len);            \
		} else if (src_index < index) {                                       \
			update_range(vec, src_index, index);                              \
		} else {                                                              \
			update_range(vec, dest_end, src_end);                             \
		}                                                                     \
                                                                              \
		memmove(&vec->elem[dest_index], src, len * sizeof(*src));             \
                                                                              \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	static inline void perform_splice_inplace(                                \
		api *vec, size_t dest_index, size_t delete_len, size_t src_start,     \
		size_t insert_len)                                                    \
	{                                                                         \
		size_t delete_start = dest_index;                                     \
		size_t delete_end = delete_start + delete_len;                        \
		size_t src_end = src_start + insert_len;                              \
		if (src_end <= delete_start || delete_end <= src_start) {             \
			destroy_range(vec, delete_start, delete_end);                     \
			update_range(vec, src_start, src_end);                            \
		} else if (src_start < delete_start) {                                \
			if (src_end < delete_end) {                                       \
				destroy_range(vec, src_end, delete_end);                      \
			}                                                                 \
			update_range(vec, src_start, delete_start);                       \
		} else if (src_start > delete_start) {                                \
			if (src_end > delete_end) {                                       \
				update_range(vec, delete_end, src_end);                       \
			}                                                                 \
			destroy_range(vec, delete_start, src_start);                      \
		} else if (src_end > delete_end) {                                    \
			update_range(vec, delete_end, src_end);                           \
		} else {                                                              \
			destroy_range(vec, src_end, delete_end);                          \
		}                                                                     \
                                                                              \
		shift_tail(vec, dest_index + insert_len, dest_index + delete_len);    \
		shift_span(vec, dest_index, src_start, insert_len);                   \
		vec->len += insert_len - delete_len;                                  \
	}                                                                         \
                                                                              \
	static inline int splice_shrink_inplace(                                  \
		api **vec_p, size_t index, size_t delete_len, size_t src_index,       \
		size_t insert_len)                                                    \
	{                                                                         \
		perform_splice_inplace(                                               \
			*vec_p, index, delete_len, src_index, insert_len);                \
		return request_shrink(vec_p);                                         \
	}                                                                         \
                                                                              \
	static inline int splice_grow_inplace(                                    \
		api **vec_p, size_t index, size_t delete_len, size_t src_index,       \
		size_t insert_len)                                                    \
	{                                                                         \
		int retval = api##_grow(vec_p, insert_len - delete_len);              \
		if (retval) {                                                         \
			return retval;                                                    \
		}                                                                     \
		perform_splice_inplace(                                               \
			*vec_p, index, delete_len, src_index, insert_len);                \
		return BC_VEC_SUCCESS;                                                \
	}                                                                         \
                                                                              \
	int api##_splice(                                                         \
		api **vec_p, size_t index, size_t delete_len, const type *src,        \
		size_t insert_len)                                                    \
	{                                                                         \
		if (!delete_len) {                                                    \
			return api##_insert(vec_p, index, src, insert_len);               \
		} else if (!insert_len) {                                             \
			return api##_delete(vec_p, index, delete_len);                    \
		} else if (delete_len == insert_len) {                                \
			return api##_overwrite(vec_p, index, src, insert_len);            \
		}                                                                     \
                                                                              \
		api *vec = *vec_p;                                                    \
		if (index > vec->len) {                                               \
			return BC_VEC_E_BOUNDS;                                           \
		} else if (vec->len - index < delete_len) {                           \
			return BC_VEC_E_UNDERFLOW;                                        \
		} else if (is_not_inplace(vec, src)) {                                \
			return api##_splice_unsafe(                                       \
				vec_p, index, delete_len, src, insert_len);                   \
		}                                                                     \
                                                                              \
		size_t src_index = (size_t)(src - vec->elem);                         \
		if (delete_len > insert_len) {                                        \
			return splice_shrink_inplace(                                     \
				vec_p, index, delete_len, src_index, insert_len);             \
		}                                                                     \
		return splice_grow_inplace(                                           \
			vec_p, index, delete_len, src_index, insert_len);                 \
	}

#endif
