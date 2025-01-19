/*
 * JayceON is a JSON parser compatible with https://json.org
 *
 * For information on how to use the API, read the header file
 * Compilation can be customized by defining the following macros:
 * 
 *   JAYCEON_STRING_GROWTH - Define a number to be the number of bytes that
 * are allocated when a string runs out of memory
 * 
 *   JAYCEON_ARRAY_GROWTH - Define a number to be the number of values that
 * are allocated when an array runs out of memory
 * 
 *   JAYCEON_ARRAY_GROWTH - Define a number to be the number of pairs that are
 * allocated when an object runs out of memory
 * 
 *   JAYCEON_NO_COMMENT_SUPPORT - Disables comment support. When this is not
 * defined, comments are simply ignored, but if it is, the parser fails when
 * it encounters them
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "jayceon.h"

#ifndef JAYCEON_STRING_GROWTH
#define JAYCEON_STRING_GROWTH (16)
#endif

#ifndef JAYCEON_ARRAY_GROWTH
#define JAYCEON_ARRAY_GROWTH (16)
#endif

#ifndef JAYCEON_OBJECT_GROWTH
#define JAYCEON_OBJECT_GROWTH (16)
#endif

typedef enum Type_ {
	TYPE_NULL,
	TYPE_BOOL,
	TYPE_NUMBER,
	TYPE_STRING,
	TYPE_ARRAY,
	TYPE_OBJECT
} Type;

struct JYArray_ {
	JYValue *values;
	size_t count;
	size_t capacity;
};

struct JYObject_ {
	struct Pair_ *pairs;
	size_t count;
	size_t capacity;
};

struct JYValue_ {
	Type type;
	union {
		int _bool;
		double _number;
		char *_string;
		JYArray _array;
		JYObject _object;
	} value;
};

typedef struct Pair_ {
	char *key;
	JYValue value;
} Pair;

struct JYDocument_ {
	JYObject root;
};

static const char *parse_space(const char *string);
static const char *parse_null(const char *string);
static const char *parse_bool(const char *string, int *out);
static const char *parse_number(const char *string, double *out);
static const char *parse_string(const char *string, char **out);
static const char *parse_array(const char *string, JYArray *out);
static const char *parse_object(const char *string, JYObject *out);
static const char *parse_value(const char *string, JYValue *out);

static void free_value(JYValue *val);
static void free_array(JYArray *array);
static void free_object(JYObject *object);

static double pow10(double x, int y) {
	int i;

	if (abs(y) > 30)
		return 0.0;

	for (i = 0; i < y; ++i)
		x *= 10.0;

	for (i = 0; i < -y; ++i)
		x *= 0.1;
	
	return x;
}

JYDocument *jy_parse(const char *string) {
	JYDocument *doc;

	doc = malloc(sizeof(*doc));
	if (!doc) {
		return NULL;
	}

	if (!parse_object(string, &doc->root)) {
		free(doc);
		return NULL;
	}

	return doc;
}

void jy_free(JYDocument *doc) {
	free_object(&doc->root);
	free((void *) doc);
}

JYObject *jy_root(JYDocument *doc) {
	return &doc->root;
}

JYValue *jy_index_s(JYObject *obj, const char *key) {
	ptrdiff_t l, r, m;
	int res;

	l = 0, r = (ptrdiff_t) obj->count - 1;
	while (l <= r) {
		m = (l + r) / 2;
		res = strcmp(key, obj->pairs[m].key);

		if (res > 0) {
			l = m + 1;
		} else if (res < 0) {
			r = m - 1;
		} else {
			return &obj->pairs[m].value;
		}
	}

	return NULL;
}

JYValue *jy_index_i(JYArray *obj, size_t key) {
	if (obj->count < key)
		return NULL;

	return &obj->values[key];
}

JYValue *jy_index_i_obj(JYObject *obj, size_t index, const char **out_key) {
	if (obj->count < index)
		return NULL;
	
	*out_key = (const char *) obj->pairs[index].key;
	return &obj->pairs[index].value;
}

/* Type checks */
int jy_is_null(JYValue *val) {
	return val->type == TYPE_NULL;
}

