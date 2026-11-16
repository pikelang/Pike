/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* AMD64 lowering for the shared bounded integer value plans. Three banks
 * retain live versions; RAX holds speculative results and RDX supplies
 * logical immediates, balanced rollback or stack publication as needed. */
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

/* Emit ADD/SUB even for zero, without negating an INT32_MIN immediate.
 * Unlike the generic helpers, these always leave fresh overflow flags. */
static void amd64_integer_alu_imm(enum amd64_reg reg, INT32 imm, int sub)
{
  int small = imm >= -128 && imm <= 127;
  rex(1, 0, 0, reg);
  opcode(small ? 0x83 : 0x81);
  modrm(3, sub ? 5 : 0, reg);
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

/* Publish a side exit before any C call. Input tags are copied before local
 * commits, since a dirty local may still hold an input's original subtype. */
static void amd64_integer_snapshot(
    const struct pikecode_ssa_integer_stack_plan *p,
    const struct pikecode_ssa_integer_snapshot *s)
{
  unsigned int i;
  if (s->depth) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer), P_REG_RDX);
    for (i = 0; i < s->depth; i++) {
      const struct pikecode_ssa_integer_value *v = &p->value[s->stack[i]];
      ptrdiff_t off = i * sizeof(struct svalue);
      mov_reg_mem(amd64_integer_reg(v), P_REG_RDX,
                  off + OFFSETOF(svalue, u.integer));
      if (v->kind == PIKE_SSA_INTEGER_LOAD) {
        mov_mem_reg(PIKE_LOCALS_REG,
                    v->local * sizeof(struct svalue) +
                    OFFSETOF(svalue, tu.t.type), P_REG_RAX);
        mov_reg_mem(P_REG_RAX, P_REG_RDX, off + OFFSETOF(svalue, tu.t.type));
      } else
        mov_imm_mem(PIKE_T_INT, P_REG_RDX, off + OFFSETOF(svalue, tu.t.type));
    }
    add_reg_imm(P_REG_RDX, s->depth * sizeof(struct svalue));
    mov_reg_mem(P_REG_RDX, Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer));
  }
  for (i = 0; i < p->locals; i++) {
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    if (!(s->dirty & (1U << i))) continue;
    mov_reg_mem(amd64_integer_reg(&p->value[s->local[i]]), PIKE_LOCALS_REG,
                off + OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, PIKE_LOCALS_REG,
                  off + OFFSETOF(svalue, tu.t.type));
  }
}

static int amd64_integer_stack_run(const struct p_instr_s *code,
                                   ptrdiff_t available, int store_lines)
{
  struct pikecode_ssa_integer_stack_plan p;
  struct label overflow[PIKE_SSA_INTEGER_LIMIT];
  struct label slow[PIKE_SSA_INTEGER_CODE], done = { .addr = -1 };
  unsigned int i, stores = 0;
  if (ssa_no_integer_stack ||
      !pikecode_ssa_plan_integer_stack(&p, code, available,
        (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
                sizeof(struct svalue)),
        !ssa_no_integer_stack_exits, !ssa_no_integer_subtract,
        !ssa_no_integer_bitwise)) return 0;
  for (i = 0; i < p.count; i++) overflow[i] = (struct label){ .addr = -1 };
  for (i = 0; i < p.consumed; i++) slow[i] = (struct label){ .addr = -1 };
  amd64_flush_code_generator_state();
  amd64_ssa_region_begin();
  /* Keep wide branches, but do not relocate published source records. */
  if (pikecode_ssa_integer_multiline(code, p.consumed))
    ssa_region.abandoned = 1;
  maybe_update_pc();
  amd64_load_fp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), PIKE_LOCALS_REG);
  for (i = 1; i <= p.locals; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type), P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(&slow[0]);
    mov_mem_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer),
                amd64_integer_reg(v));
  }
  for (i = 0; i < p.count; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.step[i].result];
    pikecode_ssa_integer_source(&code[p.step[i].first], store_lines);
    mov_reg_reg(amd64_integer_reg(&p.value[v->left]), P_REG_RAX);
    if (integer_stack_bitwise(v->kind)) {
      enum amd64_reg right;
      if (v->right) right = amd64_integer_reg(&p.value[v->right]);
      else {
        mov_imm_reg(v->imm, P_REG_RDX);
        right = P_REG_RDX;
      }
      /* All three encodings write RAX; the ordinary emitter's XOR/OR
       * helpers use the opposite operand order to its AND helper. */
      rex(1, P_REG_RAX, 0, right);
      opcode(v->kind == PIKE_SSA_INTEGER_AND ? 0x23 :
             v->kind == PIKE_SSA_INTEGER_OR ? 0x0b : 0x33);
      modrm(3, P_REG_RAX, right);
    } else {
      if (v->right) {
        if (v->kind == PIKE_SSA_INTEGER_SUBTRACT)
          sub_reg_reg(P_REG_RAX, amd64_integer_reg(&p.value[v->right]));
        else add_reg_reg(P_REG_RAX, amd64_integer_reg(&p.value[v->right]));
      } else
        amd64_integer_alu_imm(P_REG_RAX, v->imm,
                              v->kind == PIKE_SSA_INTEGER_SUBTRACT);
      jo(&overflow[i]);
    }
    mov_reg_reg(P_REG_RAX, amd64_integer_reg(v));
  }
  /* Publish live results before the next ordinary opcode. The slow replay
   * finishes at the same stack depth with empty machine caches. */
  amd64_integer_snapshot(&p, &p.final);
  jmp(&done);
  for (i = 0; i < p.count; i++) {
    if (integer_stack_bitwise(p.value[p.step[i].result].kind)) continue;
    label(&overflow[i]);
    amd64_integer_snapshot(&p, &p.step[i].before);
    jmp(&slow[p.step[i].first]);
  }
  for (i = 0; i < p.consumed; i++) {
    unsigned int instr = code[i].opcode - F_OFFSET;
    /* Incoming exits have different stack depths. Flush fallthrough before
     * binding, so all entries reload from canonical interpreter state. */
    amd64_flush_code_generator_state();
    label(&slow[i]);
    pikecode_ssa_integer_source(&code[i], store_lines);
    if (instrs[instr].flags & I_HASARG) update_arg1(code[i].arg);
    if (instrs[instr].flags & I_HASARG2) update_arg2(code[i].arg2);
    amd64_call_c_opcode(instrs[instr].address, instrs[instr].flags);
  }
  amd64_flush_code_generator_state();
  label(&done);
  amd64_ssa_region_end();
  pikecode_ssa_integer_source(&code[p.consumed - 1], store_lines);
  /* Slow calls may publish a PC which the fast branch never executed. */
  amd64_prev_stored_pc = -1;
  if (ssa_report_stats) {
    for (i = 0; i < p.locals; i++) stores += !!(p.final.dirty & (1U << i));
    ssa_stats.integer_regions++;
    ssa_stats.integer_stack_regions++;
    if (p.final.depth) ssa_stats.integer_stack_exits++;
    ssa_stats.integer_ops += p.count;
    ssa_stats.integer_values += p.values;
    ssa_stats.integer_bitwise_ops += p.bitwise;
    for (i = 0; i < p.count; i++)
      if (p.value[p.step[i].result].kind == PIKE_SSA_INTEGER_SUBTRACT)
        ssa_stats.integer_subtract_ops++;
    ssa_stats.integer_loads_avoided += p.loads - p.locals;
    ssa_stats.integer_stores_avoided += p.writes - stores;
  }
  return p.consumed;
}

