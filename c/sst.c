/* Runs SingleStepTests (sm83, v1 format) through the step interface.
 *
 * Each test sets up a flat 64 KB memory, runs one instruction by serving the
 * CPU's requests until the next opcode fetch, and compares:
 *   - the final registers (IME and IE are ignored, as the suite asks);
 *   - the whole memory against the test's final RAM (all other bytes zero);
 *   - the M-cycles: kind of each cycle, plus address and value for reads and
 *     writes. Idle cycles in the suite repeat the last bus address and value
 *     (an Ares detail, not hardware behaviour), so only their kind is compared.
 */
#include "sst.h"

#include "json.h"
#include "sm83.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CYCLES 16
#define MAX_REPORTED 5

typedef struct {
    int kind;
    uint16_t addr;
    uint8_t value;
} cycle;

static const char *const reg_names[] = { "a", "f", "b", "c", "d", "e", "h", "l", "sp", "pc" };

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    char *buf = NULL;
    if (fseek(f, 0, SEEK_END) == 0) {
        long n = ftell(f);
        if (n >= 0 && fseek(f, 0, SEEK_SET) == 0 && (buf = malloc((size_t)n + 1))) {
            if (fread(buf, 1, (size_t)n, f) == (size_t)n) {
                buf[n] = '\0';
            } else {
                free(buf);
                buf = NULL;
            }
        }
    }
    fclose(f);
    return buf;
}

static int num(const json *v, int *out)
{
    if (!v || v->type != JSON_NUMBER)
        return 0;
    *out = (int)v->number;
    return 1;
}

static int reg_value(const json *state, int i, int *out)
{
    return num(json_get(state, reg_names[i]), out);
}

/* Applies the "ram" list of `state` to `mem`. */
static int load_ram(const json *state, uint8_t *mem)
{
    const json *ram = json_get(state, "ram");
    if (!ram || ram->type != JSON_ARRAY)
        return 0;
    for (size_t i = 0; i < ram->len; i++) {
        int addr, val;
        if (!num(json_at(json_at(ram, i), 0), &addr) || !num(json_at(json_at(ram, i), 1), &val))
            return 0;
        mem[addr & 0xFFFF] = (uint8_t)val;
    }
    return 1;
}

static int parse_cycle(const json *c, cycle *out)
{
    const json *kind = json_at(c, 2);
    if (!kind || kind->type != JSON_STRING)
        return 0;
    int addr = 0, val = 0;
    num(json_at(c, 0), &addr);
    num(json_at(c, 1), &val);
    out->addr = (uint16_t)addr;
    out->value = (uint8_t)val;
    if (strcmp(kind->string, "r-m") == 0)
        out->kind = SM83_READ;
    else if (strcmp(kind->string, "-wm") == 0)
        out->kind = SM83_WRITE;
    else if (strcmp(kind->string, "---") == 0)
        out->kind = SM83_IDLE;
    else
        return 0;
    return 1;
}

static const char *kind_name(int k)
{
    static const char *const names[] = { "read", "write", "idle", "stopped" };
    return names[k & 3];
}

static void print_cycle(const char *who, size_t i, const cycle *c)
{
    if (c->kind == SM83_IDLE || c->kind == SM83_STOPPED)
        fprintf(stderr, "    %s cycle %zu: %s\n", who, i, kind_name(c->kind));
    else
        fprintf(stderr, "    %s cycle %zu: %s %04X %02X\n", who, i, kind_name(c->kind), c->addr, c->value);
}