int jy_is_bool(JYValue *val, int *out) {
	if (val->type != TYPE_BOOL)
		return 0;
	
	*out = val->value._bool;
	return 1;
}

int jy_is_number(JYValue *val, double *out) {
	if (val->type != TYPE_NUMBER)
		return 0;
	
	*out = val->value._number;
	return 1;
}

int jy_is_string(JYValue *val, const char **out) {
	if (val->type != TYPE_STRING)
		return 0;
	
	*out = (const char *) val->value._string;
	return 1;
}

int jy_is_array(JYValue *val, JYArray **out) {
	if (val->type != TYPE_ARRAY)
		return 0;
	
	*out = &val->value._array;
	return 1;
}

int jy_is_object(JYValue *val, JYObject **out) {
	if (val->type != TYPE_OBJECT)
		return 0;
	
	*out = &val->value._object;
	return 1;
}

#ifndef NDEBUG
static void print_string(const char *str);
static void print_array(JYArray *arr);
static void print_object(JYObject *obj);
static void print_value(JYValue *val);

/* Serialization? */
void jy_print(JYDocument *doc) {
	print_object(&doc->root);
	printf("\n");
}

static void print_string(const char *str) {
	printf("\"");

	while (*str) {
		if (*str == '\"' || *str == '\\')
			printf("\\%c", *str);
		else if (*str == '\n')
			printf("\\n");
		else if (*str == '\r')
			printf("\\r");
		else if (*str == '\b')
			printf("\\b");
		else if (*str == '\f')
			printf("\\f");
		else if (*str == '\t')
			printf("\\t");
		else
			printf("%c", *str);

		++str;
	}

	printf("\"");
}

static void print_array(JYArray *arr) {
	size_t i;

	printf("[");
	for (i = 0; i < arr->count; ++i) {
		print_value(&arr->values[i]);
		if (i != arr->count - 1)
			printf(",");
	}
	printf("]");
}

static void print_object(JYObject *obj) {
	size_t i;

	printf("{");
	for (i = 0; i < obj->count; ++i) {
		print_string(obj->pairs[i].key);
		printf(":");
		print_value(&obj->pairs[i].value);
		
		if (i != obj->count - 1)
			printf(",");
	}
	printf("}");
}

static void print_value(JYValue *val) {
	if (val->type == TYPE_NULL)
		printf("null");
	else if (val->type == TYPE_BOOL)
		printf("%s", val->value._bool ? "true" : "false");
	else if (val->type == TYPE_NUMBER)
		printf("%f", val->value._number);
	else if (val->type == TYPE_STRING)
		print_string(val->value._string);
	else if (val->type == TYPE_ARRAY)
		print_array(&val->value._array);
	else if (val->type == TYPE_OBJECT)
		print_object(&val->value._object);
}
#endif /* NDEBUG */

static const char *parse_space(const char *string) {
	while (*string) {
		if (!isspace(*string)) {
		#ifndef JAYCEON_NO_COMMENT_SUPPORT
			if (string[0] == '/' && string[1] == '/') {
				string += 2;

				while (*string && *string != '\n')
					++string;
			} else if (string[0] == '/' && string[1] == '*') {
				string += 2;

				while (*string) {
					if (string[0] == '*' && string[1] == '/') {
						string += 2;
						break;
					}

					++string;
				}
			} else {
		#endif
				break;
		#ifndef JAYCEON_NO_COMMENT_SUPPORT
			}
		#endif
		} else {
			++string;
		}
	}

	return string;
}

static const char *parse_null(const char *string) {
	int i;

	for (i = 0; i < 4; ++i)
		if (string[i] != "null"[i])
			return NULL;

	return string + 4;
}