/* Source records inside multiline runs require fixed native offsets. */
int amd64_integer_run(const struct p_instr_s *code, ptrdiff_t available,
                       int store_lines)
{
  struct pikecode_ssa_integer_plan p;
  struct label overflow[PIKE_SSA_INTEGER_LIMIT];
  struct label slow[PIKE_SSA_INTEGER_LIMIT], done = { .addr = -1 };
  unsigned int i, j;
  unsigned int stores = 0;
  if (!amd64_integer_regions_enabled()) return 0;
  {
    int consumed = amd64_integer_stack_run(code, available, store_lines);
    if (consumed) return consumed;
  }
  if (!pikecode_ssa_plan_integer(&p, code, available,
      (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
              sizeof(struct svalue)))) return 0;
  for (i = 0; i < p.units; i++) {
    overflow[i] = (struct label){ .addr = -1 };
    slow[i] = (struct label){ .addr = -1 };
  }
  amd64_ssa_region_begin();
  /* Keep wide branches, but do not relocate published source records. */
  if (pikecode_ssa_integer_multiline(code, p.consumed))
    ssa_region.abandoned = 1;
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
      /* A fused unit's final add follows its local load/increment. */
      unsigned int source = u->first;
      if (u->length > 1 && (u->count == 1 || j)) source++;
      pikecode_ssa_integer_source(&code[source], store_lines);
      mov_reg_reg(amd64_integer_reg(&p.value[v->left]), P_REG_RAX);
      if (v->right)
        add_reg_reg(P_REG_RAX, amd64_integer_reg(&p.value[v->right]));
      else amd64_integer_alu_imm(P_REG_RAX, v->imm, 0);
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
    /* A side exit can skip the preceding slow calls and their PC stores. */
    amd64_flush_code_generator_state();
    label(&slow[i]);
    for (j = 0; j < u->length; j++) {
      const struct p_instr_s *op = &code[u->first + j];
      unsigned int instr = op->opcode - F_OFFSET;
      pikecode_ssa_integer_source(op, store_lines);
      if (instrs[instr].flags & I_HASARG) update_arg1(op->arg);
      if (instrs[instr].flags & I_HASARG2) update_arg2(op->arg2);
      amd64_call_c_opcode(instrs[instr].address, instrs[instr].flags);
    }
  }
  label(&done);
  amd64_ssa_region_end();
  pikecode_ssa_integer_source(&code[p.consumed - 1], store_lines);
  /* Slow calls may publish a PC which the fast branch never executed. */
  amd64_prev_stored_pc = -1;
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
