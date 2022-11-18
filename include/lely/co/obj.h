/**@file
 * This header file is part of the CANopen library; it contains the object
 * dictionary declarations.
 *
 * @copyright 2022 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#ifndef LELY_CO_OBJ_H_
#define LELY_CO_OBJ_H_

#include <lely/co/type.h>

#include <stddef.h>

/// An object with no data fields.
#define CO_OBJECT_NULL 0x00

/// A large variable amount of data.
#define CO_OBJECT_DOMAIN 0x02

/// A type definitions.
#define CO_OBJECT_DEFTYPE 0x05

/// A record type definition.
#define CO_OBJECT_DEFSTRUCT 0x06

/// A single value.
#define CO_OBJECT_VAR 0x07

/**
 * A multiple data field object where each data field is a simple variable of
 * the same basic data type.
 */
#define CO_OBJECT_ARRAY 0x08

/**
 * A multiple data field object where the data fields may be any combination of
 * simple variables.
 */
#define CO_OBJECT_RECORD 0x09

/// The object can be read.
#define CO_ACCESS_READ 0x01

/// The object can be written.
#define CO_ACCESS_WRITE 0x02

/// The object can be mapped to a TPDO.
#define CO_ACCESS_TPDO 0x04

/// The object can be mapped to an RPDO.
#define CO_ACCESS_RPDO 0x08

/// Read-only access.
#define CO_ACCESS_RO (CO_ACCESS_READ | CO_ACCESS_TPDO)

/// Write-only access.
#define CO_ACCESS_WO (CO_ACCESS_WRITE | CO_ACCESS_RPDO)

/// Read or write access.
#define CO_ACCESS_RW (CO_ACCESS_RO | CO_ACCESS_WO)

/// Read or write on process input.
#define CO_ACCESS_RWR (CO_ACCESS_RO | CO_ACCESS_WRITE)

/// Read or write on process output.
#define CO_ACCESS_RWW (CO_ACCESS_WO | CO_ACCESS_READ)

/// Constant value.
#define CO_ACCESS_CONST (CO_ACCESS_RO | 0x10)

/// Refuse read on scan.
#define CO_OBJ_FLAGS_READ 0x01

/// Refuse write on download.
#define CO_OBJ_FLAGS_WRITE 0x02

/**
 * If a read access is performed for the object, the data is stored in a file.
 * In this case, the object contains the filename, _not_ the file contents.
 */
#define CO_OBJ_FLAGS_UPLOAD_FILE 0x04

/**
 * If a write access is performed for the object, the data is stored in a file.
 * In this case, the object contains the filename, _not_ the file contents.
 */
#define CO_OBJ_FLAGS_DOWNLOAD_FILE 0x08

/// The lower limit of the object value is of the form `$NODEID { "+" number }`.
#define CO_OBJ_FLAGS_MIN_NODEID 0x10

/// The upper limit of the object value is of the form `$NODEID { "+" number }`.
#define CO_OBJ_FLAGS_MAX_NODEID 0x20

/// The default object value is of the form `$NODEID { "+" number }`.
#define CO_OBJ_FLAGS_DEF_NODEID 0x40

/// The current object value is of the form `$NODEID { "+" number }`.
#define CO_OBJ_FLAGS_VAL_NODEID 0x80

/**
 * The current object value was explicitly set with the `ParamaterValue`
 * attribute in the EDS/DCF file.
 */
#define CO_OBJ_FLAGS_PARAMETER_VALUE 0x100

/// The CANopen SDO upload/download request struct from <lely/co/sdo.h>.
struct co_sdo_req;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a CANopen sub-object download indication function, invoked by an
 * SDO download request or Receive-PDO indication. In case of an SDO request,
 * this function is invoked for each segment. And once before sending the first
 * response to a non-expedited request or before sending a block confirmation,
 * in which case the <b>nbyte</b> member of *<b>req</b> is 0.
 *
 * @param sub  a pointer to a CANopen sub-object.
 * @param req  a pointer to a CANopen SDO download request. The <b>size</b>,
 *             <b>buf</b>, <b>nbyte</b> and <b>offset</b> members of *<b>req</b>
 *             are set by the caller.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
