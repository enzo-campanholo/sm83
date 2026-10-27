#include "json.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *p;
    const char *err;
} parser;

static json *parse_value(parser *ps, int depth);

static void skip_ws(parser *ps)
{
    while (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r')
        ps->p++;
}

static json *new_value(json_type t)
{
    json *v = calloc(1, sizeof *v);
    if (v)
        v->type = t;
    return v;
}

static int push(json *v, json *item, char *key)
{
    json **items = realloc(v->items, (v->len + 1) * sizeof *items);
    if (!items)
        return 0;
    v->items = items;
    if (v->type == JSON_OBJECT) {
        char **keys = realloc(v->keys, (v->len + 1) * sizeof *keys);
        if (!keys)
            return 0;
        v->keys = keys;
        v->keys[v->len] = key;
    }
    v->items[v->len++] = item;
    return 1;
}

/* Strings without escapes beyond \" \\ \/ and the single-letter ones; the
 * test files only use plain ASCII. */
static char *parse_string(parser *ps)
{
    if (*ps->p != '"') {
        ps->err = "expected string";
        return NULL;
    }
    const char *start = ++ps->p;
    size_t n = 0;
    for (const char *q = start; *q != '"'; q++, n++) {
        if (*q == '\0') {
            ps->err = "unterminated string";
            return NULL;
        }
        if (*q == '\\' && q[1] != '\0')
            q++;
    }
    char *s = malloc(n + 1);
    if (!s) {
        ps->err = "out of memory";
        return NULL;
    }
    size_t i = 0;
    while (*ps->p != '"') {
        char ch = *ps->p++;
        if (ch == '\\') {
            ch = *ps->p++;
            switch (ch) {
            case 'n': ch = '\n'; break;
            case 't': ch = '\t'; break;
            case 'r': ch = '\r'; break;
            case 'b': ch = '\b'; break;
            case 'f': ch = '\f'; break;
            case '"': case '\\': case '/': break;
            default:
                free(s);
                ps->err = "unsupported escape";
                return NULL;
            }
        }
        s[i++] = ch;
    }
    s[i] = '\0';
    ps->p++;
    return s;
}

static json *parse_container(parser *ps, int depth, json_type t)
{
    char close = t == JSON_ARRAY ? ']' : '}';
    json *v = new_value(t);
    if (!v) {
        ps->err = "out of memory";
        return NULL;
    }
    ps->p++;
    skip_ws(ps);
    if (*ps->p == close) {
        ps->p++;
        return v;
    }
    for (;;) {
        char *key = NULL;
        skip_ws(ps);
        if (t == JSON_OBJECT) {
            key = parse_string(ps);
            if (!key)
                goto fail;
            skip_ws(ps);
            if (*ps->p != ':') {
                free(key);
                ps->err = "expected ':'";
                goto fail;
            }
            ps->p++;
        }
        json *item = parse_value(ps, depth + 1);
        if (!item || !push(v, item, key)) {
            free(key);
            json_free(item);
            if (!ps->err)
                ps->err = "out of memory";
            goto fail;
        }
        skip_ws(ps);
        if (*ps->p == ',') {
            ps->p++;
        } else if (*ps->p == close) {
            ps->p++;
            return v;
        } else {
            ps->err = t == JSON_ARRAY ? "expected ',' or ']'" : "expected ',' or '}'";
            goto fail;
        }
    }
fail:
    json_free(v);
    return NULL;
}

static json *parse_value(parser *ps, int depth)
{
    if (depth > 64) {
        ps->err = "nesting too deep";
        return NULL;
    }
    skip_ws(ps);
    char ch = *ps->p;
    if (ch == '[')
        return parse_container(ps, depth, JSON_ARRAY);
    if (ch == '{')
        return parse_container(ps, depth, JSON_OBJECT);
    if (ch == '"') {
        char *s = parse_string(ps);
        if (!s)
            return NULL;
        json *v = new_value(JSON_STRING);
        if (!v) {
            free(s);
            ps->err = "out of memory";
            return NULL;
        }
        v->string = s;
        return v;
    }
    if (strncmp(ps->p, "null", 4) == 0) {
        ps->p += 4;
        return new_value(JSON_NULL);
    }
    if (strncmp(ps->p, "true", 4) == 0 || strncmp(ps->p, "false", 5) == 0) {
        json *v = new_value(JSON_BOOL);
        if (v)
            v->number = ch == 't';
        ps->p += ch == 't' ? 4 : 5;
        return v;
    }
    if (ch == '-' || (ch >= '0' && ch <= '9')) {
        char *end;
        double d = strtod(ps->p, &end);
        ps->p = end;
        json *v = new_value(JSON_NUMBER);
        if (v)
            v->number = d;
        return v;
    }
    ps->err = "unexpected character";
    return NULL;
}

json *json_parse(const char *text, const char **err)
{
    parser ps = { text, NULL };
    json *v = parse_value(&ps, 0);
    if (v) {
        skip_ws(&ps);
        if (*ps.p != '\0') {
            json_free(v);
            v = NULL;
            ps.err = "trailing characters";
        }
    }
    if (!v && err)
        *err = ps.err ? ps.err : "out of memory";
    return v;
}

void json_free(json *v)
{
    if (!v)
        return;
    for (size_t i = 0; i < v->len; i++) {
        json_free(v->items[i]);
        if (v->keys)
            free(v->keys[i]);
    }
    free(v->items);
    free(v->keys);
    free(v->string);
    free(v);
}

const json *json_get(const json *obj, const char *key)
{
    if (!obj || obj->type != JSON_OBJECT)
        return NULL;
    for (size_t i = 0; i < obj->len; i++)
        if (strcmp(obj->keys[i], key) == 0)
            return obj->items[i];
    return NULL;
}

const json *json_at(const json *arr, size_t i)
{
    if (!arr || arr->type != JSON_ARRAY || i >= arr->len)
        return NULL;
    return arr->items[i];
}
