/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_PRIVATE_HH
#define HB_PRIVATE_HH

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "hb-common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* We only use these two for debug output.  However, the debug code is
 * always seen by the compiler (and optimized out in non-debug builds.
 * If including these becomes a problem, we can start thinking about
 * someway around that. */
#include <stdio.h>
#include <errno.h>

HB_BEGIN_DECLS


/* Essentials */

#ifndef NULL
# define NULL ((void *) 0)
#endif

#undef FALSE
#define FALSE 0

#undef TRUE
#define TRUE 1


/* Basics */

HB_END_DECLS

#undef MIN
template <typename Type> static inline Type MIN (const Type &a, const Type &b) { return a < b ? a : b; }

#undef MAX
template <typename Type> static inline Type MAX (const Type &a, const Type &b) { return a > b ? a : b; }

HB_BEGIN_DECLS

#undef  ARRAY_LENGTH
#define ARRAY_LENGTH(__array) ((signed int) (sizeof (__array) / sizeof (__array[0])))

#define HB_STMT_START do
#define HB_STMT_END   while (0)

#define _ASSERT_STATIC1(_line, _cond) typedef int _static_assert_on_line_##_line##_failed[(_cond)?1:-1]
#define _ASSERT_STATIC0(_line, _cond) _ASSERT_STATIC1 (_line, (_cond))
#define ASSERT_STATIC(_cond) _ASSERT_STATIC0 (__LINE__, (_cond))

#define ASSERT_STATIC_EXPR(_cond) ((void) sizeof (char[(_cond) ? 1 : -1]))


/* Lets assert int types.  Saves trouble down the road. */

ASSERT_STATIC (sizeof (int8_t) == 1);
ASSERT_STATIC (sizeof (uint8_t) == 1);
ASSERT_STATIC (sizeof (int16_t) == 2);
ASSERT_STATIC (sizeof (uint16_t) == 2);
ASSERT_STATIC (sizeof (int32_t) == 4);
ASSERT_STATIC (sizeof (uint32_t) == 4);
ASSERT_STATIC (sizeof (int64_t) == 8);
ASSERT_STATIC (sizeof (uint64_t) == 8);

ASSERT_STATIC (sizeof (hb_codepoint_t) == 4);
ASSERT_STATIC (sizeof (hb_position_t) == 4);
ASSERT_STATIC (sizeof (hb_mask_t) == 4);
ASSERT_STATIC (sizeof (hb_var_int_t) == 4);

/* Misc */


#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _HB_BOOLEAN_EXPR(expr) ((expr) ? 1 : 0)
#define likely(expr) (__builtin_expect (_HB_BOOLEAN_EXPR(expr), 1))
#define unlikely(expr) (__builtin_expect (_HB_BOOLEAN_EXPR(expr), 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

#ifndef __GNUC__
#undef __attribute__
#define __attribute__(x)
#endif

#if __GNUC__ >= 3
#define HB_PURE_FUNC	__attribute__((pure))
#define HB_CONST_FUNC	__attribute__((const))
#else
#define HB_PURE_FUNC
#define HB_CONST_FUNC
#endif
#if __GNUC__ >= 4
#define HB_UNUSED	__attribute__((unused))
#else
#define HB_UNUSED
#endif

#ifndef HB_INTERNAL
# define HB_INTERNAL __attribute__((__visibility__("hidden")))
#endif


#if (defined(__WIN32__) && !defined(__WINE__)) || defined(_MSC_VER)
#define snprintf _snprintf
#endif

#ifdef _MSC_VER
#undef inline
#define inline __inline
#endif

#ifdef __STRICT_ANSI__
#undef inline
#define inline __inline__
#endif


#if __GNUC__ >= 3
#define HB_FUNC __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define HB_FUNC __FUNCSIG__
#else
#define HB_FUNC __func__
#endif


/* Return the number of 1 bits in mask. */
static inline HB_CONST_FUNC unsigned int
_hb_popcount32 (uint32_t mask)
{
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
  return __builtin_popcount (mask);
#else
  /* "HACKMEM 169" */
  register uint32_t y;
  y = (mask >> 1) &033333333333;
  y = mask - y - ((y >>1) & 033333333333);
  return (((y + (y >> 3)) & 030707070707) % 077);
#endif
}

/* Returns the number of bits needed to store number */
static inline HB_CONST_FUNC unsigned int
_hb_bit_storage (unsigned int number)
{
#if defined(__GNUC__) && (__GNUC__ >= 4) && defined(__OPTIMIZE__)
  return likely (number) ? (sizeof (unsigned int) * 8 - __builtin_clz (number)) : 0;
#else
  register unsigned int n_bits = 0;
  while (number) {
    n_bits++;
    number >>= 1;
  }
  return n_bits;
#endif
}

/* Returns the number of zero bits in the least significant side of number */
static inline HB_CONST_FUNC unsigned int
_hb_ctz (unsigned int number)
{
#if defined(__GNUC__) && (__GNUC__ >= 4) && defined(__OPTIMIZE__)
  return likely (number) ? __builtin_ctz (number) : 0;
#else
  register unsigned int n_bits = 0;
  if (unlikely (!number)) return 0;
  while (!(number & 1)) {
    n_bits++;
    number >>= 1;
  }
  return n_bits;
#endif
}

static inline bool
_hb_unsigned_int_mul_overflows (unsigned int count, unsigned int size)
{
  return (size > 0) && (count >= ((unsigned int) -1) / size);
}


/* Type of bsearch() / qsort() compare function */
typedef int (*hb_compare_func_t) (const void *, const void *);


HB_END_DECLS


/* arrays and maps */


template <typename Type, unsigned int StaticSize>
struct hb_prealloced_array_t {

  unsigned int len;
  unsigned int allocated;
  Type *array;
  Type static_array[StaticSize];

  inline Type& operator [] (unsigned int i) { return array[i]; }
  inline const Type& operator [] (unsigned int i) const { return array[i]; }

  inline Type *push (void)
  {
    if (!array) {
      array = static_array;
      allocated = ARRAY_LENGTH (static_array);
    }
    if (likely (len < allocated))
      return &array[len++];

    /* Need to reallocate */
    unsigned int new_allocated = allocated + (allocated >> 1) + 8;
    Type *new_array = NULL;

    if (array == static_array) {
      new_array = (Type *) calloc (new_allocated, sizeof (Type));
      if (new_array)
        memcpy (new_array, array, len * sizeof (Type));
    } else {
      bool overflows = (new_allocated < allocated) || _hb_unsigned_int_mul_overflows (new_allocated, sizeof (Type));
      if (likely (!overflows)) {
	new_array = (Type *) realloc (array, new_allocated * sizeof (Type));
      }
    }

    if (unlikely (!new_array))
      return NULL;

    array = new_array;
    allocated = new_allocated;
    return &array[len++];
  }

  inline void pop (void)
  {
    len--;
    /* TODO: shrink array if needed */
  }

  inline void shrink (unsigned int l)
  {
     if (l < len)
       len = l;
    /* TODO: shrink array if needed */
  }

  template <typename T>
  inline Type *find (T v) {
    for (unsigned int i = 0; i < len; i++)
      if (array[i] == v)
	return &array[i];
    return NULL;
  }
  template <typename T>
  inline const Type *find (T v) const {
    for (unsigned int i = 0; i < len; i++)
      if (array[i] == v)
	return &array[i];
    return NULL;
  }

  inline void sort (void)
  {
    qsort (array, len, sizeof (Type), (hb_compare_func_t) Type::cmp);
  }

  template <typename T>
  inline Type *bsearch (T *key)
  {
    return (Type *) ::bsearch (key, array, len, sizeof (Type), (hb_compare_func_t) Type::cmp);
  }
  template <typename T>
  inline const Type *bsearch (T *key) const
  {
    return (const Type *) ::bsearch (key, array, len, sizeof (Type), (hb_compare_func_t) Type::cmp);
  }

  inline void finish (void)
  {
    if (array != static_array)
      free (array);
    array = NULL;
    allocated = len = 0;
  }
};

template <typename Type>
struct hb_array_t : hb_prealloced_array_t<Type, 2> {};


template <typename item_t, typename lock_t>
struct hb_lockable_set_t
{
  hb_array_t <item_t> items;

  template <typename T>
  inline item_t *replace_or_insert (T v, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item) {
      item_t old = *item;
      *item = v;
      l.unlock ();
      old.finish ();
    } else {
      item = items.push ();
      if (likely (item))
	*item = v;
      l.unlock ();
    }
    return item;
  }

  template <typename T>
  inline void remove (T v, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item) {
      item_t old = *item;
      *item = items[items.len - 1];
      items.pop ();
      l.unlock ();
      old.finish ();
    } else {
      l.unlock ();
    }
  }

  template <typename T>
  inline bool find (T v, item_t *i, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (item)
      *i = *item;
    l.unlock ();
    return !!item;
  }

  template <typename T>
  inline item_t *find_or_insert (T v, lock_t &l)
  {
    l.lock ();
    item_t *item = items.find (v);
    if (!item) {
      item = items.push ();
      if (likely (item))
        *item = v;
    }
    l.unlock ();
    return item;
  }

  inline void finish (lock_t &l)
  {
    l.lock ();
    while (items.len) {
      item_t old = items[items.len - 1];
	items.pop ();
	l.unlock ();
	old.finish ();
	l.lock ();
    }
    items.finish ();
    l.unlock ();
  }

};