typedef co_unsigned32_t co_sub_dn_ind_t(
		co_sub_t *sub, struct co_sdo_req *req, void *data);

/**
 * The type of a CANopen sub-object upload indication function, invoked by an
 * SDO upload request or Transmit-PDO indication.
 *
 * @param sub  a pointer to a CANopen sub-object, containing the new value.
 * @param req  a pointer to a CANopen SDO upload request. On the first
 *             invocation, the <b>size</b> member of *<b>req</b> is set to 0.
 *             All members MUST be initialized by the indication function.
 * @param data a pointer to user-specified data.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
typedef co_unsigned32_t co_sub_up_ind_t(
		const co_sub_t *sub, struct co_sdo_req *req, void *data);

#if !LELY_NO_MALLOC
void *__co_obj_alloc(void);
void __co_obj_free(void *ptr);
#endif

struct __co_obj *__co_obj_init(struct __co_obj *obj, co_unsigned16_t idx,
		void *val, size_t size);
void __co_obj_fini(struct __co_obj *obj);

#if !LELY_NO_MALLOC

/**
 * Creates a CANopen object.
 *
 * @param idx the object index.
 *
 * @returns a pointer to a new CANopen object, or NULL on error. In the latter
 * case, the error number can be obtained with get_errc().
 *
 * @see co_obj_destroy()
 */
co_obj_t *co_obj_create(co_unsigned16_t idx);

/// Destroys a CANopen object, including its sub-objects. @see co_obj_create()
void co_obj_destroy(co_obj_t *obj);

#endif // !LELY_NO_MALLOC

/**
 * Finds the previous object in the object dictionary of a CANopen device.
 *
 * @returns a pointer to the previous object, or NULL if this is the first
 * object.
 *
 * @see co_dev_last_obj(), co_obj_next()
 */
co_obj_t *co_obj_prev(const co_obj_t *obj);

/**
 * Finds the next object in the object dictionary of a CANopen device.
 *
 * @returns a pointer to the next object, or NULL if this is the first object.
 *
 * @see co_dev_first_obj(), co_obj_prev()
 */
co_obj_t *co_obj_next(const co_obj_t *obj);

/// Returns a pointer to the CANopen device containing the specified object.
co_dev_t *co_obj_get_dev(const co_obj_t *obj);

/// Returns the index of a CANopen object.
co_unsigned16_t co_obj_get_idx(const co_obj_t *obj);

/**
 * Retrieves a list of sub-indices in a CANopen object.
 *
 * @param obj       a pointer to a CANopen object.
 * @param maxsubidx the maximum number of sub-indices to return.
 * @param subidx    an array of at least <b>maxsubidx</b> indices (can be NULL).
 *                  On success, *<b>idx</b> contains the sub-indices.
 *
 * @returns the total number of sub-indices in the object (which may be
 * different from <b>maxsubidx</b>).
 */
co_unsigned8_t co_obj_get_subidx(const co_obj_t *obj, co_unsigned8_t maxsubidx,
		co_unsigned8_t *subidx);

/**
 * Inserts a sub-object into a CANopen object. This function fails if the
 * sub-object is already part of another object, or of another sub-object with
 * the same sub-index already exists.
 *
 * @param obj a pointer to a CANopen object.
 * @param sub a pointer to the sub-object to be inserted.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_obj_remove_sub(), co_obj_find_sub()
 */
int co_obj_insert_sub(co_obj_t *obj, co_sub_t *sub);

/**
 * Removes a sub-object from a CANopen object.
 *
 * @param obj a pointer to a CANopen object.
 * @param sub a pointer to the sub-object to be removed.
 *
 * @returns 0 on success, or -1 on error.
 *
 * @see co_obj_insert_sub()
 */
int co_obj_remove_sub(co_obj_t *obj, co_sub_t *sub);

/**
 * Finds a sub-object in a CANopen object.
 *
 * @param obj    a pointer to a CANopen object.
 * @param subidx the object sub-index.
 *
 * @returns a pointer to the sub-object if found, or NULL if not.
 *
 * @see co_obj_insert_sub()
 */
co_sub_t *co_obj_find_sub(const co_obj_t *obj, co_unsigned8_t subidx);