/* Runs one test. Returns 1 if it passes; otherwise prints why when `report`. */
static int run_test(const json *t, int report)
{
    static uint8_t mem[0x10000], expected[0x10000];
    const json *name = json_get(t, "name");
    const json *init = json_get(t, "initial");
    const json *fin = json_get(t, "final");
    const json *cycles = json_get(t, "cycles");
    const char *tname = name && name->type == JSON_STRING ? name->string : "?";

    int r[10], want_r[10];
    for (int i = 0; i < 10; i++)
        if (!reg_value(init, i, &r[i]) || !reg_value(fin, i, &want_r[i])) {
            fprintf(stderr, "  %s: malformed test\n", tname);
            return 0;
        }
    memset(mem, 0, sizeof mem);
    memset(expected, 0, sizeof expected);
    if (!load_ram(init, mem) || !load_ram(fin, expected) || !cycles || cycles->type != JSON_ARRAY) {
        fprintf(stderr, "  %s: malformed test\n", tname);
        return 0;
    }

    lean_object *cpu = sm83_new((uint8_t)r[0], (uint8_t)r[1], (uint8_t)r[2], (uint8_t)r[3], (uint8_t)r[4],
                                (uint8_t)r[5], (uint8_t)r[6], (uint8_t)r[7], (uint16_t)r[8], (uint16_t)r[9]);
    cycle got[MAX_CYCLES];
    size_t n = 0;
    int finished = 0, locked = 0;
    for (;;) {
        uint32_t req = sm83_peek_request(cpu);
        if (sm83_req_kind(req) == SM83_STOPPED) {
            locked = 1;
            break;
        }
        if (n > 0 && sm83_req_boundary(req)) {
            finished = 1;
            break;
        }
        if (n == MAX_CYCLES)
            break;
        cycle *c = &got[n++];
        c->kind = (int)sm83_req_kind(req);
        c->addr = sm83_req_addr(req);
        c->value = 0;
        uint8_t data = 0;
        if (c->kind == SM83_READ) {
            data = mem[c->addr];
            c->value = data;
        } else if (c->kind == SM83_WRITE) {
            c->value = sm83_req_value(req);
            mem[c->addr] = c->value;
        }
        cpu = sm83_step(cpu, data, 0);
    }

    int ok = 1;
    if (!finished) {
        ok = 0;
        if (report)
            fprintf(stderr, "  %s: %s\n", tname,
                    locked ? "CPU locked (invalid or unimplemented opcode)" : "instruction did not finish");
    } else {
        for (int i = 0; i < 10; i++) {
            int want = want_r[i];
            int have = sm83_peek_reg(cpu, (uint8_t)i);
            if (have != want) {
                if (report && ok)
                    fprintf(stderr, "  %s:\n", tname);
                if (report)
                    fprintf(stderr, "    %s = %02X, expected %02X\n", reg_names[i], have, want);
                ok = 0;
            }
        }
        for (size_t a = 0; a < sizeof mem; a++) {
            if (mem[a] != expected[a]) {
                if (report && ok)
                    fprintf(stderr, "  %s:\n", tname);
                if (report)
                    fprintf(stderr, "    mem[%04zX] = %02X, expected %02X\n", a, mem[a], expected[a]);
                ok = 0;
            }
        }
        int cycles_ok = n == cycles->len;
        for (size_t i = 0; cycles_ok && i < n; i++) {
            cycle want;
            if (!parse_cycle(json_at(cycles, i), &want)) {
                cycles_ok = 0;
                break;
            }
            cycles_ok = want.kind == got[i].kind &&
                        (want.kind == SM83_IDLE || (want.addr == got[i].addr && want.value == got[i].value));
        }
        if (!cycles_ok) {
            if (report && ok)
                fprintf(stderr, "  %s:\n", tname);
            if (report) {
                fprintf(stderr, "    bus cycles differ (%zu, expected %zu)\n", n, cycles->len);
                for (size_t i = 0; i < n; i++)
                    print_cycle("model", i, &got[i]);
                for (size_t i = 0; i < cycles->len; i++) {
                    cycle want = { 0 };
                    if (parse_cycle(json_at(cycles, i), &want))
                        print_cycle("test ", i, &want);
                }
            }
            ok = 0;
        }
    }
    lean_dec_ref(cpu);
    return ok;
}

int sst_run_file(const char *path)
{
    char *text = read_file(path);
    if (!text) {
        fprintf(stderr, "%s: cannot read file\n", path);
        return -1;
    }
    const char *err = NULL;
    json *tests = json_parse(text, &err);
    free(text);
    if (!tests || tests->type != JSON_ARRAY) {
        fprintf(stderr, "%s: invalid JSON (%s)\n", path, err ? err : "not an array");
        json_free(tests);
        return -1;
    }
    size_t failed = 0;
    for (size_t i = 0; i < tests->len; i++)
        if (!run_test(tests->items[i], failed < MAX_REPORTED))
            failed++;
    printf("%s: %zu/%zu passed\n", path, tests->len - failed, tests->len);
    json_free(tests);
    return failed == 0 ? 0 : 1;
}
