/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Included at the end of amd64.c. Integer values in a short arithmetic run
 * have SSA versions. Coalesce successive versions of each local into one
 * register, computing a new result in RAX so overflow leaves the input
 * version intact. Every side exit commits the completed prefix before
 * executing the remaining operations through their ordinary C opcodes.
 */
#define AMD64_INTEGER_LOCALS 3
#define AMD64_INTEGER_VALUES (AMD64_INTEGER_LOCALS + CODE_GENERATOR_REGION_LIMIT)

enum amd64_integer_kind { AMD64_INTEGER_LOAD, AMD64_INTEGER_ADD };
struct amd64_integer_value {
  enum amd64_integer_kind kind;
  unsigned int left, right;
  INT32 local, imm;
  enum amd64_reg reg;
};
struct amd64_integer_unit {
  unsigned int first, length, count, result[2], dirty_before;
};
struct amd64_integer_plan {
  unsigned int units, count, consumed, locals, values, dirty;
  INT32 local[AMD64_INTEGER_LOCALS];
  unsigned int current[AMD64_INTEGER_LOCALS];
  struct amd64_integer_unit unit[CODE_GENERATOR_REGION_LIMIT];
  struct amd64_integer_value value[AMD64_INTEGER_VALUES + 1];
};

int amd64_integer_regions_enabled(void)
{
#if SIZEOF_INT_TYPE == 8
  return ssa_integer_regions;
#else
  return 0;
#endif
}

/* Each unit starts and ends at the same Pike stack depth. Its fast path
 * writes one local. Multi-step units can replay from their beginning after
 * restoring that local's input register: no partial result was published. */