/**
 * Finds the first sub-object (with the lowest sub-index) in a CANopen object.
 *
 * @returns a pointer to the first sub-object, or NULL if the object is empty.
 *
 * @see co_dev_last_obj(), co_sub_next()
 */
co_sub_t *co_obj_first_sub(const co_obj_t *obj);

/**
 * Finds the last sub-object (with the highest sub-index) in a CANopen object.
 *
 * @returns a pointer to the last sub-object, or NULL if the object is empty.
 *
 * @see co_obj_first_sub(), co_sub_prev()
 */
co_sub_t *co_obj_last_sub(const co_obj_t *obj);

/// Returns the name of a CANopen object. @see co_obj_set_name()
const char *co_obj_get_name(const co_obj_t *obj);

/**
 * Sets the name of a CANopen object.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_obj_get_name()
 */
int co_obj_set_name(co_obj_t *obj, const char *name);

/// Returns the object code of a CANopen object. @see co_obj_set_code()
co_unsigned8_t co_obj_get_code(const co_obj_t *obj);

/**
 * Sets the code (type) of a CANopen object.
 *
 * @param obj  a pointer to a CANopen object.
 * @param code the object code (one of #CO_OBJECT_NULL, #CO_OBJECT_DOMAIN,
 *             #CO_OBJECT_DEFTYPE, #CO_OBJECT_DEFSTRUCT, #CO_OBJECT_VAR,
 *             #CO_OBJECT_ARRAY or #CO_OBJECT_RECORD).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_obj_get_code()
 */
int co_obj_set_code(co_obj_t *obj, co_unsigned8_t code);

/**
 * Returns the address of the value of a CANopen object. In case of compound
 * data types, this is a pointer to the struct or array containing the members.
 *
 * @see co_obj_sizeof_val()
 */
void *co_obj_addressof_val(const co_obj_t *obj);

/**
 * Returns size (in bytes) of the value of a CANopen object.
 *
 * @see co_obj_addressof_val()
 */
size_t co_obj_sizeof_val(const co_obj_t *obj);

/**
 * Returns a pointer to the current value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of a pointer to the first byte in
 * the array.
 *
 * @see co_obj_set_val(), co_sub_get_val()
 */
const void *co_obj_get_val(const co_obj_t *obj, co_unsigned8_t subidx);

/**
 * Sets the current value of a CANopen sub-object.
 *
 * @param obj    a pointer to a CANopen object.
 * @param subidx the object sub-index.
 * @param ptr    a pointer to the bytes to be copied. In case of strings or
 *               domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n      the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *               SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_obj_get_val(), co_sub_set_val()
 */
size_t co_obj_set_val(co_obj_t *obj, co_unsigned8_t subidx, const void *ptr,
		size_t n);

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_obj_get_val_##c( \
			const co_obj_t *obj, co_unsigned8_t subidx); \
	size_t co_obj_set_val_##c( \
			co_obj_t *obj, co_unsigned8_t subidx, co_##b##_t c);
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

/**
 * Sets the download indication function for a CANopen object. This function
 * invokes co_sub_set_dn_ind() for all sub-objects.
 *
 * @param obj  a pointer to a CANopen object.
 * @param ind  a pointer to the indication function. If <b>ind</b> is NULL, the
 *             default indication function will be used.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 */
void co_obj_set_dn_ind(co_obj_t *obj, co_sub_dn_ind_t *ind, void *data);

/**
 * Sets the upload indication function for a CANopen object. This function
 * invokes co_sub_set_up_ind() for all sub-objects.
 *
 * @param obj  a pointer to a CANopen object.
 * @param ind  a pointer to the indication function. If <b>ind</b> is NULL, the
 *             default indication function will be used.
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 */
void co_obj_set_up_ind(co_obj_t *obj, co_sub_up_ind_t *ind, void *data);

#if !LELY_NO_MALLOC
void *__co_sub_alloc(void);
void __co_sub_free(void *ptr);
#endif

struct __co_sub *__co_sub_init(struct __co_sub *sub, co_unsigned8_t subidx,
		co_unsigned16_t type, void *val);
void __co_sub_fini(struct __co_sub *sub);

#if !LELY_NO_MALLOC

