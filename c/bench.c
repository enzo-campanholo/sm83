/* Performance criterion of the trial (proposal, table 5): the model, driven by
 * a C loop over a dummy bus, must run at least 4.2 million M-cycles per second.
 *
 * The bus is a flat 64 KB array. The program is a loop that uses every opcode
 * of the prototype; one iteration takes 29 M-cycles.
 */
#include "bench.h"

#include "sm83.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const uint8_t program[] = {
    /* 0100 */ 0x06, 0x00,       /* LD B,$00                          */
    /* 0102 */ 0x80,             /* loop: ADD A,B                     */
    /* 0103 */ 0x77,             /* LD (HL),A                         */
    /* 0104 */ 0x7E,             /* LD A,(HL)                         */
    /* 0105 */ 0xC6, 0x03,       /* ADD A,$03                         */
    /* 0107 */ 0xC5,             /* PUSH BC                           */
    /* 0108 */ 0xF1,             /* POP AF                            */
    /* 0109 */ 0xCD, 0x20, 0x01, /* CALL $0120                        */
    /* 010C */ 0x00,             /* NOP                               */
    /* 010D */ 0x04,             /* INC B                             */
    /* 010E */ 0x20, 0xF2,       /* JR NZ,loop                        */
    /* 0110 */ 0xC3, 0x00, 0x01, /* JP $0100                          */
};
static const uint8_t subroutine[] = {
    /* 0120 */ 0xC9,             /* RET                               */
};

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int bench_run(uint64_t mcycles)
{
    static uint8_t mem[0x10000];
    memset(mem, 0, sizeof mem);
    memcpy(mem + 0x0100, program, sizeof program);
    memcpy(mem + 0x0120, subroutine, sizeof subroutine);

    /* Post-boot DMG registers, except HL, which points into work RAM. */
    lean_object *cpu = sm83_new(0x01, 0xB0, 0x00, 0x13, 0x00, 0xD8, 0xC0, 0x00, 0xFFFE, 0x0100);
    uint64_t instructions = 0;

    double t0 = now();
    for (uint64_t i = 0; i < mcycles; i++) {
        uint32_t req = sm83_peek_request(cpu);
        uint8_t data = 0;
        switch (sm83_req_kind(req)) {
        case SM83_READ:
            data = mem[sm83_req_addr(req)];
            break;
        case SM83_WRITE:
            mem[sm83_req_addr(req)] = sm83_req_value(req);
            break;
        default:
            break;
        }
        instructions += (uint64_t)sm83_req_boundary(req);
        cpu = sm83_step(cpu, data, 0);
    }
    double elapsed = now() - t0;

    /* Printed so the work cannot be optimized away, and to check determinism
     * across runs and machines. */
    uint32_t sum = 0;
    for (uint8_t i = SM83_A; i <= SM83_LOCKED; i++)
        sum = sum * 31 + sm83_peek_reg(cpu, i);
    for (size_t a = 0; a < sizeof mem; a++)
        sum = sum * 31 + mem[a];
    int locked = sm83_peek_reg(cpu, SM83_LOCKED);
    lean_dec_ref(cpu);

    double rate = (double)mcycles / elapsed;
    printf("M-cycles:      %llu\n", (unsigned long long)mcycles);
    printf("instructions:  %llu\n", (unsigned long long)instructions);
    printf("time:          %.3f s\n", elapsed);
    printf("rate:          %.2f M-cycles/s (%.2fx DMG)\n", rate / 1e6, rate / 1048576.0);
    printf("checksum:      %08X\n", sum);
    printf("criterion:     %s (>= 4.2 M-cycles/s)\n", rate >= 4.2e6 ? "PASS" : "FAIL");
    if (locked) {
        fprintf(stderr, "error: the CPU locked during the benchmark\n");
        return 1;
    }
    return rate >= 4.2e6 ? 0 : 1;
}
