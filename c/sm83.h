/* Interface to the Lean model of the SM83 (Sm83/Ffi.lean). */
#ifndef SM83_API_H
#define SM83_API_H

#include <stdint.h>
#include <lean/lean.h>

/* Exported from Lean. Every function takes ownership of `cpu`. */
lean_object *sm83_new(uint8_t a, uint8_t f, uint8_t b, uint8_t c, uint8_t d,
                      uint8_t e, uint8_t h, uint8_t l, uint16_t sp, uint16_t pc);
lean_object *sm83_step(lean_object *cpu, uint8_t data, uint8_t irq);
uint32_t sm83_request(lean_object *cpu);
uint16_t sm83_reg(lean_object *cpu, uint8_t i);

/* Lean runtime (not declared in lean.h) and the initializer Lake generates
 * for the `Sm83` library. */
void lean_initialize_runtime_module(void);
lean_object *initialize_sm83_Sm83(uint8_t builtin);

/* Request encoding (Cpu.request in Sm83/Step.lean). */
enum { SM83_READ = 0, SM83_WRITE = 1, SM83_IDLE = 2, SM83_STOPPED = 3 };

static inline unsigned sm83_req_kind(uint32_t r) { return (r >> 24) & 3; }
static inline uint16_t sm83_req_addr(uint32_t r) { return (uint16_t)r; }
static inline uint8_t sm83_req_value(uint32_t r) { return (uint8_t)(r >> 16); }
/* The request is the opcode fetch of a new instruction. */
static inline int sm83_req_boundary(uint32_t r) { return (r >> 26) & 1; }

/* Register indices for sm83_reg; values are those at the last instruction boundary. */
enum { SM83_A, SM83_F, SM83_B, SM83_C, SM83_D, SM83_E, SM83_H, SM83_L,
       SM83_SP, SM83_PC, SM83_LOCKED };

/* Getters that leave the caller's reference intact. */
static inline uint32_t sm83_peek_request(lean_object *cpu)
{
    lean_inc_ref(cpu);
    return sm83_request(cpu);
}

static inline uint16_t sm83_peek_reg(lean_object *cpu, uint8_t i)
{
    lean_inc_ref(cpu);
    return sm83_reg(cpu, i);
}

/* Starts the Lean runtime and the model. Returns 0 on success. */
static inline int sm83_init(void)
{
    lean_initialize_runtime_module();
    lean_object *res = initialize_sm83_Sm83(1);
    if (!lean_io_result_is_ok(res)) {
        lean_io_result_show_error(res);
        lean_dec_ref(res);
        return 1;
    }
    lean_dec_ref(res);
    lean_io_mark_end_initialization();
    return 0;
}

#endif
