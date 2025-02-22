#pragma once
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/** AVP manipulation and search API
 *
 * @file src/lib/util/dpair.h
 *
 * @copyright 2020 The FreeRADIUS server project
 */
RCSIDH(dpair_h, "$Id$")

#include <freeradius-devel/build.h>
#include <freeradius-devel/missing.h>
#include <freeradius-devel/util/dcursor.h>
#include <freeradius-devel/util/value.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *	Allow public and private versions of the same structures
 */
#ifdef _CONST
#  error _CONST can only be defined in the local header
#endif
#ifndef _PAIR_PRIVATE
#  define _CONST const
#else
#  define _CONST
#endif

typedef struct value_pair_s fr_pair_t;

FR_DLIST_TYPES(pair)

typedef struct {
        FR_DLIST_HEAD_TYPE(pair)		order;			//!< Maintains the relative order of pairs in a list.
} fr_pair_list_t;

/** Stores an attribute, a value and various bits of other data
 *
 * fr_pair_ts are the main data structure used in the server
 *
 * They also specify what behaviour should be used when the attribute is merged into a new list/tree.
 */
struct value_pair_s {
	fr_dict_attr_t const * _CONST da;			//!< Dictionary attribute defines the attribute
								//!< number, vendor and type of the pair.
								///< Note: This should not be modified outside
								///< of pair.c except via #fr_pair_reinit_from_da.

	FR_DLIST_ENTRY_TYPE(pair) _CONST	order_entry;	//!< Entry to maintain relative order within a list
								///< of pairs.  This ensures pairs within the list
								///< are encoded in the same order as they were
								///< received or inserted.


	/*
	 *	Pairs can have children or data but not both.
	 */
	union {
		fr_value_box_t		data;			//!< The value of this pair.
		fr_pair_list_t		children;		//!< Nested attributes of this pair.
	};

	/*
	 *	Legacy stuff that needs to die.
	 */
	struct {
		fr_token_t		op;			//!< Operator to use when moving or inserting
								//!< valuepair into a list.
		char const 		*xlat;			//!< Source string for xlat expansion.
	};
};

/** A fr_pair_t in string format.
 *
 * Used to represent pairs in the legacy 'users' file format.
 */
typedef struct {
	char l_opand[256];					//!< Left hand side of the pair.
	char r_opand[1024];					//!< Right hand side of the pair.

	fr_token_t quote;					//!< Type of quoting around the r_opand.

	fr_token_t op;						//!< Operator.
} fr_pair_t_RAW;

#define vp_strvalue		data.vb_strvalue
#define vp_octets		data.vb_octets
#define vp_ptr			data.datum.ptr			//!< Either octets or strvalue
#define vp_length		data.vb_length

#define vp_ipv4addr		data.vb_ip.addr.v4.s_addr
#define vp_ipv6addr		data.vb_ip.addr.v6.s6_addr
#define vp_ip			data.vb_ip
#define vp_ifid			data.vb_ifid
#define vp_ether		data.vb_ether

#define vp_bool			data.datum.boolean
#define vp_uint8		data.vb_uint8
#define vp_uint16		data.vb_uint16
#define vp_uint32		data.vb_uint32
#define vp_uint64		data.vb_uint64

#define vp_int8			data.vb_int8
#define vp_int16		data.vb_int16
#define vp_int32		data.vb_int32
#define vp_int64		data.vb_int64

#define vp_float32		data.vb_float32
#define vp_float64		data.vb_float64

#define vp_date			data.vb_date
#define vp_time_delta		data.vb_time_delta

#define vp_group		children

#define vp_size			data.datum.size
#define vp_filter		data.datum.filter

#define vp_type			data.type
#define vp_tainted		data.tainted

#define ATTRIBUTE_EQ(_x, _y) ((_x && _y) && (_x->da == _y->da))

/** If WITH_VERIFY_PTR is defined, we perform runtime checks to ensure the fr_pair_t are sane
 *
 */
#ifdef WITH_VERIFY_PTR
void		fr_pair_verify(char const *file, int line, fr_pair_t const *vp) CC_HINT(nonnull(3));