/**
 * Creates a CANopen sub-object.
 *
 * @param subidx the object sub-index.
 * @param type   the data type of the sub-object value (in the range [1..27]).
 *               This MUST be the object index of one of the static data types.
 *
 * @returns a pointer to a new CANopen sub-object, or NULL on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_sub_destroy()
 */
co_sub_t *co_sub_create(co_unsigned8_t subidx, co_unsigned16_t type);

/// Destroys a CANopen sub-object. @see co_sub_create()
void co_sub_destroy(co_sub_t *sub);

#endif // !LELY_NO_MALLOC

/**
 * Finds the previous sub-object in a CANopen object.
 *
 * @returns a pointer to the previous sub-object, or NULL if this is the first
 * sub-object.
 *
 * @see co_obj_last_sub(), co_sub_next()
 */
co_sub_t *co_sub_prev(const co_sub_t *sub);

/**
 * Finds the next sub-object in a CANopen object.
 *
 * @returns a pointer to the next sub-object, or NULL if this is the first
 * sub-object.
 *
 * @see co_obj_first_sub(), co_sub_prev()
 */
co_sub_t *co_sub_next(const co_sub_t *sub);

/**
 * Returns the a pointer to the CANopen object containing the specified
 * sub-object.
 */
co_obj_t *co_sub_get_obj(const co_sub_t *sub);

/// Returns the sub-index of a CANopen sub-object.
co_unsigned8_t co_sub_get_subidx(const co_sub_t *sub);

/// Returns the name of a CANopen sub-object. @see co_sub_set_name()
const char *co_sub_get_name(const co_sub_t *sub);

/**
 * Sets the name of a CANopen sub-object.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_sub_get_name()
 */
int co_sub_set_name(co_sub_t *sub, const char *name);

/// Returns the data type of a CANopen sub-object.
co_unsigned16_t co_sub_get_type(const co_sub_t *sub);

/**
 * Returns the address of the lower limit of the value of a CANopen sub-object.
 * In the case of strings or domains, this is the address of the first byte in
 * the array.
 *
 * @see co_sub_sizeof_min()
 */
const void *co_sub_addressof_min(const co_sub_t *sub);

/**
 * Returns size (in bytes) of the lower limit of the value of a CANopen
 * sub-object. In the case of strings or domains, this is the number of bytes in
 * the array.
 *
 * @see co_sub_addressof_min()
 */
size_t co_sub_sizeof_min(const co_sub_t *sub);

/**
 * Returns a pointer to the lower limit of the value of a CANopen sub-object. In
 * the case of strings or domains, this is the address of a pointer to the first
 * byte in the array.
 *
 * @see co_sub_set_min()
 */
const void *co_sub_get_min(const co_sub_t *sub);

/**
 * Sets the lower limit of a value of a CANopen sub-object.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param ptr a pointer to the bytes to be copied. In case of strings or
 *            domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n   the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *            SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_sub_get_min()
 */
size_t co_sub_set_min(co_sub_t *sub, const void *ptr, size_t n);

/**
 * Returns the address of the upper limit of the value of a CANopen sub-object.
 * In the case of strings or domains, this is the address of the first byte in
 * the array.
 *
 * @see co_sub_sizeof_max()
 */
const void *co_sub_addressof_max(const co_sub_t *sub);

/**
 * Returns size (in bytes) of the upper limit of the value of a CANopen
 * sub-object. In the case of strings or domains, this is the number of bytes in
 * the array.
 *
 * @see co_sub_addressof_max()
 */
size_t co_sub_sizeof_max(const co_sub_t *sub);

/**
 * Returns a pointer to the upper limit of the value of a CANopen sub-object. In
 * the case of strings or domains, this is the address of a pointer to the first
 * byte in the array.
 *
 * @see co_sub_set_max()
 */
const void *co_sub_get_max(const co_sub_t *sub);

/**
 * Sets the upper limit of a value of a CANopen sub-object.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param ptr a pointer to the bytes to be copied. In case of strings or
 *            domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n   the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *            SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_sub_get_max()
 */
size_t co_sub_set_max(co_sub_t *sub, const void *ptr, size_t n);

