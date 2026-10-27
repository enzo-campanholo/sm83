/* Minimal JSON reader, enough for the SingleStepTests files. */
#ifndef JSON_H
#define JSON_H

#include <stddef.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY, JSON_OBJECT } json_type;

typedef struct json json;
struct json {
    json_type type;
    double number;   /* JSON_NUMBER; JSON_BOOL as 0 or 1 */
    char *string;    /* JSON_STRING */
    size_t len;      /* JSON_ARRAY, JSON_OBJECT */
    json **items;    /* JSON_ARRAY, JSON_OBJECT */
    char **keys;     /* JSON_OBJECT */
};

/* Parses `text`. On error returns NULL and sets *err to a message. */
json *json_parse(const char *text, const char **err);
void json_free(json *v);

/* Member `key` of an object, or NULL. */
const json *json_get(const json *obj, const char *key);
/* Element `i` of an array, or NULL. */
const json *json_at(const json *arr, size_t i);

#endif