HB_BEGIN_DECLS


/* Big-endian handling */

static inline uint16_t hb_be_uint16 (const uint16_t v)
{
  const uint8_t *V = (const uint8_t *) &v;
  return (uint16_t) (V[0] << 8) + V[1];
}

#define hb_be_uint16_put(v,V)	HB_STMT_START { v[0] = (V>>8); v[1] = (V); } HB_STMT_END
#define hb_be_uint16_get(v)	(uint16_t) ((v[0] << 8) + v[1])
#define hb_be_uint16_eq(a,b)	(a[0] == b[0] && a[1] == b[1])

#define hb_be_uint32_put(v,V)	HB_STMT_START { v[0] = (V>>24); v[1] = (V>>16); v[2] = (V>>8); v[3] = (V); } HB_STMT_END
#define hb_be_uint32_get(v)	(uint32_t) ((v[0] << 24) + (v[1] << 16) + (v[2] << 8) + v[3])
#define hb_be_uint32_eq(a,b)	(a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3])


/* ASCII tag/character handling */

static inline unsigned char ISALPHA (unsigned char c)
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline unsigned char ISALNUM (unsigned char c)
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
static inline unsigned char TOUPPER (unsigned char c)
{ return (c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c; }
static inline unsigned char TOLOWER (unsigned char c)
{ return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c; }

#define HB_TAG_CHAR4(s)   (HB_TAG(((const char *) s)[0], \
				  ((const char *) s)[1], \
				  ((const char *) s)[2], \
				  ((const char *) s)[3]))


/* C++ helpers */

/* Makes class uncopyable.  Use in private: section. */
#define NO_COPY(T) \
  T (const T &o); \
  T &operator = (const T &o);


/* Debug */

#ifndef HB_DEBUG
#define HB_DEBUG 0
#endif

static inline bool /* always returns TRUE */
_hb_trace (const char *what,
	   const char *function,
	   const void *obj,
	   unsigned int depth,
	   unsigned int max_depth)
{
  (void) ((depth < max_depth) && fprintf (stderr, "%s(%p) %-*d-> %s\n", what, obj, depth, depth, function));
  return TRUE;
}


HB_END_DECLS

#endif /* HB_PRIVATE_HH */