/**
 * Returns the address of the default value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of the first byte in the array.
 *
 * @see co_sub_sizeof_def()
 */
const void *co_sub_addressof_def(const co_sub_t *sub);

/**
 * Returns the size (in bytes) of the default value of a CANopen sub-object. In
 * the case of strings or domains, this is the number of bytes in the array.
 *
 * @see co_sub_addressof_def()
 */
size_t co_sub_sizeof_def(const co_sub_t *sub);

/**
 * Returns a pointer to the default value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of a pointer to the first byte in
 * the array.
 *
 * @see co_sub_set_def()
 */
const void *co_sub_get_def(const co_sub_t *sub);

/**
 * Sets the default value of a CANopen sub-object.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param ptr a pointer to the bytes to be copied. In case of strings or
 *            domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n   the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *            SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_sub_get_def()
 */
size_t co_sub_set_def(co_sub_t *sub, const void *ptr, size_t n);

/**
 * Returns the address of the current value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of the first byte in the array.
 *
 * @see co_sub_sizeof_val()
 */
const void *co_sub_addressof_val(const co_sub_t *sub);

/**
 * Returns the size (in bytes) of the current value of a CANopen sub-object. In
 * the case of strings or domains, this is the number of bytes in the array.
 *
 * @see co_sub_addressof_val()
 */
size_t co_sub_sizeof_val(const co_sub_t *sub);

/**
 * Returns a pointer to the current value of a CANopen sub-object. In the case
 * of strings or domains, this is the address of a pointer to the first byte in
 * the array.
 *
 * @see co_sub_set_val()
 */
const void *co_sub_get_val(const co_sub_t *sub);

/**
 * Sets the current value of a CANopen sub-object.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param ptr a pointer to the bytes to be copied. In case of strings or
 *            domains, <b>ptr</b> MUST point to the first byte in the array.
 * @param n   the number of bytes at <b>ptr</b>. In case of strings, <b>n</b>
 *            SHOULD exclude the terminating null byte(s).
 *
 * @returns the number of bytes copied (i.e., <b>n</b>), or 0 on error. In the
 * latter case, the error number can be obtained with get_errc().
 *
 * @see co_sub_get_val()
 */
size_t co_sub_set_val(co_sub_t *sub, const void *ptr, size_t n);

#define LELY_CO_DEFINE_TYPE(a, b, c, d) \
	co_##b##_t co_sub_get_val_##c(const co_sub_t *sub); \
	size_t co_sub_set_val_##c(co_sub_t *sub, co_##b##_t c);
#include <lely/co/def/basic.def>
#undef LELY_CO_DEFINE_TYPE

/**
 * Checks if the specifed value would be a valid value for a CANopen sub-object.
 * This function checks if the specified type matches that of the sub-object
 * and, in the case of basic types, if the value is within range.
 *
 * @param sub  a pointer to a CANopen sub-object.
 * @param type the data type (in the range [1..27]). This MUST be the object
 *             index of one of the static data types.
 * @param val  a pointer to the value to be checked. In the case of strings or
 *             domains, this MUST be the address of pointer.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_sub_chk_val(
		const co_sub_t *sub, co_unsigned16_t type, const void *val);

/// Returns the access type of a CANopen sub-object. @see co_sub_set_access()
unsigned int co_sub_get_access(const co_sub_t *sub);

/**
 * Sets the access type of a CANopen sub-object.
 *
 * @param sub    a pointer to a CANopen sub-object.
 * @param access the access type (one of #CO_ACCESS_RO, #CO_ACCESS_WO,
 *               #CO_ACCESS_RW or #CO_ACCESS_CONST).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_sub_get_access()
 */
int co_sub_set_access(co_sub_t *sub, unsigned int access);

/**
 * Returns 1 if it is possible to map the specified CANopen sub-object into a
 * PDO, and 0 if not.
 *
 * @see co_sub_set_pdo_mapping()
 */
int co_sub_get_pdo_mapping(const co_sub_t *sub);

/**
 * Enables or disables PDO mapping a CANopen sub-object.
 *
 * @see co_sub_get_pdo_mapping()
 */
void co_sub_set_pdo_mapping(co_sub_t *sub, int pdo_mapping);

