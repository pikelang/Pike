/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* AMD64 lowering for the shared bounded integer value plan. Registers
 * remain unchanged so the extraction preserves the emitted instruction
 * sequence, guards, rollback and fallback paths. */
int amd64_integer_regions_enabled(void)
{
#if SIZEOF_INT_TYPE == 8
  return ssa_integer_regions;
#else
  return 0;
#endif
}

static enum amd64_reg
amd64_integer_reg(const struct pikecode_ssa_integer_value *value)
{
  static const enum amd64_reg regs[] = { P_REG_R8, P_REG_R9, P_REG_R10 };
  return regs[value->bank];
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

static void amd64_integer_commit(struct pikecode_ssa_integer_plan *p,
                                unsigned int dirty)
{
  unsigned int i;
  for (i = 0; i < p->locals; i++) {
    struct pikecode_ssa_integer_value *v = &p->value[p->current[i]];
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    if (!(dirty & (1U << i))) continue;
    mov_reg_mem(amd64_integer_reg(v), PIKE_LOCALS_REG,
                off + OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, PIKE_LOCALS_REG,
                  off + OFFSETOF(svalue, tu.t.type));
  }
}

/* The caller publishes no source records inside this same-line run. */
int amd64_integer_run(const struct p_instr_s *code, ptrdiff_t available)
{
  struct pikecode_ssa_integer_plan p;
  struct label overflow[PIKE_SSA_INTEGER_LIMIT];
  struct label slow[PIKE_SSA_INTEGER_LIMIT], done = { .addr = -1 };
  unsigned int i, j;
  unsigned int stores = 0;
  if (!amd64_integer_regions_enabled()) return 0;
  if (!pikecode_ssa_plan_integer(&p, code, available,
      (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
              sizeof(struct svalue)))) return 0;
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
    struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (v->kind != PIKE_SSA_INTEGER_LOAD) continue;
    mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type), P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(&slow[0]);
    mov_mem_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer),
                amd64_integer_reg(v));
  }
  for (i = 0; i < p.units; i++) {
    struct pikecode_ssa_integer_unit *u = &p.unit[i];
    if (u->count > 1)
      mov_reg_reg(amd64_integer_reg(&p.value[u->result[0]]), P_REG_RDX);
    for (j = 0; j < u->count; j++) {
      struct pikecode_ssa_integer_value *v = &p.value[u->result[j]];
      mov_reg_reg(amd64_integer_reg(&p.value[v->left]), P_REG_RAX);
      if (v->right)
        add_reg_reg(P_REG_RAX, amd64_integer_reg(&p.value[v->right]));
      else amd64_integer_add_imm(P_REG_RAX, v->imm);
      jo(&overflow[i]);
      mov_reg_reg(P_REG_RAX, amd64_integer_reg(v));
    }
  }
  amd64_integer_commit(&p, p.dirty);
  jmp(&done);
  for (i = 0; i < p.units; i++) {
    struct pikecode_ssa_integer_unit *u = &p.unit[i];
    label(&overflow[i]);
    if (u->count > 1)
      mov_reg_reg(P_REG_RDX, amd64_integer_reg(&p.value[u->result[0]]));
    amd64_integer_commit(&p, u->dirty_before);
    jmp(&slow[i]);
  }
  for (i = 0; i < p.units; i++) {
    struct pikecode_ssa_integer_unit *u = &p.unit[i];
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
    ssa_stats.integer_loads_avoided += p.loads - p.locals;
    ssa_stats.integer_stores_avoided += p.count - stores;
  }
  return p.consumed;
}
