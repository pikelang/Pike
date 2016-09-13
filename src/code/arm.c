/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Common parts of both the 32 and 64 bit arm code generation */

#include "global.h"
#include "svalue.h"
#include "operators.h"
#include "bitvector.h"
#include "object.h"
#include "builtin_functions.h"
#include "bignum.h"
#include "constants.h"

#define MACRO  ATTRIBUTE((unused)) static
#define OPCODE_FUN ATTRIBUTE((unused,warn_unused_result)) static PIKE_OPCODE_T

MACRO void break_my_arm(void) {
}

#ifdef ARM_LOW_DEBUG
static unsigned INT32 stats_m[F_MAX_INSTR - F_OFFSET];
static unsigned INT32 stats_b[F_MAX_INSTR - F_OFFSET];

#define OPCODE0(X,Y,F) case X: return #X;
#define OPCODE1(X,Y,F) case X: return #X;
#define OPCODE2(X,Y,F) case X: return #X;
#define OPCODE0_TAIL(X,Y,F) case X: return #X;
#define OPCODE1_TAIL(X,Y,F) case X: return #X;
#define OPCODE2_TAIL(X,Y,F) case X: return #X;
#define OPCODE0_JUMP(X,Y,F) case X: return #X;
#define OPCODE1_JUMP(X,Y,F) case X: return #X;
#define OPCODE2_JUMP(X,Y,F) case X: return #X;
#define OPCODE0_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE1_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE2_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE0_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE1_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE2_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE0_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE1_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE2_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE0_RETURN(X,Y,F) case X: return #X;
#define OPCODE1_RETURN(X,Y,F) case X: return #X;
#define OPCODE2_RETURN(X,Y,F) case X: return #X;
#define OPCODE0_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE1_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE2_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE0_BRANCH(X,Y,F) case X: return #X;
#define OPCODE1_BRANCH(X,Y,F) case X: return #X;
#define OPCODE2_BRANCH(X,Y,F) case X: return #X;
#define OPCODE0_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE1_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE2_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE0_ALIAS(X,Y,F,A) case X: return #X;
#define OPCODE1_ALIAS(X,Y,F,A) case X: return #X;
#define OPCODE2_ALIAS(X,Y,F,A) case X: return #X;

const char* arm_get_opcode_name(PIKE_OPCODE_T code) {
    switch (code+F_OFFSET) {
#include "interpret_protos.h"
    default:
        return "<unknown>";
    }
}

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL
#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP
#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN
#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP
#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS

ATTRIBUTE((destructor))
MACRO void write_stats() {
    int i;
    FILE* file = fopen("/home/el/opcode.state", "a");

    for (i = 0; i < F_MAX_INSTR - F_OFFSET; i++) {
        if (!stats_m[i] && !stats_b[i]) continue;

        fprintf(file, "%s\t%u\t%u\n", arm_get_opcode_name(i), stats_b[i], stats_m[i] - stats_b[i]);
    }

    fclose(file);
}
#endif

MACRO void record_opcode(PIKE_OPCODE_T code, int bytecode) {
    code = code; /* prevent unused warning */
    bytecode = bytecode;
#ifdef ARM_LOW_DEBUG
    if (bytecode) {
        stats_b[code - F_OFFSET] ++;
    } else {
        stats_m[code - F_OFFSET] ++;
    }
#endif
}

#define FLAG_SP_LOADED  1
#define FLAG_FP_LOADED  2
#define FLAG_LOCALS_LOADED 4
#define FLAG_GLOBALS_LOADED 8
#define FLAG_NOT_DESTRUCTED 16

struct location_list_entry {
    unsigned INT32 location;
    struct location_list_entry *next;
};

struct compiler_state {
    /* currently unused and dirt registers */
    unsigned INT32 free, dirt;
    /* address into which the initial stack push has to be
     * generated
     */
    unsigned INT32 flags;
} compiler_state;


static struct location_list_entry* add_to_list(struct location_list_entry *list, unsigned INT32 location) {
    struct location_list_entry *e = xalloc(sizeof(struct location_list_entry));

    e->location = location;
    e->next = list;

    return e;
}

static void free_list(struct location_list_entry *list) {
    while (list) {
        struct location_list_entry * next = list->next;
        free(list);
        list = next;
    }
}

struct label {
    struct location_list_entry *list;
    unsigned INT32 loc;
};

static void label_init(struct label *l) {
    l->list = NULL;
    l->loc = (unsigned INT32)-1;
}

MACRO INT32 label_dist(struct label *l) {
    if (l->loc != (unsigned INT32)-1)
        return l->loc - PIKE_PC;
    l->list = add_to_list(l->list, PIKE_PC);
    return 0;
}

/* Register allocator */

MACRO unsigned INT32 RBIT(enum arm_register reg) {
    return 1U<<reg;
}

MACRO void ra_init(void) {
    /* FIXME: this ought to happen in init_interpreter_state, only, but it is currently overwritten
     * later */
    instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;

    compiler_state.free = arm_alloc_registers;
    compiler_state.dirt = 0;
    compiler_state.flags = 0;
}

MACRO enum arm_register ra_alloc(enum arm_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    if (!(rbit & compiler_state.free))
        Pike_fatal("Register %d is already in use.\n", reg);

    compiler_state.free ^= rbit;
    compiler_state.dirt |= rbit;

    return reg;
}

MACRO enum arm_register ra_alloc_persistent(void) {
    unsigned INT32 free = compiler_state.free;

    /* we dont want a temporary register */
    free &= ~arm_temp_registers;

    if (!free)
        Pike_fatal("No register left: %x\n", compiler_state.free);

    return ra_alloc(ctz32(free));
}

MACRO enum arm_register ra_alloc_any(void) {
    if (!compiler_state.free)
        Pike_fatal("No register left.\n");

    return ra_alloc(ctz32(compiler_state.free));
}

MACRO void ra_free(enum arm_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    if (rbit & compiler_state.free)
        Pike_fatal("Register %d is not in use.\n", reg);

    compiler_state.free |= rbit;
}

MACRO int ra_is_dirty(enum arm_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    return !!(rbit & compiler_state.dirt);
}

MACRO int ra_is_free(enum arm_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    return !!(rbit & compiler_state.free);
}
