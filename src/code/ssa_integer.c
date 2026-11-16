/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "constants.h"
#include "operators.h"

/* Included by pikecode.c. These planners contain no native registers,
 * instruction encodings or branch fixups. The balanced planner coalesces
 * each local's versions; the stack planner shares banks by liveness. */
#ifdef CODE_GENERATOR_INTEGER_RUN
#include "ssa_integer_profile.c"

/* Source records follow the caller's per-function setting. */
static void pikecode_ssa_integer_source(const struct p_instr_s *op,
                                       int store_lines)
{
  if (store_lines) store_linenumber(op->line, op->file);
}

static int pikecode_ssa_integer_multiline(const struct p_instr_s *code,
                                         unsigned int consumed)
{
  unsigned int i;
  for (i = 1; i < consumed; i++)
    if (code[i].line != code[0].line) return 1;
  return 0;
}

#endif

/* Each unit starts and ends at the same Pike stack depth. Its fast path
 * writes one local. Multi-step units can replay from their beginning after
 * restoring that local's input register: no partial result was published. */
static int pikecode_ssa_integer_unit(const struct p_instr_s *c,
                                    ptrdiff_t available, INT32 *dst,
                                    INT32 *src, INT32 imm[2],
                                    unsigned int *count)
{
  *dst = c->arg;
  *src = -1;
  *count = 1;
  switch (c->opcode) {
  case F_INC_LOCAL_AND_POP: imm[0] = 1; return 1;
  case F_DEC_LOCAL_AND_POP: imm[0] = -1; return 1;
  case F_ADD_LOCAL_INT_AND_POP: imm[0] = c->arg2; return 1;
  case F_ADD_LOCALS_AND_POP:
    *src = c->arg2;
    return *src >= 0;
  }
  if (available < 3 || c[2].opcode != F_ASSIGN_LOCAL_AND_POP ||
      c[2].arg != *dst) return 0;
  if (c->opcode == F_2_LOCALS && c[1].opcode == F_ADD_INTS) {
    *src = c->arg2;
    return *src < 0 ? 0 : 3;
  }
  if (c->opcode != F_LOCAL && c->opcode != F_INC_LOCAL &&
      c->opcode != F_DEC_LOCAL) return 0;
  if (c[1].opcode != F_ADD_INT && c[1].opcode != F_ADD_NEG_INT) return 0;
  if (c[1].opcode == F_ADD_NEG_INT && INT32_NEG_OVERFLOW(c[1].arg))
    return 0;
  imm[0] = c[1].opcode == F_ADD_INT ? c[1].arg : -c[1].arg;
  if (c->opcode != F_LOCAL) {
    imm[1] = imm[0];
    imm[0] = c->opcode == F_INC_LOCAL ? 1 : -1;
    *count = 2;
  }
  return 3;
}

static int pikecode_ssa_integer_local(struct pikecode_ssa_integer_plan *p,
                                     INT32 local, INT32 max_local)
{
  struct pikecode_ssa_integer_value *v;
  unsigned int i;
  for (i = 0; i < p->locals; i++)
    if (p->local[i] == local) return i;
  if (p->locals == PIKE_SSA_INTEGER_LOCALS || local < 0 ||
      local > max_local)
    return -1;
  p->local[i] = local;
  p->current[i] = ++p->values;
  v = &p->value[p->values];
  v->kind = PIKE_SSA_INTEGER_LOAD;
  v->local = local;
  v->bank = i;
  p->locals++;
  return i;
}