/// Returns the object flags of a CANopen sub-object. @see co_sub_set_flags()
unsigned int co_sub_get_flags(const co_sub_t *sub);

/// Sets the object flags of a CANopen sub-object. @see co_sub_get_flags()
void co_sub_set_flags(co_sub_t *sub, unsigned int flags);

/**
 * Returns a pointer to the value of the UploadFile attribute of a CANopen
 * sub-object, or NULL if the attribute does not exist.
 *
 * @see co_sub_set_upload_file()
 */
const char *co_sub_get_upload_file(const co_sub_t *sub);

/**
 * Sets the value of the UploadFile attribute of a CANopen sub-object.
 *
 * @param sub      a pointer to a CANopen sub-object.
 * @param filename a pointer to the null-terminated string to be copied to the
 *                 UploadFile attribute.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @pre the result of co_sub_get_flags() contains #CO_OBJ_FLAGS_UPLOAD_FILE.
 *
 * @see co_sub_get_upload_file()
 */
int co_sub_set_upload_file(co_sub_t *sub, const char *filename);

/**
 * Returns a pointer to the value of the DownloadFile attribute of a CANopen
 * sub-object, or NULL if the attribute does not exist.
 *
 * @see co_sub_set_download_file()
 */
const char *co_sub_get_download_file(const co_sub_t *sub);

/**
 * Sets the value of the DownloadFile attribute of a CANopen sub-object.
 *
 * @param sub      a pointer to a CANopen sub-object.
 * @param filename a pointer to the null-terminated string to be copied to the
 *                 DownloadFile attribute.
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @pre the result of co_sub_get_flags() contains #CO_OBJ_FLAGS_DOWNLOAD_FILE.
 *
 * @see co_sub_get_download_file()
 */
int co_sub_set_download_file(co_sub_t *sub, const char *filename);

/**
 * Retrieves the download indication function for a CANopen sub-object.
 *
 * @param sub   a pointer to a CANopen sub-object.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_sub_set_dn_ind()
 */
void co_sub_get_dn_ind(
		const co_sub_t *sub, co_sub_dn_ind_t **pind, void **pdata);

/**
 * Sets the download indication function for a CANopen sub-object. This function
 * is invoked by co_sub_dn_ind().
 *
 * @param sub  a pointer to a CANopen sub-object.
 * @param ind  a pointer to the indication function. If <b>ind</b> is NULL, the
 *             default indication function will be used (which invokes
 *             co_sub_on_dn()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_sub_get_dn_ind()
 */
void co_sub_set_dn_ind(co_sub_t *sub, co_sub_dn_ind_t *ind, void *data);

/**
 * Implements the default behavior when a download indication is received by a
 * CANopen sub-object. For a domain value with the #CO_OBJ_FLAGS_DOWNLOAD_FILE
 * flag set, this function invokes co_sdo_req_dn_file() to write the value to
 * file. Otherwise the value is read from the SDO download request with
 * co_sdo_req_dn_val() and, if it is within the specified range, written to the
 * object dictionary with co_sub_dn().
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param req a pointer to a CANopen SDO download request.
 * @param pac the address of a value which, on error, contains the SDO abort
 *            code (can be NULL).
 *
 * @returns 0 if all segments have been received and the value has been
 * successfully written to the object dictionary, and -1 if one or more segments
 * remain or an error has occurred. In the latter case, *<b>pac</b> contains the
 * SDO abort code.
 */
int co_sub_on_dn(co_sub_t *sub, struct co_sdo_req *req, co_unsigned32_t *pac);

/**
 * Invokes the download indication function of a CANopen sub-object, registered
 * with co_sub_set_dn_ind(). This is used for writing values to the object
 * dictionary. If the indication function returns an error, or the
 * refuse-write-on-download flag (#CO_OBJ_FLAGS_WRITE) is set, the value of the
 * sub-object is left untouched.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param req a pointer to a CANopen SDO download request. All members of
 *            *<b>req</b>, except <b>membuf</b>, MUST be set by the caller. The
 *            <b>membuf</b> MUST be initialized before the first invocation and
 *            MUST only be used by the indication function.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_sub_dn_ind_val()
 */
