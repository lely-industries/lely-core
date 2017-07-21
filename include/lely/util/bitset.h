/*!\file
 * This header file is part of the utilities library; it contains the bitset
 * declarations.
 *
 * \copyright 2017 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LELY_UTIL_BITSET_H
#define LELY_UTIL_BITSET_H

#include <lely/util/util.h>

//! A variable-sized bitset.
struct bitset {
	//! The number of integers in #bits.
	int size;
	//! An array of #size integers.
	unsigned int *bits;
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initializes a bitset. On success, all bits are cleared.
 *
 * \param set  a pointer to a bitset.
 * \param size the requested size (in number of bits) of the set. This number is
 *             rounded up to the nearest multiple of `sizeof(int) * CHAR_BIT`.
 *
 * \returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errnum().
 *
 * \see bitset_fini()
 */
LELY_UTIL_EXTERN int bitset_init(struct bitset *set, int size);

//! Finalizes a bitset. \see bitset_init().
LELY_UTIL_EXTERN void bitset_fini(struct bitset *set);

//! Returns the size (in number of bits) of \a set.
LELY_UTIL_EXTERN int bitset_size(const struct bitset *set);

/*!
 * Resizes a bitset. On success, if the set has grown, all new bits are cleared.
 *
 * \param set  a pointer to a bitset.
 * \param size the requester size (in number of bits) of the set. This number is
 *             rounded up to the nearest multiple of `sizeof(int) * CHAR_BIT`.
 *
 * \returns the new size (in number of bits) of the bitset, or 0 on error. In
 * the latter case, the error number can be obtained with get_errnum().
 */
LELY_UTIL_EXTERN int bitset_resize(struct bitset *set, int size);

//! Returns 1 if bit \a n in \a set is set, and 0 otherwise.
LELY_UTIL_EXTERN int bitset_test(const struct bitset *set, int n);

//! Sets bit \a n in \a set.
LELY_UTIL_EXTERN void bitset_set(struct bitset *set, int n);

//! Sets all bits in \a set.
LELY_UTIL_EXTERN void bitset_set_all(struct bitset *set);

//! Clears bit \a n in \a set.
LELY_UTIL_EXTERN void bitset_clr(struct bitset *set, int n);

//! Clears all bits in \a set.
LELY_UTIL_EXTERN void bitset_clr_all(struct bitset *set);

//! Flip all bits in \a set.
LELY_UTIL_EXTERN void bitset_compl(struct bitset *set);

/*!
 * Returns the index (starting from one) of the first set bit in \a set, or 0 if
 * all bits are zero.
 */
LELY_UTIL_EXTERN int bitset_ffs(const struct bitset *set);

/*!
 * Returns the index (starting from one) of the first zero bit in \a set, or 0
 * if all bits are set.
 */
LELY_UTIL_EXTERN int bitset_ffz(const struct bitset *set);

/*!
 * Returns the index (starting from one) of the first set bit higher or equal to
 * \a n in \a set, or 0 if all bits starting from \a n are zero.
 */
LELY_UTIL_EXTERN int bitset_fns(const struct bitset *set, int n);

/*!
 * Returns the index (starting from one) of the first zero bit higher or equal
 * to \a n in \a set, or 0 if all bits starting from \a n are set.
 */
LELY_UTIL_EXTERN int bitset_fnz(const struct bitset *set, int n);

#ifdef __cplusplus
}
#endif

#endif

