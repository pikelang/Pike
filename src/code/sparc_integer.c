/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Included at the end of sparc.c. No C call is made with uncommitted
 * integer values. L5 holds locals, O0..O2 hold value banks, O3 a speculative
 * result, O4 a multi-step unit's input and O5 an immediate operand.
 *
 * Pointer width follows the ABI; integer loads, stores and condition
 * codes follow Pike's native integer width. Word arithmetic needs only V7
 * instructions. Doubleword values currently require the SPARC64 backend. */
#if SIZEOF_INT_TYPE == 4
#define SPARC_INTEGER_LD SPARC_LDUW
#define SPARC_INTEGER_ST SPARC_STW
#define SPARC_INTEGER_BVS 0x0e800000 /* bvs %icc */
#elif defined(PIKE_BYTECODE_SPARC64) && SIZEOF_INT_TYPE == 8
#define SPARC_INTEGER_LD SPARC_LDX
#define SPARC_INTEGER_ST SPARC_STX
#define SPARC_INTEGER_BVS 0x0e600000 /* bvs,pn %xcc */
#endif

int sparc_integer_regions_enabled(void)
{
#ifdef SPARC_INTEGER_LD
  return sparc_ssa_integer_regions;
#else
  return 0;
#endif
}

#ifdef SPARC_INTEGER_LD
struct sparc_integer_label {
  unsigned int count;
  INT32 use[PIKE_SSA_INTEGER_LOCALS + 2];
};

static int sparc_integer_reg(const struct pikecode_ssa_integer_value *value)
{
  return SPARC_REG_O0 + value->bank;
}

static void sparc_integer_branch(struct sparc_integer_label *target,
                                 unsigned INT32 instruction)
{
  if (target->count == NELEM(target->use))
    Pike_fatal("Too many SPARC integer branch references.\n");
  target->use[target->count++] = Pike_compiler->new_program->num_program;
  add_to_program(instruction);
  /* The speculative result must not overwrite its input in a delay slot. */
  SPARC_NOOP();
}

static void sparc_integer_bind(struct sparc_integer_label *target)
{
  struct program *p = Pike_compiler->new_program;
  unsigned int i;
  for (i = 0; i < target->count; i++) {
    INT32 off = target->use[i], delta = p->num_program - off;
    unsigned INT32 word = p->program[off];
    /* V9 BPcc has a signed 19-bit word displacement; Bicc has 22 bits. */
    unsigned INT32 mask = ((word >> 22) & 7) == 1 ? 0x7ffff : 0x3fffff;
    if (delta < -(INT32)(mask / 2 + 1) || delta > (INT32)(mask / 2))
      Pike_fatal("SPARC integer branch is out of range.\n");
    p->program[off] = (word & ~mask) | ((unsigned INT32)delta & mask);
  }
}