co_unsigned32_t co_sub_dn_ind(co_sub_t *sub, struct co_sdo_req *req);

/**
 * Invokes the download indication function of a CANopen sub-object, registered
 * with co_sub_set_dn_ind(). This is used for writing values to the object
 * dictionary. If the indication function returns an error, or the
 * refuse-write-on-download flag (#CO_OBJ_FLAGS_WRITE) is set, the value of the
 * sub-object is left untouched.
 *
 * @param sub   a pointer to a CANopen sub-object.
 * @param type  the data type (in the range [1..27]). This MUST be the object
 *              index of one of the static data types and SHOULD be the same as
 *              the return value of co_sub_get_type().
 * @param val   the address of the value to be written. In case of string or
 *              domains, this MUST be the address of pointer.
 *
 * @returns 0 on success, or an SDO abort code on error.
 *
 * @see co_sub_dn_ind()
 */
co_unsigned32_t co_sub_dn_ind_val(
		co_sub_t *sub, co_unsigned16_t type, const void *val);

/**
 * Downloads (moves) a value into a CANopen sub-object if the
 * refuse-write-on-download flag (#CO_OBJ_FLAGS_WRITE) is _not_ set. This
 * function is invoked by the default download indication function.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param val a pointer to the value to be written. In the case of strings or
 *            domains, this MUST be the address of pointer (which is set to NULL
 *            if the value is moved).
 *
 * @returns 0 on success, or -1 on error. In the latter case, the error number
 * can be obtained with get_errc().
 *
 * @see co_val_move()
 */
int co_sub_dn(co_sub_t *sub, void *val);

/**
 * Retrieves the upload indication function for a CANopen sub-object.
 *
 * @param sub   a pointer to a CANopen sub-object.
 * @param pind  the address at which to store a pointer to the indication
 *              function (can be NULL).
 * @param pdata the address at which to store a pointer to user-specified data
 *              (can be NULL).
 *
 * @see co_sub_set_up_ind()
 */
void co_sub_get_up_ind(
		const co_sub_t *sub, co_sub_up_ind_t **pind, void **pdata);

/**
 * Sets the upload indication function for a CANopen sub-object. This function
 * is invoked by co_sub_up_ind().
 *
 * @param sub  a pointer to a CANopen sub-object.
 * @param ind  a pointer to the indication function. If <b>ind</b> is NULL, the
 *             default indication function will be used (which invokes
 *             co_sub_on_up()).
 * @param data a pointer to user-specified data (can be NULL). <b>data</b> is
 *             passed as the last parameter to <b>func</b>.
 *
 * @see co_sub_get_up_ind()
 */
void co_sub_set_up_ind(co_sub_t *sub, co_sub_up_ind_t *ind, void *data);

/**
 * Implements the default behavior when an upload indication is received by a
 * CANopen sub-object. For a domain value with the #CO_OBJ_FLAGS_UPLOAD_FILE
 * flag set, this function invokes co_sdo_req_up_file() to read the value from
 * file. Otherwise the value is read from the object dictionary with
 * co_sub_get_val() and written to the SDO upload request with
 * co_sdo_req_up_val().
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param req a pointer to a CANopen SDO upload request.
 * @param pac the address of a value which, on error, contains the SDO abort
 *            code (can be NULL).
 *
 * @returns 0 on success, or -1 on error. In the latter case, *<b>pac</b>
 * contains the SDO abort code.
 */
int co_sub_on_up(const co_sub_t *sub, struct co_sdo_req *req,
		co_unsigned32_t *pac);

/**
 * Invokes the upload indication function of a CANopen sub-object, registered
 * with co_sub_set_up_ind(). This is used for reading values from the object
 * dictionary.
 *
 * @param sub a pointer to a CANopen sub-object.
 * @param req a pointer to a CANopen SDO upload request. The <b>size</b> member
 *            of *<b>req</b> MUST be set to 0 on the first invocation. All
 *            members MUST be initialized by the indication function.
 *
 * @returns 0 on success, or an SDO abort code on error.
 */
co_unsigned32_t co_sub_up_ind(const co_sub_t *sub, struct co_sdo_req *req);

#ifdef __cplusplus
}
#endif

#endif // !LELY_CO_OBJ_H_