int pikecode_ssa_plan_integer(struct pikecode_ssa_integer_plan *p,
                              const struct p_instr_s *code,
                              ptrdiff_t available, INT32 max_local)
{
  unsigned int i, j;
  if (available <= 0) return 0;
  switch (code[0].opcode) {
  case F_INC_LOCAL_AND_POP: case F_DEC_LOCAL_AND_POP:
  case F_ADD_LOCAL_INT_AND_POP: case F_ADD_LOCALS_AND_POP:
  case F_LOCAL: case F_2_LOCALS: case F_INC_LOCAL: case F_DEC_LOCAL:
    break;
  default:
    return 0;
  }
  memset(p, 0, sizeof(*p));
  while (p->count < PIKE_SSA_INTEGER_LIMIT &&
         (ptrdiff_t)p->consumed < available) {
    const struct p_instr_s *op = &code[p->consumed];
    struct pikecode_ssa_integer_unit *u = &p->unit[p->units];
    unsigned int steps;
    INT32 local, source, imm[2];
    int dst, src = -1;
    int length = pikecode_ssa_integer_unit(op, available - p->consumed,
                                           &local, &source, imm, &steps);
    if (!length || p->count + steps > PIKE_SSA_INTEGER_LIMIT) break;
    for (i = 0; i < (unsigned int)length; i++)
      if (op[i].file != code[0].file) break;
    if (i != (unsigned int)length) break;
    dst = pikecode_ssa_integer_local(p, local, max_local);
    if (dst < 0) break;
    if (source >= 0) {
      src = pikecode_ssa_integer_local(p, source, max_local);
      if (src < 0) break;
    }
    u->first = p->consumed;
    u->length = length;
    u->count = steps;
    u->dirty_before = p->dirty;
    for (i = 0; i < steps; i++) {
      struct pikecode_ssa_integer_value *v = &p->value[++p->values];
      v->kind = PIKE_SSA_INTEGER_ADD;
      v->left = p->current[dst];
      v->right = src < 0 ? 0 : p->current[src];
      v->local = local;
      v->imm = src < 0 ? imm[i] : 0;
      v->bank = p->value[v->left].bank;
      p->current[dst] = p->values;
      u->result[i] = p->values;
      p->loads += src < 0 ? 1 : 2;
    }
    p->dirty |= 1U << dst;
    p->count += steps;
    p->consumed += length;
    p->units++;
  }
  /* Do not speculate on untouched locals from an operation that exceeded
   * the register budget. A single value operation has nothing to reuse. */
  if (p->count < 2) return 0;
  for (i = 0; i < p->locals; i++) {
    int used = 0;
    for (j = 0; j < p->units; j++) {
      struct pikecode_ssa_integer_value *v = &p->value[p->unit[j].result[0]];
      if (p->value[v->left].local == p->local[i] ||
          (v->right && p->value[v->right].local == p->local[i])) used = 1;
    }
    if (!used) return 0;
  }
  if (p->loads <= p->locals) return 0;
  return p->consumed;
}

/* Retained assignments and live-stack exits use a separate plan so the
 * original balanced-unit planner remains a register-budget fallback.
 * Three physical banks are shared by local versions and virtual stack values.
 * A bank may be reused only after its last local/stack reference disappears.
 * The result is copied into that bank after the overflow branch, so the
 * before-snapshot still names intact operands on every side exit.
 */
static int integer_stack_local(struct pikecode_ssa_integer_stack_plan *p,
                               INT32 local, INT32 max_local)
{
  unsigned int i;
  for (i = 0; i < p->locals; i++)
    if (p->local[i] == local) return i;
  if (local < 0 || local > max_local ||
      p->locals == PIKE_SSA_INTEGER_LOCALS) return -1;
  p->local[p->locals++] = local;
  return p->locals - 1;
}

/* Logical operations on native integers stay within the native width.
 * Native lowerings use this predicate to omit impossible overflow exits. */
static int integer_stack_bitwise(enum pikecode_ssa_integer_kind kind)
{
  return kind == PIKE_SSA_INTEGER_AND || kind == PIKE_SSA_INTEGER_OR ||
    kind == PIKE_SSA_INTEGER_XOR;
}

/* Only the immutable builtin addition is eligible, never an arbitrary call.
 * Arguments are native integers guarded at region entry. A failed partial
 * sum publishes the original argument snapshot and replays the entire call.
 */
static int integer_stack_expressions(void)
{
  static int enabled = -1;
  if (enabled < 0)
    enabled = getenv("PIKE_SSA_NO_INTEGER_EXPRESSIONS") == NULL;
  return enabled;
}

static int integer_stack_sum(const struct p_instr_s *op)
{
  const struct svalue *f;
  struct program *p;
  if (op->opcode != F_CALL_BUILTIN_N || op->arg2 < 2 ||
      op->arg2 > PIKE_SSA_INTEGER_LIMIT || !Pike_compiler) return 0;
  p = Pike_compiler->new_program;
  if (op->arg < 0 || op->arg >= p->num_constants) return 0;
  f = &p->constants[op->arg].sval;
  return TYPEOF(*f) == PIKE_T_FUNCTION &&
    SUBTYPEOF(*f) == FUNCTION_BUILTIN && f->u.efun->function == f_add;
}