static int amd64_integer_unit(const struct p_instr_s *c, ptrdiff_t available,
                              INT32 *dst, INT32 *src, INT32 imm[2],
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

static int amd64_integer_local(struct amd64_integer_plan *p, INT32 local)
{
  static const enum amd64_reg regs[] = { P_REG_R8, P_REG_R9, P_REG_R10 };
  struct amd64_integer_value *v;
  unsigned int i;
  for (i = 0; i < p->locals; i++)
    if (p->local[i] == local) return i;
  if (p->locals == AMD64_INTEGER_LOCALS || local < 0 ||
      local > (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
                        sizeof(struct svalue)))
    return -1;
  p->local[i] = local;
  p->current[i] = ++p->values;
  v = &p->value[p->values];
  v->kind = AMD64_INTEGER_LOAD;
  v->local = local;
  v->reg = regs[i];
  p->locals++;
  return i;
}

/* Unlike add_reg_imm(), emit an ADD even for zero, and never negate an
 * INT32_MIN immediate. The following overflow branch needs fresh flags. */
static void amd64_integer_add_imm(enum amd64_reg reg, INT32 imm)
{
  int small = imm >= -128 && imm <= 127;
  rex(1, 0, 0, reg);
  opcode(small ? 0x83 : 0x81);
  modrm(3, 0, reg);
  if (small) ib(imm);
  else id(imm);
}

static void amd64_integer_commit(struct amd64_integer_plan *p,
                                 unsigned int dirty)
{
  unsigned int i;
  for (i = 0; i < p->locals; i++) {
    struct amd64_integer_value *v = &p->value[p->current[i]];
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    if (!(dirty & (1U << i))) continue;
    mov_reg_mem(v->reg, PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, PIKE_LOCALS_REG,
                  off + OFFSETOF(svalue, tu.t.type));
  }
}

/* The caller publishes no source records inside this same-line run. */
int amd64_integer_run(const struct p_instr_s *code, ptrdiff_t available)
{
  struct amd64_integer_plan p;
  struct label overflow[CODE_GENERATOR_REGION_LIMIT];
  struct label slow[CODE_GENERATOR_REGION_LIMIT], done = { .addr = -1 };
  unsigned int i, j;
  unsigned int loads = 0, stores = 0;
  if (!amd64_integer_regions_enabled()) return 0;
  switch (code[0].opcode) {
  case F_INC_LOCAL_AND_POP: case F_DEC_LOCAL_AND_POP:
  case F_ADD_LOCAL_INT_AND_POP: case F_ADD_LOCALS_AND_POP:
  case F_LOCAL: case F_2_LOCALS: case F_INC_LOCAL: case F_DEC_LOCAL:
    break;
  default:
    return 0;
  }
  memset(&p, 0, sizeof(p));
  while (p.count < CODE_GENERATOR_REGION_LIMIT && p.consumed < available) {
    const struct p_instr_s *op = &code[p.consumed];
    struct amd64_integer_unit *u = &p.unit[p.units];
    unsigned int steps;
    INT32 local, source, imm[2];
    int dst, src = -1;
    int length = amd64_integer_unit(op, available - p.consumed,
                                   &local, &source, imm, &steps);
    if (!length || p.count + steps > CODE_GENERATOR_REGION_LIMIT) break;
    for (i = 0; i < (unsigned int)length; i++)
      if (op[i].line != code[0].line || op[i].file != code[0].file) break;
    if (i != (unsigned int)length) break;
    dst = amd64_integer_local(&p, local);
    if (dst < 0) break;
    if (source >= 0) {
      src = amd64_integer_local(&p, source);
      if (src < 0) break;
    }
    u->first = p.consumed;
    u->length = length;
    u->count = steps;
    u->dirty_before = p.dirty;
    for (i = 0; i < steps; i++) {
      struct amd64_integer_value *v = &p.value[++p.values];
      v->kind = AMD64_INTEGER_ADD;
      v->left = p.current[dst];
      v->right = src < 0 ? 0 : p.current[src];
      v->local = local;
      v->imm = src < 0 ? imm[i] : 0;
      v->reg = p.value[v->left].reg;
      p.current[dst] = p.values;
      u->result[i] = p.values;
      loads += src < 0 ? 1 : 2;
    }
    p.dirty |= 1U << dst;
    p.count += steps;
    p.consumed += length;
    p.units++;
  }
  /* Do not speculate on untouched locals from an operation that exceeded
   * the register budget. A single value operation has nothing to reuse. */
  if (p.count < 2) return 0;
  for (i = 0; i < p.locals; i++) {
    int used = 0;
    for (j = 0; j < p.units; j++) {
      struct amd64_integer_value *v = &p.value[p.unit[j].result[0]];
      if (p.value[v->left].local == p.local[i] ||
          (v->right && p.value[v->right].local == p.local[i])) used = 1;
    }
    if (!used) return 0;
  }
  if (loads <= p.locals) return 0;
  for (i = 0; i < p.units; i++) {
    overflow[i] = (struct label){ .addr = -1 };
    slow[i] = (struct label){ .addr = -1 };
  }
  amd64_ssa_region_begin();
  ssa_locals_valid = 0;
  pikecode_ssa_clobber(&ssa_state, PIKE_SSA_LOCALS);
  maybe_update_pc();
  amd64_load_fp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), PIKE_LOCALS_REG);
  for (i = 1; i <= p.values; i++) {
    struct amd64_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (v->kind != AMD64_INTEGER_LOAD) continue;
    mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type), P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(&slow[0]);
    mov_mem_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer), v->reg);
  }
  for (i = 0; i < p.units; i++) {
    struct amd64_integer_unit *u = &p.unit[i];
    if (u->count > 1)
      mov_reg_reg(p.value[u->result[0]].reg, P_REG_RDX);
    for (j = 0; j < u->count; j++) {
      struct amd64_integer_value *v = &p.value[u->result[j]];
      mov_reg_reg(p.value[v->left].reg, P_REG_RAX);
      if (v->right) add_reg_reg(P_REG_RAX, p.value[v->right].reg);
      else amd64_integer_add_imm(P_REG_RAX, v->imm);
      jo(&overflow[i]);
      mov_reg_reg(P_REG_RAX, v->reg);
    }
  }
  amd64_integer_commit(&p, p.dirty);
  jmp(&done);
  for (i = 0; i < p.units; i++) {
    struct amd64_integer_unit *u = &p.unit[i];
    label(&overflow[i]);
    if (u->count > 1)
      mov_reg_reg(P_REG_RDX, p.value[u->result[0]].reg);
    amd64_integer_commit(&p, u->dirty_before);
    jmp(&slow[i]);
  }
  for (i = 0; i < p.units; i++) {
    struct amd64_integer_unit *u = &p.unit[i];
    label(&slow[i]);
    for (j = 0; j < u->length; j++) {
      const struct p_instr_s *op = &code[u->first + j];
      unsigned int instr = op->opcode - F_OFFSET;
      if (instrs[instr].flags & I_HASARG) update_arg1(op->arg);
      if (instrs[instr].flags & I_HASARG2) update_arg2(op->arg2);
      amd64_call_c_opcode(instrs[instr].address, instrs[instr].flags);
    }
  }
  label(&done);
  amd64_ssa_region_end();
  if (ssa_report_stats) {
    for (i = 0; i < p.locals; i++) stores += !!(p.dirty & (1U << i));
    ssa_stats.integer_regions++;
    ssa_stats.integer_ops += p.count;
    ssa_stats.integer_values += p.values;
    ssa_stats.integer_loads_avoided += loads - p.locals;
    ssa_stats.integer_stores_avoided += p.count - stores;
  }
  return p.consumed;
}
