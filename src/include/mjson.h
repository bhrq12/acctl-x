/*
 * =====================================================================================
 *       Filename:  mjson.h — MicroJSON declarations (matches mjson.c)
 *       Author:    Eric S. Raymond (BSD License)
 *       Modified:  jianxi sun
 * =====================================================================================
 */
#ifndef __MJSON_H__
#define __MJSON_H__

#include <stdbool.h>

/* Attribute type tags */
enum json_type {
	t_null, t_boolean, t_string, t_integer, t_uinteger,
	t_time, t_real, t_character, t_array, t_object,
	t_structobject, t_check, t_ignore,
};

/* Single attribute specification for JSON parsing */
struct json_attr_t {
	const char *attribute;   /* JSON attribute name */
	enum json_type type;     /* Expected value type */
	union {
		int *integer;
		unsigned int *uinteger;
		double *real;
		char *string;
		bool *boolean;
		char *character;
		struct json_array_t *array;
		const char *check;
	} addr;                 /* Where to store parsed value */
	union {
		int integer;
		unsigned int uinteger;
		double real;
		const char *string;
		bool boolean;
		char character;
	} dflt;                 /* Default value if attr missing */
	size_t len;              /* For strings: max length */
	const struct json_enum_t *map; /* For enum-to-string mapping */
	int nodefault;           /* If set: don't use dflt when attr missing */
};

/* Array specification */
struct json_array_t {
	enum json_type element_type;
	union {
		struct {
			const struct json_attr_t *subtype;
			char *store;
			size_t storelen;
			char **ptrs[1];
		} strings;
		int *integers;
		unsigned int *uintegers;
		double *reals;
		bool *boaleans;
	} arr;
	int maxlen;     /* Maximum number of elements */
	int *count;     /* Output: actual element count written */
};

/* Enumeration mapping: string name → integer value */
struct json_enum_t {
	const char *name;
	int value;
};

/* Parse a JSON object into C structs via attrs template.
 * @cp: JSON string to parse
 * @attrs: Attribute spec array (last entry: attribute==NULL)
 * @end: (output) pointer to first unparsed char (may be NULL)
 * Returns: 0 on success, negative on error
 */
int json_read_object(const char *cp, const struct json_attr_t *attrs,
	const char **end);

/* Parse a JSON array into parallel C arrays.
 * @cp: JSON string to parse
 * @arr: Array spec with element type and output pointers
 * @end: (output) pointer to first unparsed char (may be NULL)
 * Returns: 0 on success, negative on error
 */
int json_read_array(const char *cp, const struct json_array_t *arr,
	const char **end);

/* Return human-readable error string for json_read_* error codes */
const char *json_error_string(int err);

#endif /* __MJSON_H__ */