static int integer_stack_constant(const struct p_instr_s *op, INT32 *imm)
{
  switch (op->opcode) {
  case F_CONST0: *imm = 0; return 1;
  case F_CONST1: *imm = 1; return 1;
  case F_CONST_1: *imm = -1; return 1;
  case F_NUMBER: *imm = op->arg; return 1;
  case F_NEG_NUMBER:
    if (INT32_NEG_OVERFLOW(op->arg)) return 0;
    *imm = -op->arg; return 1;
  }
  return 0;
}

static int integer_stack_refs(const struct p_instr_s *op,
                               INT32 *a, INT32 *b, int subtract, int bitwise)
{
  INT32 imm;
  *a = *b = -1;
  if (integer_stack_expressions() &&
      (op->opcode == F_NEGATE || integer_stack_constant(op, &imm) ||
       integer_stack_sum(op))) return 1;
  switch (op->opcode) {
  case F_2_LOCALS: case F_ADD_LOCALS_AND_POP:
    *b = op->arg2;
    if (*b < 0) return 0;
    /* FALLTHRU */
  case F_LOCAL: case F_INC_LOCAL: case F_DEC_LOCAL:
  case F_INC_LOCAL_AND_POP: case F_DEC_LOCAL_AND_POP:
  case F_ASSIGN_LOCAL: case F_ASSIGN_LOCAL_AND_POP:
  case F_ADD_LOCAL_INT_AND_POP:
    *a = op->arg;
    return *a >= 0;
  case F_ADD_INTS: case F_ADD_INT: case F_POP_VALUE:
    return 1;
  case F_AND: case F_OR: case F_XOR:
  case F_AND_INT: case F_OR_INT: case F_XOR_INT:
    return bitwise;
  case F_SUBTRACT:
    return subtract;
  case F_SUBTRACT_INT:
    return subtract || !INT32_NEG_OVERFLOW(op->arg);
  case F_ADD_NEG_INT:
    return !INT32_NEG_OVERFLOW(op->arg);
  }
  return 0;
}