static void sparc_integer_commit(const struct pikecode_ssa_integer_plan *p,
                                 unsigned int dirty)
{
  unsigned int i;
  for (i = 0; i < p->locals; i++) {
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    if (!(dirty & (1U << i))) continue;
    SPARC_INTEGER_ST(SPARC_REG_O0 + i, SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
    /* Both type and subtype become an ordinary integer. On big-endian
     * SPARC the contiguous tag word begins at tu.t.type. */
    SPARC_STW(SPARC_REG_G0, SPARC_REG_L5,
              off + OFFSETOF(svalue, tu.t.type), 1);
  }
}
#endif

#ifdef SPARC_INTEGER_LD
static void sparc_integer_snapshot(
    const struct pikecode_ssa_integer_stack_plan *p,
    const struct pikecode_ssa_integer_snapshot *s)
{
  unsigned int i;
  /* Copy input tags before committing locals that might own those tags. */
  if (s->depth) {
    PIKE_LDPTR(SPARC_REG_O4, SPARC_REG_PIKE_IP,
               OFFSETOF(Pike_interpreter_struct, stack_pointer), 1);
    for (i = 0; i < s->depth; i++) {
      const struct pikecode_ssa_integer_value *v = &p->value[s->stack[i]];
      ptrdiff_t off = i * sizeof(struct svalue);
      SPARC_INTEGER_ST(sparc_integer_reg(v), SPARC_REG_O4,
                       off + OFFSETOF(svalue, u.integer), 1);
      if (v->kind == PIKE_SSA_INTEGER_LOAD) {
        SPARC_LDUW(SPARC_REG_O3, SPARC_REG_L5,
                    v->local * sizeof(struct svalue) +
                    OFFSETOF(svalue, tu.t.type), 1);
        SPARC_STW(SPARC_REG_O3, SPARC_REG_O4,
                   off + OFFSETOF(svalue, tu.t.type), 1);
      } else
        SPARC_STW(SPARC_REG_G0, SPARC_REG_O4,
                   off + OFFSETOF(svalue, tu.t.type), 1);
    }
    SPARC_ADD(SPARC_REG_O4, SPARC_REG_O4,
               s->depth * sizeof(struct svalue), 1);
    PIKE_STPTR(SPARC_REG_O4, SPARC_REG_PIKE_IP,
                OFFSETOF(Pike_interpreter_struct, stack_pointer), 1);
  }
  for (i = 0; i < p->locals; i++) {
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    if (!(s->dirty & (1U << i))) continue;
    SPARC_INTEGER_ST(sparc_integer_reg(&p->value[s->local[i]]), SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
    SPARC_STW(SPARC_REG_G0, SPARC_REG_L5,
               off + OFFSETOF(svalue, tu.t.type), 1);
  }
}

static int sparc_integer_stack_run(const struct p_instr_s *code,
                                   ptrdiff_t available)
{
  struct pikecode_ssa_integer_stack_plan p;
  struct sparc_integer_label overflow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label slow[PIKE_SSA_INTEGER_CODE] = {{0}};
  struct sparc_integer_label done = {0};
  unsigned int i, stores = 0;
  if (sparc_ssa_no_integer_stack ||
      !pikecode_ssa_plan_integer_stack(&p, code, available,
        (4095 - OFFSETOF(svalue, u.integer)) / sizeof(struct svalue)))
    return 0;
  sparc_flush_codegen_state();
  sparc_update_pc();
  LOAD_PIKE_FP();
  PIKE_LDPTR(SPARC_REG_L5, SPARC_REG_PIKE_FP,
             OFFSETOF(pike_frame, locals), 1);
  for (i = 1; i <= p.locals; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    SPARC_LDUH(SPARC_REG_O3, SPARC_REG_L5,
               off + OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    sparc_integer_branch(&slow[0], 0x12800000); /* bne %icc */
    SPARC_INTEGER_LD(sparc_integer_reg(v), SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
  }
  for (i = 0; i < p.count; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.step[i].result];
    int right;
    if (v->right) right = sparc_integer_reg(&p.value[v->right]);
    else {
      SET_REG(SPARC_REG_O5, v->imm);
      right = SPARC_REG_O5;
    }
    SPARC_ALU_OP(0x10, SPARC_REG_O3,
                 sparc_integer_reg(&p.value[v->left]), right, 0); /* addcc */
    sparc_integer_branch(&overflow[i], SPARC_INTEGER_BVS);
    SPARC_OR(sparc_integer_reg(v), SPARC_REG_O3, SPARC_REG_G0, 0);
  }
  sparc_integer_snapshot(&p, &p.final);
  sparc_integer_branch(&done, 0x10800000);
  for (i = 0; i < p.count; i++) {
    sparc_integer_bind(&overflow[i]);
    sparc_integer_snapshot(&p, &p.step[i].before);
    sparc_integer_branch(&slow[p.step[i].first], 0x10800000);
  }
  for (i = 0; i < p.consumed; i++) {
    const struct p_instr_s *op = &code[i];
    unsigned int flags = instrs[op->opcode - F_OFFSET].flags;
    /* Fallthrough publishes its deferred stores before the label. Side
     * exits skip them and arrive with their own canonical stack snapshot. */
    sparc_flush_codegen_state();
    sparc_integer_bind(&slow[i]);
    if (flags & I_HASARG2)
      ins_f_byte_with_2_args(op->opcode, op->arg, op->arg2);
    else if (flags & I_HASARG) ins_f_byte_with_arg(op->opcode, op->arg);
    else ins_f_byte(op->opcode);
  }
  sparc_flush_codegen_state();
  sparc_integer_bind(&done);
  if (sparc_ssa_stats) {
    for (i = 0; i < p.locals; i++) stores += !!(p.final.dirty & (1U << i));
    sparc_stats.integer_regions++;
    sparc_stats.integer_stack_regions++;
    sparc_stats.integer_ops += p.count;
    sparc_stats.integer_values += p.values;
    sparc_stats.integer_loads_avoided += p.loads - p.locals;
    sparc_stats.integer_stores_avoided += p.writes - stores;
  }
  return p.consumed;
}
#endif

int sparc_integer_run(const struct p_instr_s *code, ptrdiff_t available)
{
#ifdef SPARC_INTEGER_LD
  struct pikecode_ssa_integer_plan p;
  struct sparc_integer_label overflow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label slow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label done = {0};
  unsigned int i, j, stores = 0;
  if (!sparc_integer_regions_enabled()) return 0;
  {
    int consumed = sparc_integer_stack_run(code, available);
    if (consumed) return consumed;
  }
  if (!pikecode_ssa_plan_integer(&p, code, available,
        (4095 - OFFSETOF(svalue, u.integer)) / sizeof(struct svalue)))
    return 0;

  sparc_flush_codegen_state();
  sparc_update_pc();
  LOAD_PIKE_FP();
  PIKE_LDPTR(SPARC_REG_L5, SPARC_REG_PIKE_FP,
            OFFSETOF(pike_frame, locals), 1);
  for (i = 1; i <= p.values; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (v->kind != PIKE_SSA_INTEGER_LOAD) continue;
    SPARC_LDUH(SPARC_REG_O3, SPARC_REG_L5,
               off + OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    sparc_integer_branch(&slow[0], 0x12800000); /* bne %icc */
    SPARC_INTEGER_LD(sparc_integer_reg(v), SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
  }
  for (i = 0; i < p.units; i++) {
    const struct pikecode_ssa_integer_unit *u = &p.unit[i];
    if (u->count > 1)
      SPARC_OR(SPARC_REG_O4, sparc_integer_reg(&p.value[u->result[0]]),
               SPARC_REG_G0, 0);
    for (j = 0; j < u->count; j++) {
      const struct pikecode_ssa_integer_value *v = &p.value[u->result[j]];
      int right;
      if (v->right) right = sparc_integer_reg(&p.value[v->right]);
      else {
        SET_REG(SPARC_REG_O5, v->imm);
        right = SPARC_REG_O5;
      }
      SPARC_ALU_OP(0x10, SPARC_REG_O3,
                   sparc_integer_reg(&p.value[v->left]), right, 0); /* addcc */
      sparc_integer_branch(&overflow[i], SPARC_INTEGER_BVS);
      SPARC_OR(sparc_integer_reg(v), SPARC_REG_O3, SPARC_REG_G0, 0);
    }
  }
  sparc_integer_commit(&p, p.dirty);
  sparc_integer_branch(&done, 0x10800000); /* ba */
  for (i = 0; i < p.units; i++) {
    const struct pikecode_ssa_integer_unit *u = &p.unit[i];
    sparc_integer_bind(&overflow[i]);
    if (u->count > 1)
      SPARC_OR(sparc_integer_reg(&p.value[u->result[0]]), SPARC_REG_O4,
               SPARC_REG_G0, 0);
    sparc_integer_commit(&p, u->dirty_before);
    sparc_integer_branch(&slow[i], 0x10800000);
  }
  for (i = 0; i < p.units; i++) {
    const struct pikecode_ssa_integer_unit *u = &p.unit[i];
    sparc_integer_bind(&slow[i]);
    for (j = 0; j < u->length; j++) {
      const struct p_instr_s *op = &code[u->first + j];
      unsigned int flags = instrs[op->opcode - F_OFFSET].flags;
      if (flags & I_HASARG2)
        ins_f_byte_with_2_args(op->opcode, op->arg, op->arg2);
      else if (flags & I_HASARG) ins_f_byte_with_arg(op->opcode, op->arg);
      else ins_f_byte(op->opcode);
    }
  }
  /* Flush only on the slow fallthrough. The fast branch skips this code;
   * both paths leave the compiler's cache description conservatively empty. */
  sparc_flush_codegen_state();
  sparc_integer_bind(&done);
  if (sparc_ssa_stats) {
    for (i = 0; i < p.locals; i++) stores += !!(p.dirty & (1U << i));
    sparc_stats.integer_regions++;
    sparc_stats.integer_ops += p.count;
    sparc_stats.integer_values += p.values;
    sparc_stats.integer_loads_avoided += p.loads - p.locals;
    sparc_stats.integer_stores_avoided += p.count - stores;
  }
  return p.consumed;
#else
  (void)code;
  (void)available;
  return 0;
#endif
}