void		fr_pair_list_verify(char const *file, int line,
				    TALLOC_CTX const *expected, fr_pair_list_t const *list) CC_HINT(nonnull(4));

#  define PAIR_VERIFY(_x)		fr_pair_verify(__FILE__, __LINE__, _x)
#  define PAIR_LIST_VERIFY(_x)	fr_pair_list_verify(__FILE__, __LINE__, NULL, _x)
#else
DIAG_OFF(nonnull-compare)
/** Wrapper function to defeat nonnull checks
 *
 * We may sprinkle PAIR_VERIFY and PAIR_LIST_VERIFY in functions which
 * have their pair argument marked up as nonnull.
 *
 * This would usually generate errors when WITH_VERIFY_PTR is not
 * defined, as the assert macros check for an arguments NULLness.
 *
 * This function wraps the assert but has nonnull-compare disabled
 * meaning a warning won't be emitted.
 */
static inline bool fr_pair_nonnull_assert(fr_pair_t const *vp)
{
	return fr_cond_assert(vp);
}

static inline bool fr_pair_list_nonnull_assert(fr_pair_list_t const *pair_list)
{
	return fr_cond_assert(pair_list);
}
DIAG_ON(nonnull-compare)

/*
 *	Even if were building without WITH_VERIFY_PTR
 *	the pointer must not be NULL when these various macros are used
 *	so we can add some sneaky soft asserts.
 */
#  define PAIR_VERIFY(_x)		fr_pair_nonnull_assert(_x)
#  define PAIR_LIST_VERIFY(_x)	fr_pair_list_nonnull_assert(_x)
#endif

/* Initialisation */
void fr_pair_list_init(fr_pair_list_t *head) CC_HINT(nonnull);

/*
 *  Temporary macro to point the head of a pair_list to a specific vp
 */
#define fr_pair_list_set_head(_list, _vp) (_list = &_vp)

#define fr_pair_list_from_dcursor(_cursor) (fr_pair_list_t *) (((uint8_t *) (_cursor->dlist)) - offsetof(fr_pair_list_t, order))

/* Allocation and management */
fr_pair_t	*fr_pair_alloc_null(TALLOC_CTX *ctx) CC_HINT(warn_unused_result);

fr_pair_list_t	*fr_pair_list_alloc(TALLOC_CTX *ctx) CC_HINT(warn_unused_result);

fr_pair_t	*fr_pair_root_afrom_da(TALLOC_CTX *ctx, fr_dict_attr_t const *da) CC_HINT(warn_unused_result) CC_HINT(nonnull(2));

/** @hidecallergraph */
fr_pair_t	*fr_pair_afrom_da(TALLOC_CTX *ctx, fr_dict_attr_t const *da) CC_HINT(warn_unused_result) CC_HINT(nonnull(2));

fr_pair_t	*fr_pair_afrom_da_with_pool(TALLOC_CTX *ctx, fr_dict_attr_t const *da, size_t value_len)
		CC_HINT(warn_unused_result) CC_HINT(nonnull(2));

int		fr_pair_reinit_from_da(fr_pair_list_t *list, fr_pair_t *vp, fr_dict_attr_t const *da)
		CC_HINT(nonnull(2, 3));

fr_pair_t	*fr_pair_afrom_child_num(TALLOC_CTX *ctx, fr_dict_attr_t const *parent, unsigned int attr) CC_HINT(warn_unused_result);

fr_pair_t	*fr_pair_copy(TALLOC_CTX *ctx, fr_pair_t const *vp) CC_HINT(nonnull(2)) CC_HINT(warn_unused_result);