int pikecode_ssa_plan_integer_stack(
    struct pikecode_ssa_integer_stack_plan *p,
    const struct p_instr_s *code, ptrdiff_t available, INT32 max_local,
    int stack_exits, int subtract, int bitwise)
{
  struct pikecode_ssa_integer_snapshot state;
  struct pikecode_ssa_integer_stack_plan best;
  unsigned int stack_limit = integer_stack_expressions() ?
    PIKE_SSA_INTEGER_STACK : 2;
  unsigned int n, e, i, arithmetic = 0, retained = 0, used = 0, best_used = 0;
  if (available <= 0) return 0;
  switch (code[0].opcode) {
  case F_LOCAL: case F_2_LOCALS: case F_INC_LOCAL: case F_DEC_LOCAL:
  case F_INC_LOCAL_AND_POP: case F_DEC_LOCAL_AND_POP:
  case F_ADD_LOCAL_INT_AND_POP: case F_ADD_LOCALS_AND_POP:
    break;
  default: return 0;
  }
  p->locals = 0;
  /* All guards run at entry. Discover initial local versions before any
   * temporary can borrow a bank, including locals first read later. */
  for (n = 0; n < PIKE_SSA_INTEGER_CODE && (ptrdiff_t)n < available; n++) {
    const struct p_instr_s *op = &code[n];
    INT32 a, b;
    int is_arithmetic;
    INT32 imm;
    if (op->file != code[0].file ||
        !integer_stack_refs(op, &a, &b, subtract, bitwise)) break;
    is_arithmetic = op->opcode != F_LOCAL && op->opcode != F_2_LOCALS &&
      op->opcode != F_ASSIGN_LOCAL && op->opcode != F_ASSIGN_LOCAL_AND_POP &&
      op->opcode != F_POP_VALUE && !integer_stack_constant(op, &imm);
    /* Count every native add in an n-ary operation against the same budget. */
    if (op->opcode == F_CALL_BUILTIN_N) is_arithmetic = op->arg2 - 1;
    if (arithmetic + is_arithmetic > PIKE_SSA_INTEGER_LIMIT) break;
    if (a >= 0 && integer_stack_local(p, a, max_local) < 0) break;
    if (b >= 0 && integer_stack_local(p, b, max_local) < 0) break;
    arithmetic += is_arithmetic;
    retained |= op->opcode == F_ASSIGN_LOCAL;
  }
  if ((!retained && !stack_exits && !bitwise) || arithmetic < 2) return 0;
  p->values = p->count = p->consumed = p->loads = p->writes = p->retained = 0;
  p->bitwise = p->normal = p->expressions = 0;
  memset(p->value, 0, sizeof(p->value));
  memset(p->step, 0, sizeof(p->step));
  memset(&state, 0, sizeof(state));
  memset(&best, 0, sizeof(best));
  for (i = 0; i < p->locals; i++) {
    struct pikecode_ssa_integer_value *v = &p->value[++p->values];
    v->kind = PIKE_SSA_INTEGER_LOAD;
    v->local = p->local[i];
    v->bank = i;
    state.local[i] = p->values;
  }
  for (e = 0; e < n; e++) {
    const struct p_instr_s *op = &code[e];
    struct pikecode_ssa_integer_snapshot before = state;
    unsigned int left = 0, right = 0, push = 0, write = 0, live = 0, bank;
    INT32 imm = 0, a, b;
    enum pikecode_ssa_integer_kind kind = PIKE_SSA_INTEGER_ADD;
    int dst = -1, src = -1;
    unsigned int args = 0, operand[PIKE_SSA_INTEGER_LIMIT];
    integer_stack_refs(op, &a, &b, subtract, bitwise);
    for (i = 0; i < p->locals; i++) {
      if (p->local[i] == a) dst = i;
      if (p->local[i] == b) src = i;
    }
    if (integer_stack_constant(op, &imm)) {
      struct pikecode_ssa_integer_value *v;
      if (state.depth == stack_limit) goto finished;
      v = &p->value[++p->values];
      v->kind = PIKE_SSA_INTEGER_CONSTANT;
      v->imm = imm;
      state.stack[state.depth++] = p->values;
      goto accepted;
    }
    switch (op->opcode) {
    case F_NEGATE:
      if (!state.depth) goto finished;
      left = state.stack[--state.depth];
      kind = PIKE_SSA_INTEGER_NEGATE;
      push = 1;
      break;
    case F_CALL_BUILTIN_N:
      args = op->arg2;
      if (state.depth < args) goto finished;
      state.depth -= args;
      for (i = 0; i < args; i++) operand[i] = state.stack[state.depth + i];
      /* f_add preserves UNDEFINED only if all arguments are UNDEFINED.
       * Any computed value or literal ensures an ordinary result. Otherwise
       * require one original input to have the ordinary integer subtype. */
      for (i = 0; i < args; i++)
        if (p->value[operand[i]].kind != PIKE_SSA_INTEGER_LOAD) break;
      if (i == args) p->normal |= 1U << p->value[operand[0]].bank;
      left = operand[0];
      kind = PIKE_SSA_INTEGER_SUM;
      push = 1;
      break;
    case F_LOCAL: case F_2_LOCALS:
      if (state.depth + (src >= 0 ? 2 : 1) > stack_limit)
        goto finished;
      state.stack[state.depth++] = state.local[dst];
      if (src >= 0) state.stack[state.depth++] = state.local[src];
      goto accepted;
    case F_ASSIGN_LOCAL: case F_ASSIGN_LOCAL_AND_POP:
      if (!state.depth) goto finished;
      left = state.stack[state.depth - 1];
      /* Bare input copies would need parallel subtype-tag moves at commit.
       * Keep those on the ordinary path; arithmetic results have tag INT/0. */
      if (p->value[left].kind == PIKE_SSA_INTEGER_LOAD ||
          p->value[left].kind == PIKE_SSA_INTEGER_CONSTANT) goto finished;
      state.local[dst] = left;
      state.dirty |= 1U << dst;
      p->writes++;
      if (op->opcode == F_ASSIGN_LOCAL_AND_POP) state.depth--;
      else p->retained = 1;
      goto accepted;
    case F_POP_VALUE:
      if (!state.depth) goto finished;
      state.depth--;
      goto accepted;
    case F_AND: case F_OR: case F_XOR:
      kind = op->opcode == F_AND ? PIKE_SSA_INTEGER_AND :
        op->opcode == F_OR ? PIKE_SSA_INTEGER_OR : PIKE_SSA_INTEGER_XOR;
      goto binary;
    case F_SUBTRACT:
      kind = PIKE_SSA_INTEGER_SUBTRACT;
      /* FALLTHRU */
    case F_ADD_INTS:
    binary:
      if (state.depth < 2) goto finished;
      right = state.stack[--state.depth];
      left = state.stack[--state.depth];
      push = 1;
      break;
    case F_AND_INT: case F_OR_INT: case F_XOR_INT:
      kind = op->opcode == F_AND_INT ? PIKE_SSA_INTEGER_AND :
        op->opcode == F_OR_INT ? PIKE_SSA_INTEGER_OR : PIKE_SSA_INTEGER_XOR;
      /* FALLTHRU */
    case F_ADD_INT: case F_ADD_NEG_INT: case F_SUBTRACT_INT:
      if (!state.depth) goto finished;
      left = state.stack[--state.depth];
      if (integer_stack_bitwise(kind)) imm = op->arg;
      else if (op->opcode == F_SUBTRACT_INT && subtract) {
        kind = PIKE_SSA_INTEGER_SUBTRACT;
        imm = op->arg;
      } else
        imm = op->opcode == F_ADD_INT ? op->arg : -op->arg;
      push = 1;
      break;
    default:
      left = state.local[dst];
      write = 1;
      if (op->opcode == F_ADD_LOCALS_AND_POP) right = state.local[src];
      else if (op->opcode == F_ADD_LOCAL_INT_AND_POP) imm = op->arg2;
      else imm = (op->opcode == F_INC_LOCAL ||
                  op->opcode == F_INC_LOCAL_AND_POP) ? 1 : -1;
      push = op->opcode == F_INC_LOCAL || op->opcode == F_DEC_LOCAL;
      if (push && state.depth == stack_limit) goto finished;
      state.local[dst] = 0;
      break;
    }
    for (i = 0; i < p->locals; i++)
      if (state.local[i]) live |= 1U << p->value[state.local[i]].bank;
    for (i = 0; i < state.depth; i++)
      if (p->value[state.stack[i]].kind != PIKE_SSA_INTEGER_CONSTANT)
        live |= 1U << p->value[state.stack[i]].bank;
    bank = p->value[left].bank;
    if (live & (1U << bank)) {
      for (bank = 0; bank < PIKE_SSA_INTEGER_LOCALS; bank++)
        if (!(live & (1U << bank))) break;
      if (bank == PIKE_SSA_INTEGER_LOCALS) goto finished;
    }
    {
      struct pikecode_ssa_integer_step *step = &p->step[p->count++];
      struct pikecode_ssa_integer_value *v = &p->value[++p->values];
      step->first = e;
      step->before = before;
      step->result = p->values;
      v->kind = kind;
      v->left = left;
      v->right = right;
      v->imm = imm;
      v->bank = bank;
      v->local = -1;
      v->args = args;
      for (i = 0; i < args; i++) v->operand[i] = operand[i];
      if (write) {
        state.local[dst] = p->values;
        state.dirty |= 1U << dst;
        p->writes++;
      }
      if (push) state.stack[state.depth++] = p->values;
      p->loads += args ? args : right ? 2 : 1;
      p->bitwise += integer_stack_bitwise(kind);
      p->expressions += kind == PIKE_SSA_INTEGER_NEGATE ||
        kind == PIKE_SSA_INTEGER_SUM;
    }
  accepted:
    if (dst >= 0) used |= 1U << dst;
    if (src >= 0) used |= 1U << src;
    p->final = state;
    p->consumed = e + 1;
    /* Keep the longest profitable prefix, including a live-stack exit.
     * Its final snapshot is published just like an overflow snapshot. */
    if (((!state.depth && (p->retained || p->bitwise || p->expressions)) ||
         (state.depth && stack_exits)) &&
        p->count >= 2 &&
        p->loads > p->locals) {
      best = *p;
      best_used = used;
    }
  }
 finished:
  if (!best.consumed || best_used != (1U << best.locals) - 1) return 0;
  *p = best;
  return p->consumed;
}