static const char *parse_bool(const char *string, int *out) {
	const char *word;
	int val;

	word = *string == 't' ? "true" : "false";

	val = *word == 't';

	while (*word) {
		if (*word != *string)
			return NULL;
		
		++word, ++string;
	}

	*out = val;
	return string;
}

static const char *parse_number(const char *string, double *out) {
	int sign;
	double val;

	if (*string == '-') {
		++string;
		sign = 1;
	} else {
		sign = 0;
	}

	val = 0.0;

	if (*string == '0') {
		++string;
	} else {
		while (*string >= '0' && *string <= '9') {
			val = val * 10.0 + (double) (*string - '0');

			++string;
		}

		if (!string || val == 0.0)
			return NULL;
	}

	if (*string == '.') {
		double frac;

		++string;

		if (*string < '0' || *string > '9')
			return NULL;

		frac = 1.0;

		while (*string >= '0' && *string <= '9') {
			frac *= 0.1;
			val += (double) (*string - '0') * frac;

			++string;
		}
	}

	if (*string == 'e' || *string == 'E') {
		int sign;
		int exp;

		++string;

		if (*string == '+')
			sign = 0;
		else if (*string == '-')
			sign = 1;
		else
			return NULL;

		++string;
		
		if (*string < '0' || *string > '9')
			return NULL;

		exp = 0.0;

		while (*string >= '0' && *string <= '9') {
			exp = exp * 10 + *string - '0';
			++string;
		}

		if (sign)
			exp *= -1;
		
		val = pow10(val, exp);
	}

	if (sign)
		val *= -1.0;

	*out = val;
	return string;
}

/*
 * NOTE: The current string implementation rejects strings that contain \uXXXX
 * escape sequences.
 */
static const char *parse_string(const char *string, char **out) {
	char *val;
	size_t cap, len;

	const size_t GROWTH = JAYCEON_STRING_GROWTH;

	if (*string != '\"')
		return NULL;

	++string;

	val = NULL;
	cap = len = 0;

	while (*string != '\"') {
		char c;

		if (*string == '\n' || *string == '\0') {
			free(val);
			return NULL;
		} else if (*string == '\\') {
			++string;

			if (*string == 'n') {
				c = '\n';
			} else if (*string == 'r') {
				c = '\r';
			} else if (*string == 'b') {
				c = '\b';
			} else if (*string == 'f') {
				c = '\f';
			} else if (*string == 't') {
				c = '\t';
			} else if (*string == '\"' || *string == '\\' || *string == '/') {
				c = *string;
			} else {
				free(val);
				return NULL;
			}
		} else {
			c = *string;
		}

		if (len == cap) {
			size_t newcap;
			char *newval;

			newcap = cap + GROWTH;
			newval = realloc(val, newcap);
			if (!newval) {
				free(val);
				return NULL;
			}

			cap = newcap;
			val = newval;
		}

		val[len++] = c;

		++string;
	}

	if (cap == len) {
		size_t newcap;
		char *newval;
		
		newcap = cap + 1;
		newval = realloc(val, newcap);
		if (!newval) {
			free(val);
			return NULL;
		}

		cap = newcap;
		val = newval;
	}

	val[len] = '\0';

	do {
		char *newval;
		
		newval = realloc(val, len + 1);
		if (newval)
			val = newval;
	} while (0);

	++string;
	*out = val;
	return string;
}

static const char *parse_array(const char *string, JYArray *out) {
	const size_t GROWTH = JAYCEON_ARRAY_GROWTH;

	if (*string != '[') {
		return NULL;
	}

	++string;

	out->values = NULL;
	out->capacity = out->count = 0;

	string = parse_space(string);

	if (*string == '\0') {
		return NULL;
	}

	while (*string != ']') {
		JYValue res;

		if (!(string = parse_value(string, &res))) {
			free_array(out);
			return NULL;
		}

		if (out->count == out->capacity) {
			size_t newcap;
			JYValue *newvals;
			
			newcap = out->capacity + GROWTH;
			newvals = realloc(out->values, sizeof(JYValue) * newcap);
			if (!newvals) {
				free_value(&res);
				free_array(out);
				return NULL;
			}

			out->capacity = newcap;
			out->values = newvals;
		}

		out->values[out->count++] = res;

		string = parse_space(string);
		
		if (*string == ',') {
			++string;
		} else if (*string != ']') {
			free_array(out);
			return NULL;
		}

		string = parse_space(string);
	}

	return ++string;
}