int		fr_pair_steal(TALLOC_CTX *ctx, fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_steal_append(TALLOC_CTX *nctx, fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_steal_prepend(TALLOC_CTX *nctx, fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

/** @hidecallergraph */
void		fr_pair_list_free(fr_pair_list_t *list) CC_HINT(nonnull);

/** @hidecallergraph */
bool		fr_pair_list_empty(fr_pair_list_t const *list) CC_HINT(nonnull);

size_t		fr_pair_list_len(fr_pair_list_t const *list) CC_HINT(nonnull);

/* Searching and list modification */
int		fr_pair_to_unknown(fr_pair_t *vp) CC_HINT(nonnull);

void		*fr_pair_iter_next_by_da(fr_dlist_head_t *list,
					 void *to_eval, void *uctx) CC_HINT(nonnull);

void		*fr_pair_iter_next_by_ancestor(fr_dlist_head_t *list,
					       void *to_eval, void *uctx) CC_HINT(nonnull);

bool		fr_pair_matches_da(void const *item, void const *uctx) CC_HINT(nonnull);

/** @hidecallergraph */
unsigned int	fr_pair_count_by_da(fr_pair_list_t const *list, fr_dict_attr_t const *da)
				    CC_HINT(nonnull);

fr_pair_t	*fr_pair_find_by_da(fr_pair_list_t const *list,
				    fr_pair_t const *prev, fr_dict_attr_t const *da) CC_HINT(nonnull(1,3));

fr_pair_t	*fr_pair_find_by_da_idx(fr_pair_list_t const *list,
					fr_dict_attr_t const *da, unsigned int idx) CC_HINT(nonnull);

fr_pair_t	*fr_pair_find_by_ancestor(fr_pair_list_t const *list, fr_pair_t const *prev,
					  fr_dict_attr_t const *ancestor) CC_HINT(nonnull(1,3));

fr_pair_t	*fr_pair_find_by_ancestor_idx(fr_pair_list_t const *list,
					      fr_dict_attr_t const *ancestor, unsigned int idx) CC_HINT(nonnull);

fr_pair_t	*fr_pair_find_by_child_num(fr_pair_list_t const *list, fr_pair_t const *prev,
					   fr_dict_attr_t const *parent, unsigned int attr) CC_HINT(nonnull(1,3));

fr_pair_t	*fr_pair_find_by_child_num_idx(fr_pair_list_t const *list,
					       fr_dict_attr_t const *parent, unsigned int attr,
					       unsigned int idx) CC_HINT(nonnull);

int		fr_pair_append(fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_prepend(fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_insert_after(fr_pair_list_t *list, fr_pair_t *pos, fr_pair_t *to_add) CC_HINT(nonnull(1,3));

int		fr_pair_insert_before(fr_pair_list_t *list, fr_pair_t *pos, fr_pair_t *to_add) CC_HINT(nonnull(1,3));

void		fr_pair_replace(fr_pair_list_t *list, fr_pair_t *to_replace, fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_delete_by_child_num(fr_pair_list_t *list,
					    fr_dict_attr_t const *parent, unsigned int attr) CC_HINT(nonnull);

int		fr_pair_append_by_da(TALLOC_CTX *ctx, fr_pair_t **out, fr_pair_list_t *list,
				     fr_dict_attr_t const *da) CC_HINT(nonnull(3,4));

int		fr_pair_prepend_by_da(TALLOC_CTX *ctx, fr_pair_t **out, fr_pair_list_t *list,
				      fr_dict_attr_t const *da) CC_HINT(nonnull(3,4));

int		fr_pair_update_by_da(TALLOC_CTX *ctx, fr_pair_t **out, fr_pair_list_t *list,
				     fr_dict_attr_t const *da, unsigned int n) CC_HINT(nonnull(3,4));

int		fr_pair_delete_by_da(fr_pair_list_t *head, fr_dict_attr_t const *da) CC_HINT(nonnull);

fr_pair_t	*fr_pair_remove(fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

fr_pair_t	*fr_pair_delete(fr_pair_list_t *list, fr_pair_t *vp) CC_HINT(nonnull);

/* functions for FR_TYPE_STRUCTURAL */
fr_pair_list_t	*fr_pair_children(fr_pair_t *head) CC_HINT(nonnull);

/** Initialises a special dcursor with callbacks that will maintain the attr sublists correctly
 *
 * Filters can be applied later with fr_dcursor_filter_set.
 *
 * @note This is the only way to use a dcursor in non-const mode with fr_pair_list_t.
 *
 * @param[out] cursor	to initialise.
 * @param[in] list	to iterate over.
 * @param[in] iter	Iterator to use when filtering pairs.
 * @param[in] uctx	To pass to iterator.
 * @return
 *	- NULL if src does not point to any items.
 *	- The first pair in the list.
 */
#define		fr_pair_dcursor_iter_init(_cursor, _list, _iter, _uctx) \
		_fr_pair_dcursor_iter_init(_cursor, \
					   _list, \
					   _iter, \
					   _uctx, \
					   IS_CONST(fr_pair_list_t *, _list))
fr_pair_t	*_fr_pair_dcursor_iter_init(fr_dcursor_t *cursor, fr_pair_list_t const *list,
					    fr_dcursor_iter_t iter, void const *uctx,
					    bool is_const) CC_HINT(nonnull);

/** Initialises a special dcursor with callbacks that will maintain the attr sublists correctly
 *
 * Filters can be applied later with fr_dcursor_filter_set.
 *
 * @note This is the only way to use a dcursor in non-const mode with fr_pair_list_t.
 *
 * @param[out] cursor	to initialise.
 * @param[in] list	to iterate over.
 * @return
 *	- NULL if src does not point to any items.
 *	- The first pair in the list.
 */
#define		fr_pair_dcursor_init(_cursor, _list) \
		_fr_pair_dcursor_init(_cursor, \
				      _list, \
				      IS_CONST(fr_pair_list_t *, _list))
fr_pair_t	*_fr_pair_dcursor_init(fr_dcursor_t *cursor, fr_pair_list_t const *list,
				       bool is_const) CC_HINT(nonnull);

/** Initialise a cursor that will return only attributes matching the specified #fr_dict_attr_t
 *
 * @param[in] cursor	to initialise.
 * @param[in] list	to iterate over.
 * @param[in] da	to search for.
 * @return
 *	- The first matching pair.
 *	- NULL if no pairs match.
 */
#define		fr_pair_dcursor_by_da_init(_cursor, _list, _da) \
		_fr_pair_dcursor_by_da_init(_cursor, \
					    _list, \
					    _da, \
					    IS_CONST(fr_pair_list_t *, _list))
fr_pair_t	*_fr_pair_dcursor_by_da_init(fr_dcursor_t *cursor,
					     fr_pair_list_t const *list, fr_dict_attr_t const *da,
					     bool is_const) CC_HINT(nonnull);

/** Initialise a cursor that will return only attributes descended from the specified #fr_dict_attr_t
 *
 * @param[in] cursor	to initialise.
 * @param[in] list	to iterate over.
 * @param[in] da	who's decentness to search for.
 * @return
 *	- The first matching pair.
 *	- NULL if no pairs match.
 */
#define		fr_pair_dcursor_by_ancestor_init(_cursor, _list, _da) \
		_fr_pair_dcursor_by_ancestor_init(_cursor, \
						  _list, \
						  _da, \
						  IS_CONST(fr_pair_list_t *, _list))
fr_pair_t	*_fr_pair_dcursor_by_ancestor_init(fr_dcursor_t *cursor,
						   fr_pair_list_t const *list, fr_dict_attr_t const *da,
						   bool is_const) CC_HINT(nonnull);

/** Compare two attributes using and operator.
 *
 * @return
 *	- 1 if equal.
 *	- 0 if not equal.
 *	- -1 on failure.
 */
#define		fr_pair_cmp_op(_op, _a, _b)	fr_value_box_cmp_op(_op, &_a->data, &_b->data)

int8_t		fr_pair_cmp_by_da(void const *a, void const *b);

int8_t		fr_pair_cmp_by_parent_num(void const *a, void const *b);

int		fr_pair_cmp(fr_pair_t const *a, fr_pair_t const *b);

int		fr_pair_list_cmp(fr_pair_list_t const *a, fr_pair_list_t const *b) CC_HINT(nonnull);

void		fr_pair_list_sort(fr_pair_list_t *list, fr_cmp_t cmp) CC_HINT(nonnull);

/* Filtering */
void		fr_pair_validate_debug(TALLOC_CTX *ctx, fr_pair_t const *failed[2]) CC_HINT(nonnull(2));

bool		fr_pair_validate(fr_pair_t const *failed[2], fr_pair_list_t *filter,
				 fr_pair_list_t *list) CC_HINT(nonnull(2,3));

bool 		fr_pair_validate_relaxed(fr_pair_t const *failed[2], fr_pair_list_t *filter,
					 fr_pair_list_t *list) CC_HINT(nonnull(2,3));

/* Lists */
int		fr_pair_list_copy(TALLOC_CTX *ctx, fr_pair_list_t *to, fr_pair_list_t const *from);

int		fr_pair_list_copy_by_da(TALLOC_CTX *ctx, fr_pair_list_t *to,
					fr_pair_list_t const *from, fr_dict_attr_t const *da, unsigned int count);

int		fr_pair_list_copy_by_ancestor(TALLOC_CTX *ctx, fr_pair_list_t *to,
					      fr_pair_list_t const *from,
					      fr_dict_attr_t const *parent_da, unsigned int count) CC_HINT(nonnull);

int		fr_pair_sublist_copy(TALLOC_CTX *ctx, fr_pair_list_t *to,
				     fr_pair_list_t const *from,
				     fr_pair_t const *start, unsigned int count) CC_HINT(nonnull(2,3));

void		fr_pair_list_append(fr_pair_list_t *dst, fr_pair_list_t *src) CC_HINT(nonnull);

void		fr_pair_list_prepend(fr_pair_list_t *dst, fr_pair_list_t *src) CC_HINT(nonnull);

/** @hidecallergraph */
fr_pair_t	*fr_pair_list_head(fr_pair_list_t const *list) CC_HINT(nonnull);

/** @hidecallergraph */
fr_pair_t      	*fr_pair_list_next(fr_pair_list_t const *list, fr_pair_t const *item) CC_HINT(nonnull(1));

fr_pair_t      	*fr_pair_list_prev(fr_pair_list_t const *list, fr_pair_t const *item) CC_HINT(nonnull(1));

fr_pair_t      	*fr_pair_list_tail(fr_pair_list_t const *list) CC_HINT(nonnull);

/** @name Pair to pair copying
 *
 * @{
 */
void		fr_pair_value_clear(fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_value_copy(fr_pair_t *dst, fr_pair_t *src) CC_HINT(nonnull);
/** @} */

/** @name Assign and manipulate binary-unsafe C strings
 *
 * @{
 */
int		fr_pair_value_from_str(fr_pair_t *vp,
				       char const *value, size_t len, fr_sbuff_unescape_rules_t const *erules,
				       bool tainted) CC_HINT(nonnull(1,2));

int		fr_pair_value_strdup(fr_pair_t *vp, char const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_strdup_shallow(fr_pair_t *vp, char const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_strtrim(fr_pair_t *vp) CC_HINT(nonnull);

int		fr_pair_value_aprintf(fr_pair_t *vp,
				      char const *fmt, ...) CC_HINT(nonnull) CC_HINT(format (printf, 2, 3));
/** @} */

/** @name Assign and manipulate binary-safe strings
 *
 * @{
 */
int		fr_pair_value_bstr_alloc(fr_pair_t *vp, char **out, size_t size, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_bstr_realloc(fr_pair_t *vp, char **out, size_t size) CC_HINT(nonnull(1));

int		fr_pair_value_bstrndup(fr_pair_t *vp, char const *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_bstrdup_buffer(fr_pair_t *vp, char const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_bstrndup_shallow(fr_pair_t *vp, char const *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_bstrdup_buffer_shallow(fr_pair_t *vp, char const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_bstrn_append(fr_pair_t *vp, char const *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_bstr_append_buffer(fr_pair_t *vp, char const *src, bool tainted) CC_HINT(nonnull);
 /** @} */

/** @name Assign and manipulate octets strings
 *
 * @{
 */
int		fr_pair_value_mem_alloc(fr_pair_t *vp, uint8_t **out, size_t size, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_mem_realloc(fr_pair_t *vp, uint8_t **out, size_t size) CC_HINT(nonnull(1));

int		fr_pair_value_memdup(fr_pair_t *vp, uint8_t const *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_memdup_buffer(fr_pair_t *vp, uint8_t const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_memdup_shallow(fr_pair_t *vp, uint8_t const *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_memdup_buffer_shallow(fr_pair_t *vp, uint8_t const *src, bool tainted) CC_HINT(nonnull);

int		fr_pair_value_mem_append(fr_pair_t *vp, uint8_t *src, size_t len, bool tainted) CC_HINT(nonnull(1));

int		fr_pair_value_mem_append_buffer(fr_pair_t *vp, uint8_t *src, bool tainted) CC_HINT(nonnull);
 /** @} */

/** @name Enum functions
 *
 * @{
 */
char const	*fr_pair_value_enum(fr_pair_t const *vp, char buff[static 20]) CC_HINT(nonnull);

int		fr_pair_value_enum_box(fr_value_box_t const **out, fr_pair_t *vp) CC_HINT(nonnull);
/** @} */

/** @name Printing functions
 *
 * @{
 */
ssize_t   	fr_pair_print_value_quoted(fr_sbuff_t *out,
					   fr_pair_t const *vp, fr_token_t quote) CC_HINT(nonnull);

static inline size_t CC_HINT(nonnull(2,3))
		fr_pair_aprint_value_quoted(TALLOC_CTX *ctx, char **out,
					    fr_pair_t const *vp, fr_token_t quote)
{
		SBUFF_OUT_TALLOC_FUNC_NO_LEN_DEF(fr_pair_print_value_quoted, vp, quote)
}

ssize_t		fr_pair_print(fr_sbuff_t *out, fr_pair_t const *parent,
			      fr_pair_t const *vp) CC_HINT(nonnull(1,3));

static inline size_t CC_HINT(nonnull(2,4))
		fr_pair_aprint(TALLOC_CTX *ctx, char **out, fr_pair_t const *parent, fr_pair_t const *vp)
{
		SBUFF_OUT_TALLOC_FUNC_NO_LEN_DEF(fr_pair_print, parent, vp)
}

void		fr_pair_fprint(FILE *, fr_pair_t const *vp) CC_HINT(nonnull);

#define		fr_pair_list_log(_log, _list) _fr_pair_list_log(_log, 4, NULL, _list, __FILE__, __LINE__);
void		_fr_pair_list_log(fr_log_t const *log, int lvl, fr_pair_t *parent,
				  fr_pair_list_t const *list, char const *file, int line) CC_HINT(nonnull(1,4));

void		fr_pair_list_debug(fr_pair_list_t const *list) CC_HINT(nonnull);
void		fr_pair_debug(fr_pair_t const *pair) CC_HINT(nonnull);

/** @} */

void		fr_pair_list_tainted(fr_pair_list_t *vps) CC_HINT(nonnull);

void		fr_pair_list_afrom_box(TALLOC_CTX *ctx, fr_pair_list_t *out,
				       fr_dict_t const *dict, fr_value_box_t *box) CC_HINT(nonnull);

/* Tokenization */
typedef struct {
	TALLOC_CTX		*ctx;			//!< to allocate VPs in
	fr_dict_attr_t	const	*parent;	       	//!< current attribute to allocate VPs in
	fr_pair_list_t		*list;			//!< of VPs to add
} fr_pair_ctx_t;

ssize_t		fr_pair_ctx_afrom_str(fr_pair_ctx_t *pair_ctx, char const *in, size_t inlen) CC_HINT(nonnull);
void		fr_pair_ctx_reset(fr_pair_ctx_t *pair_ctx, fr_dict_t const *dict) CC_HINT(nonnull);

#undef _CONST
#ifdef __cplusplus
}
#endif
