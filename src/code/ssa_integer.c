/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Included by pikecode.c. This planner contains no native registers,
 * instruction encodings or branch fixups. Value banks express coalescing:
 * every version of one local occupies the same backend-selected register.
 */
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
  while (p->count < PIKE_SSA_INTEGER_LIMIT && p->consumed < available) {
    const struct p_instr_s *op = &code[p->consumed];
    struct pikecode_ssa_integer_unit *u = &p->unit[p->units];
    unsigned int steps;
    INT32 local, source, imm[2];
    int dst, src = -1;
    int length = pikecode_ssa_integer_unit(op, available - p->consumed,
                                           &local, &source, imm, &steps);
    if (!length || p->count + steps > PIKE_SSA_INTEGER_LIMIT) break;
    for (i = 0; i < (unsigned int)length; i++)
      if (op[i].line != code[0].line || op[i].file != code[0].file) break;
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