static const char *parse_object(const char *string, JYObject *out) {
	const size_t GROWTH = JAYCEON_OBJECT_GROWTH;

	if (*string != '{')
		return NULL;
	
	++string;

	out->pairs = NULL;
	out->capacity = out->count = 0;

	string = parse_space(string);

	if (*string == '\0') {
		return NULL;
	}

	while (*string != '}') {
		char *key;
		JYValue val;

		if (!(string = parse_string(string, &key))) {
			free_object(out);
			return NULL;
		}

		string = parse_space(string);

		if (*string != ':') {
			free(key);
			free_object(out);
			return NULL;
		}

		++string;

		string = parse_space(string);
		
		if (!(string = parse_value(string, &val))) {
			free(key);
			free_object(out);
			return NULL;
		}

		if (out->count == out->capacity) {
			size_t newcap;
			Pair *newpairs;
			
			newcap = out->capacity + GROWTH;
			newpairs = realloc(out->pairs, sizeof(Pair) * newcap);
			if (!newpairs) {
				free(key);
				free_value(&val);
				free_object(out);
				return NULL;
			}

			out->capacity = newcap;
			out->pairs = newpairs;
		}

		do {
			ptrdiff_t l, r, m;

			l = 0, r = (ptrdiff_t) out->count - 1;
			while (l <= r) {
				int res;

				m = (l + r) / 2;
				res = strcmp(key, out->pairs[m].key);

				if (res > 0) {
					l = m + 1;
				} else if (res < 0 ) {
					r = m - 1;
				} else {
					/* Duplicate keys not permitted */
					free(key);
					free_value(&val);
					free_object(out);
					return NULL;
				}
			}

			memmove(out->pairs + l + 1, out->pairs + l, (out->count++ - (size_t) l) * sizeof(Pair));

			out->pairs[l].key = key;
			out->pairs[l].value = val;
		} while (0);

		string = parse_space(string);
		
		if (*string == ',') {
			++string;
		} else if (*string != '}') {
			free_object(out);
			return NULL;
		}

		string = parse_space(string);
	}

	return ++string;
}

static const char *parse_value(const char *string, JYValue *out) {
	const char *tmp;

	if ((tmp = parse_null(string)))
		out->type = TYPE_NULL;
	else if ((tmp = parse_bool(string, &out->value._bool)))
		out->type = TYPE_BOOL;
	else if ((tmp = parse_number(string, &out->value._number)))
		out->type = TYPE_NUMBER;
	else if ((tmp = parse_string(string, &out->value._string)))
		out->type = TYPE_STRING;
	else if ((tmp = parse_array(string, &out->value._array)))
		out->type = TYPE_ARRAY;
	else if ((tmp = parse_object(string, &out->value._object)))
		out->type = TYPE_OBJECT;

	return tmp;
}

static void free_value(JYValue *val) {
	if (val->type == TYPE_STRING)
		free(val->value._string);
	else if (val->type == TYPE_ARRAY)
		free_array(&val->value._array);
	else if (val->type == TYPE_OBJECT)
		free_object(&val->value._object);
}

static void free_array(JYArray *array) {
	size_t i;

	for (i = 0; i < array->count; ++i)
		free_value(&array->values[i]);
	free(array->values);
}

static void free_object(JYObject *object) {
	size_t i;

	for (i = 0; i < object->count; ++i) {
		free(object->pairs[i].key);
		free_value(&object->pairs[i].value);
	}

	free(object->pairs);
}
