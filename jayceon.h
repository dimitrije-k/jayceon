/*
 * jayceon.h - Simplistic JSON parser
 *
 ********************************* LICENSE ***********************************
 *
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <https://unlicense.org>
 */
#ifndef JAYCEON_H_
#define JAYCEON_H_

#include <stddef.h>

/** @brief Generic JSON value, accessed using jy_is_* functions */
typedef struct JYValue_ JYValue;
/** @brief Object representing a JSON array */
typedef struct JYArray_ JYArray;
/** @brief Object representing a JSON object (associative array) */
typedef struct JYObject_ JYObject;
/** @brief Object representing a JSON document */
typedef struct JYDocument_ JYDocument;

/**
 * @brief Parses a serialized JSON object
 * @param string The string to be parsed
 * @return Resulting document, or NULL on failure
 */
JYDocument *jy_parse(const char *string);

/**
 * @brief Frees all memory allocated to the document
 * @param doc the document to free
 */
void jy_free(JYDocument *doc);

/**
 * @brief Returns the root of the JSON document
 * @param doc The document to query
 * @return The root object of the document
 * @note This function cannot fail
 */
JYObject *jy_root(JYDocument *doc);

/**
 * @brief Serializes the document and prints it out into stdout
 * @param doc The document to print
 * @warning Only defined when NDEBUG is not defined
 */
void jy_print(JYDocument *doc);

/**
 * @brief Indexes an object by a string key
 * @param obj The object to index
 * @param key The string key to use
 * @return The resulting value, or NULL if it does not exist
 */
JYValue *jy_index_s(JYObject *obj, const char *key);

/**
 * @brief Indexes an array by an integer key
 * @param obj The object to index
 * @param key The key to use
 * @return The resulting value, or NULL if it does not exist
 */
JYValue *jy_index_i(JYArray *obj, size_t key);

/**
 * @brief Indexes an object by an integer key
 * @param obj The object to index
 * @param index The integer key to use
 * @param out_key The string key of the value (not written on failure)
 * @return The resulting value, or NULL if it does not exist
 */
JYValue *jy_index_i_obj(JYObject *obj, size_t index, const char **out_key);

/**
 * @brief Checks if the value type is a null
 * @param val The value to check
 * @return Non-zero if the value type is a null
 */
int jy_is_null(JYValue *val);

/**
 * @brief Checks if the value type is a boolean
 * @param val The value to check
 * @param out The boolean value of the value object
 * @return Non-zero if the value type is a boolean
 */
int jy_is_bool(JYValue *val, int *out);

/**
 * @brief Checks if the value type is a number
 * @param val The value to check
 * @param out The number value of the value object
 * @return Non-zero if the value type is a number
 */
int jy_is_number(JYValue *val, double *out);

/**
 * @brief Checks if the value type is a string
 * @param val The value to check
 * @param out The string value of the value object
 * @return Non-zero if the value type is a string
 */
int jy_is_string(JYValue *val, const char **out);

/**
 * @brief Checks if the value type is an array
 * @param val The value to check
 * @param out The array value of the value object
 * @return Non-zero if the value type is an array
 */
int jy_is_array(JYValue *val, JYArray **out);

/**
 * @brief Checks if the value type is an object
 * @param val The value to check
 * @param out The object value of the value object
 * @return Non-zero if the value type is an object
 */
int jy_is_object(JYValue *val, JYObject **out);

#endif /* JAYCEON_H_ */
