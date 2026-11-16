/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_CODE_SSA_INTEGER_H
#define PIKE_CODE_SSA_INTEGER_H
#define PIKE_SSA_INTEGER_LOCALS 3
#define PIKE_SSA_INTEGER_LIMIT 8
#define PIKE_SSA_INTEGER_VALUES (PIKE_SSA_INTEGER_LOCALS + PIKE_SSA_INTEGER_LIMIT)

enum pikecode_ssa_integer_kind {
  PIKE_SSA_INTEGER_LOAD, PIKE_SSA_INTEGER_ADD, PIKE_SSA_INTEGER_SUBTRACT,
  PIKE_SSA_INTEGER_AND, PIKE_SSA_INTEGER_OR, PIKE_SSA_INTEGER_XOR,
  PIKE_SSA_INTEGER_CONSTANT, PIKE_SSA_INTEGER_NEGATE, PIKE_SSA_INTEGER_SUM
};
struct pikecode_ssa_integer_value {
  enum pikecode_ssa_integer_kind kind;
  unsigned int left, right;
  INT32 local, imm;
  unsigned int bank;
  unsigned int args, operand[PIKE_SSA_INTEGER_LIMIT];
};
struct pikecode_ssa_integer_unit {
  unsigned int first, length, count, result[2], dirty_before;
};
struct pikecode_ssa_integer_plan {
  unsigned int units, count, consumed, locals, values, dirty, loads;
  INT32 local[PIKE_SSA_INTEGER_LOCALS];
  unsigned int current[PIKE_SSA_INTEGER_LOCALS];
  struct pikecode_ssa_integer_unit unit[PIKE_SSA_INTEGER_LIMIT];
  struct pikecode_ssa_integer_value value[PIKE_SSA_INTEGER_VALUES + 1];
};

/* The stack extension retains bounded virtual operands. Constants need no
 * register; nonconstant versions still share three physical banks. Overflow
 * snapshots precede arithmetic while all operand versions remain resident.
 * Final snapshots may publish live stack operands.
 */
#define PIKE_SSA_INTEGER_STACK PIKE_SSA_INTEGER_LIMIT
#define PIKE_SSA_INTEGER_CODE 32
#define PIKE_SSA_INTEGER_STACK_VALUES \
  (PIKE_SSA_INTEGER_VALUES + PIKE_SSA_INTEGER_CODE)
struct pikecode_ssa_integer_snapshot {
  unsigned int local[PIKE_SSA_INTEGER_LOCALS];
  unsigned int stack[PIKE_SSA_INTEGER_STACK];
  unsigned int depth, dirty;
};
struct pikecode_ssa_integer_step {
  unsigned int first, result;
  struct pikecode_ssa_integer_snapshot before;
};
struct pikecode_ssa_integer_stack_plan {
  unsigned int locals, values, count, consumed, loads, writes, retained;
  unsigned int bitwise, normal, expressions;
  INT32 local[PIKE_SSA_INTEGER_LOCALS];
  struct pikecode_ssa_integer_value value[PIKE_SSA_INTEGER_STACK_VALUES + 1];
  struct pikecode_ssa_integer_step step[PIKE_SSA_INTEGER_LIMIT];
  struct pikecode_ssa_integer_snapshot final;
};

struct p_instr_s;
int pikecode_ssa_plan_integer(struct pikecode_ssa_integer_plan *plan,
                              const struct p_instr_s *code,
                              ptrdiff_t available, INT32 max_local);
int pikecode_ssa_plan_integer_stack(
    struct pikecode_ssa_integer_stack_plan *plan,
    const struct p_instr_s *code, ptrdiff_t available, INT32 max_local,
    int stack_exits, int subtract, int bitwise);
#endif
