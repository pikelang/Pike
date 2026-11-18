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
 * Balanced regions and isolated separate-local additions keep proven L0/L1
 * values through their entry flush and retain L0 at the replay merge. A
 * single-step overflow with no completed writes uses the guard replay entry
 * when execution counters are disabled.
 * Rollback and profiled exits retain their separate paths.
 *
 * Isolated separate-local additions combine both source tags and the
 * destination reference bit into one guard branch. Every branch delay
 * slot is useful: an operand load after the guard, a move into the free
 * result bank after overflow, and the tag store after success. No annulment
 * is needed; failed guards/overflow leave the original locals and stack intact.
 * Replay starts with unchanged entry IP/FP and canonical stack state;
 * ordinary opcodes handle cache invalidation without extra per-opcode
 * flushes. Only IP is retained after callbacks. Other plans retain their
 * snapshot flushes and conservative NOP slots around live intermediate banks.
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
/* O3 is dead at counter sites; O5 is an immediate/address scratch. Keep
 * O4 intact for balanced-unit rollback. Use only V7 instructions on SPARC32.
 */
static void sparc_integer_count(struct pikecode_ssa_integer_profile *p,
                                enum pikecode_ssa_integer_counter counter)
{
  size_t addr;
  if (!p) return;
  addr = (size_t)&p->count[counter];
#ifdef PIKE_BYTECODE_SPARC64
  /* Build the complete pointer, without SET_REG's immediate-range limit. */
  SPARC_SETHI(SPARC_REG_O5, addr >> 32);
  SPARC_OR(SPARC_REG_O5, SPARC_REG_O5, (addr >> 32) & 0x3ff, 1);
  SPARC_ALU_OP(SPARC_OP3_SLL, SPARC_REG_O5, SPARC_REG_O5, 0x1020, 1);
  SPARC_SETHI(SPARC_REG_O3, addr & 0xffffffff);
  SPARC_OR(SPARC_REG_O3, SPARC_REG_O3, addr & 0x3ff, 1);
  SPARC_OR(SPARC_REG_O5, SPARC_REG_O5, SPARC_REG_O3, 0);
  SPARC_LDX(SPARC_REG_O3, SPARC_REG_O5, 0, 1);
  SPARC_ADD(SPARC_REG_O3, SPARC_REG_O3, 1, 1);
  SPARC_STX(SPARC_REG_O3, SPARC_REG_O5, 0, 1);
#else
  SPARC_SETHI(SPARC_REG_O5, addr);
  SPARC_OR(SPARC_REG_O5, SPARC_REG_O5, addr & 0x3ff, 1);
#if PIKE_BYTEORDER == 4321
#define INTEGER_COUNT_LOW 4
#define INTEGER_COUNT_HIGH 0
#else
#define INTEGER_COUNT_LOW 0
#define INTEGER_COUNT_HIGH 4
#endif
  SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O5, INTEGER_COUNT_LOW, 1);
  SPARC_ALU_OP(SPARC_OP3_ADDcc, SPARC_REG_O3, SPARC_REG_O3, 1, 1);
  SPARC_STW(SPARC_REG_O3, SPARC_REG_O5, INTEGER_COUNT_LOW, 1);
  SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O5, INTEGER_COUNT_HIGH, 1);
  SPARC_ALU_OP(0x08, SPARC_REG_O3, SPARC_REG_O3, 0, 1); /* addx */
  SPARC_STW(SPARC_REG_O3, SPARC_REG_O5, INTEGER_COUNT_HIGH, 1);
#undef INTEGER_COUNT_LOW
#undef INTEGER_COUNT_HIGH
#endif
}

struct sparc_integer_label {
  unsigned int count;
  INT32 use[PIKE_SSA_INTEGER_LIMIT];
};

static int sparc_integer_reg(const struct pikecode_ssa_integer_value *value)
{
  return SPARC_REG_O0 + value->bank;
}

/* The caller must emit exactly one instruction in the delay slot. */
static void sparc_integer_branch_start(struct sparc_integer_label *target,
                                       unsigned INT32 instruction)
{
  if (target->count == NELEM(target->use))
    Pike_fatal("Too many SPARC integer branch references.\n");
  target->use[target->count++] = Pike_compiler->new_program->num_program;
  add_to_program(instruction);
}

static void sparc_integer_branch(struct sparc_integer_label *target,
                                 unsigned INT32 instruction)
{
  sparc_integer_branch_start(target, instruction);
  /* General plans can still need every input bank on a side exit. */
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
static int sparc_integer_value(
    const struct pikecode_ssa_integer_value *v, int scratch)
{
  if (v->kind != PIKE_SSA_INTEGER_CONSTANT) return sparc_integer_reg(v);
  SET_REG(scratch, v->imm);
  return scratch;
}

static void sparc_integer_snapshot(
    const struct pikecode_ssa_integer_stack_plan *p,
    const struct pikecode_ssa_integer_snapshot *s)
{
  unsigned int i;
  /* Copy input tags before committing locals that might own those tags. */
  if (s->depth || p->inputs) {
    PIKE_LDPTR(SPARC_REG_O4, SPARC_REG_PIKE_IP,
               OFFSETOF(Pike_interpreter_struct, stack_pointer), 1);
    if (p->inputs)
      SPARC_ADD(SPARC_REG_O4, SPARC_REG_O4, -(INT32)sizeof(struct svalue), 1);
    for (i = 0; i < s->depth; i++) {
      const struct pikecode_ssa_integer_value *v = &p->value[s->stack[i]];
      ptrdiff_t off = i * sizeof(struct svalue);
      int reg = sparc_integer_value(v, SPARC_REG_O3);
      SPARC_INTEGER_ST(reg, SPARC_REG_O4,
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
    const struct pikecode_ssa_integer_value *v;
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    int reg;
    if (!(s->dirty & (1U << i))) continue;
    v = &p->value[s->local[i]];
    reg = sparc_integer_value(v, SPARC_REG_O3);
    SPARC_INTEGER_ST(reg, SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
    SPARC_STW(SPARC_REG_G0, SPARC_REG_L5,
               off + OFFSETOF(svalue, tu.t.type), 1);
  }
}

/* The identity guard has already selected indirect indexing. Preserve the
 * opcode's destructed-object error and publish SP after a getter callback. */
static void sparc_integer_global_indirect(struct object *o, ptrdiff_t index)
{
  if (!o->prog) object_low_set_index(o, 0, NULL);
  low_object_index_no_free(Pike_sp, o, index);
  Pike_sp++;
}

/* Copy the complete svalue, including padding/float payload on SPARC32.
 * O4 is the direct storage address retained by all entry guards. */
static void sparc_integer_push_global(void)
{
  struct sparc_integer_label unreferenced = {0};
  size_t off;
  LOAD_PIKE_SP();
  for (off = 0; off < sizeof(struct svalue); off += 4) {
    SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O4, off, 1);
    SPARC_STW(SPARC_REG_O3, SPARC_REG_PIKE_SP,
               sparc_pike_sp_bias + off, 1);
  }
  SPARC_LDUH(SPARC_REG_O3, SPARC_REG_O4, OFFSETOF(svalue, tu.t.type), 1);
  SPARC_ANDcc(SPARC_REG_G0, SPARC_REG_O3, MIN_REF_TYPE, 1);
  sparc_integer_branch(&unreferenced, 0x02800000); /* be %icc */
  PIKE_LDPTR(SPARC_REG_O5, SPARC_REG_O4, OFFSETOF(svalue, u.refs), 1);
  SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O5, OFFSETOF(pike_string, refs), 1);
  SPARC_ADD(SPARC_REG_O3, SPARC_REG_O3, 1, 1);
  SPARC_STW(SPARC_REG_O3, SPARC_REG_O5, OFFSETOF(pike_string, refs), 1);
  sparc_integer_bind(&unreferenced);
  sparc_pike_sp_bias += sizeof(struct svalue);
  SPARC_SSA_DEFINE(SP, 1);
  sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;
}

/* Publish stack state while retaining proven callee-saved pointers. */
static void sparc_integer_balanced_entry(void)
{
  unsigned int keep = sparc_codegen_state &
    (SPARC_CODEGEN_IP_IS_SET | SPARC_CODEGEN_FP_IS_SET);
  /* Publish dirty stack state without forgetting unchanged L0/L1.
   * This is a local flush, independent of optional forward joins. */
  sparc_flush_codegen_state();
  if (keep & SPARC_CODEGEN_IP_IS_SET)
    pikecode_ssa_merge_reg(&sparc_ssa, PIKE_SSA_IP);
  if (keep & SPARC_CODEGEN_FP_IS_SET)
    pikecode_ssa_merge_reg(&sparc_ssa, PIKE_SSA_FP);
  sparc_codegen_state |= keep;
  /* FP can be known without IP at an incoming join. Establish L0 on every
   * path so it can also remain known after the replay/fast merge. */
  LOAD_PIKE_INTERPRETER();
}

/* This isolated assignment has two guarded operands and a free result
 * bank. Every guard/overflow exit replays the whole untouched source unit,
 * so speculative register writes are safe in its branch delay slots. */
static void sparc_integer_single_assign(
    const struct pikecode_ssa_integer_stack_plan *p,
    struct pikecode_ssa_integer_profile *profile,
    struct sparc_integer_label *guard, struct sparc_integer_label *overflow,
    struct sparc_integer_label *done, const struct p_instr_s *code,
    int store_lines)
{
  const struct pikecode_ssa_integer_value *result =
    &p->value[p->step[0].result];
  unsigned int i, dst = p->locals - 1;
  ptrdiff_t off = p->local[dst] * sizeof(struct svalue);
  /* An ordinary integer tag is zero. OR the source tags with the
   * destination's reference bit: every failure uses the same replay entry.
   * Only the first source needs a full tag to preserve UNDEFINED sums. */
  for (i = 1; i < p->locals; i++) {
    const struct pikecode_ssa_integer_value *v = &p->value[i];
    ptrdiff_t input = v->local * sizeof(struct svalue);
    int reg = i == 1 ? SPARC_REG_O3 : SPARC_REG_O5;
    if (p->normal & (1U << (i - 1)))
      SPARC_LDUW(reg, SPARC_REG_L5,
                 input + OFFSETOF(svalue, tu.t.type), 1);
    else
      SPARC_LDUH(reg, SPARC_REG_L5,
                 input + OFFSETOF(svalue, tu.t.type), 1);
    if (i != 1) SPARC_OR(SPARC_REG_O3, SPARC_REG_O3, reg, 0);
  }
  pikecode_ssa_integer_source(&code[p->step[0].first], store_lines);
  SPARC_LDUH(SPARC_REG_O5, SPARC_REG_L5,
             off + OFFSETOF(svalue, tu.t.type), 1);
  SPARC_ANDcc(SPARC_REG_O5, SPARC_REG_O5, MIN_REF_TYPE, 1);
  SPARC_ALU_OP(SPARC_OP3_ORcc, SPARC_REG_G0, SPARC_REG_O3,
               SPARC_REG_O5, 0);
  sparc_integer_branch_start(guard, 0x12800000); /* bne %icc */
  /* Reading unused payload bits is harmless if any tag check fails. */
  SPARC_INTEGER_LD(sparc_integer_reg(&p->value[result->left]), SPARC_REG_L5,
                   p->value[result->left].local * sizeof(struct svalue) +
                   OFFSETOF(svalue, u.integer), 1);
  if (result->right != result->left)
    SPARC_INTEGER_LD(sparc_integer_reg(&p->value[result->right]), SPARC_REG_L5,
                     p->value[result->right].local * sizeof(struct svalue) +
                     OFFSETOF(svalue, u.integer), 1);
  SPARC_ALU_OP(SPARC_OP3_ADDcc, SPARC_REG_O3,
               sparc_integer_reg(&p->value[result->left]),
               sparc_integer_reg(&p->value[result->right]), 0);
  sparc_integer_branch_start(overflow, SPARC_INTEGER_BVS);
  /* The free result bank is never read by guard or overflow replay. */
  SPARC_OR(sparc_integer_reg(result), SPARC_REG_O3, SPARC_REG_G0, 0);
  SPARC_INTEGER_ST(sparc_integer_reg(result), SPARC_REG_L5,
                   off + OFFSETOF(svalue, u.integer), 1);
  sparc_integer_count(profile, PIKE_SSA_INTEGER_FAST);
  sparc_integer_branch_start(done, 0x10800000);
  SPARC_STW(SPARC_REG_G0, SPARC_REG_L5,
             off + OFFSETOF(svalue, tu.t.type), 1);
}

static int sparc_integer_stack_run(const struct p_instr_s *code,
                                   ptrdiff_t available, int store_lines,
                                   struct pikecode_ssa_integer_stop *stop)
{
  struct pikecode_ssa_integer_stack_plan p;
  struct pikecode_ssa_integer_profile *profile;
  struct sparc_integer_label overflow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label slow[PIKE_SSA_INTEGER_CODE] = {{0}};
  struct sparc_integer_label done = {0}, guard = {0}, indirect = {0};
  struct sparc_integer_label prepared = {0};
  unsigned int i, stores = 0, write_only = 0;
  if (sparc_ssa_no_integer_stack ||
      !pikecode_ssa_plan_integer_stack(&p, code, available,
        (4095 - OFFSETOF(svalue, u.integer)) / sizeof(struct svalue),
        !sparc_ssa_no_integer_stack_exits, !sparc_ssa_no_integer_subtract,
        !sparc_ssa_no_integer_bitwise, stop))
    return 0;
  profile = pikecode_ssa_integer_profile_new(code, available, p.consumed,
                                             p.count, p.locals, "stack", stop);
  if (p.write_only) sparc_integer_balanced_entry();
  else sparc_flush_codegen_state();
  sparc_update_pc();
  sparc_integer_count(profile, PIKE_SSA_INTEGER_ENTRIES);
  LOAD_PIKE_FP();
  if (p.global_input) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.locals + 1];
    int reg = sparc_integer_reg(v);
    PIKE_LDPTR(SPARC_REG_O4, SPARC_REG_PIKE_FP,
               OFFSETOF(pike_frame, current_object), 1);
    PIKE_LDPTR(SPARC_REG_O5, SPARC_REG_PIKE_FP,
               OFFSETOF(pike_frame, context), 1);
    PIKE_LDPTR(SPARC_REG_O3, SPARC_REG_O4, OFFSETOF(object, prog), 1);
    PIKE_LDPTR(reg, SPARC_REG_O5, OFFSETOF(inherit, prog), 1);
    /* The active context program is live. Compare full pointers, even
     * when native integer arithmetic uses narrower condition codes. */
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, reg, 0);
#ifdef PIKE_BYTECODE_SPARC64
    sparc_integer_branch(&indirect, 0x12600000);
#else
    sparc_integer_branch(&indirect, 0x12800000);
#endif
    PIKE_LDPTR(SPARC_REG_O4, SPARC_REG_O4, OFFSETOF(object, storage), 1);
    PIKE_LDPTR(SPARC_REG_O5, SPARC_REG_O5,
               OFFSETOF(inherit, storage_offset), 1);
    SPARC_ADD(SPARC_REG_O4, SPARC_REG_O4, SPARC_REG_O5, 0);
    if (p.global_offset) {
      SET_REG(SPARC_REG_O5, p.global_offset);
      SPARC_ADD(SPARC_REG_O4, SPARC_REG_O4, SPARC_REG_O5, 0);
    }
    SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O4, OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    sparc_integer_branch((profile || p.global_input) ? &guard : &slow[0],
                         0x12800000);
    SPARC_INTEGER_LD(reg, SPARC_REG_O4, OFFSETOF(svalue, u.integer), 1);
  }
  PIKE_LDPTR(SPARC_REG_L5, SPARC_REG_PIKE_FP,
             OFFSETOF(pike_frame, locals), 1);
  if (p.write_only) {
    sparc_integer_single_assign(&p, profile, profile ? &guard : &slow[0],
                                profile ? &overflow[0] : &slow[0], &done,
                                code, store_lines);
    goto overflow_exits;
  }
  for (i = 1; i <= p.locals; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    /* Ordinary integer type and subtype form a zero tag word. */
    if (p.normal & (1U << (i - 1)))
      SPARC_LDUW(SPARC_REG_O3, SPARC_REG_L5,
                 off + OFFSETOF(svalue, tu.t.type), 1);
    else
      SPARC_LDUH(SPARC_REG_O3, SPARC_REG_L5,
                 off + OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    /* bne %icc */
    sparc_integer_branch((profile || p.global_input) ? &guard : &slow[0],
                         0x12800000);
    SPARC_INTEGER_LD(sparc_integer_reg(v), SPARC_REG_L5,
                     off + OFFSETOF(svalue, u.integer), 1);
  }
  if (p.inputs) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.locals + 1];
    ptrdiff_t off = -(ptrdiff_t)sizeof(struct svalue);
    PIKE_LDPTR(SPARC_REG_O4, SPARC_REG_PIKE_IP,
               OFFSETOF(Pike_interpreter_struct, stack_pointer), 1);
    /* Full tag guard; the original operand remains available for replay. */
    SPARC_LDUW(SPARC_REG_O3, SPARC_REG_O4,
               off + OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    sparc_integer_branch((profile || p.global_input) ? &guard : &slow[0],
                         0x12800000);
    SPARC_INTEGER_LD(sparc_integer_reg(v), SPARC_REG_O4,
                     off + OFFSETOF(svalue, u.integer), 1);
  }
  for (i = 0; i < p.count; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.step[i].result];
    int right;
    pikecode_ssa_integer_source(&code[p.step[i].first], store_lines);
    if (v->kind == PIKE_SSA_INTEGER_NEGATE) {
      int left = sparc_integer_value(&p.value[v->left], SPARC_REG_O5);
      SPARC_ALU_OP(SPARC_OP3_SUBcc, SPARC_REG_O3, SPARC_REG_G0, left, 0);
      sparc_integer_branch(&overflow[i], SPARC_INTEGER_BVS);
    } else if (v->kind == PIKE_SSA_INTEGER_SUM) {
      unsigned int j;
      int left = sparc_integer_value(&p.value[v->left], SPARC_REG_O5);
      SPARC_OR(SPARC_REG_O3, left, SPARC_REG_G0, 0);
      for (j = 1; j < v->args; j++) {
        right = sparc_integer_value(&p.value[v->operand[j]], SPARC_REG_O5);
        SPARC_ALU_OP(SPARC_OP3_ADDcc, SPARC_REG_O3, SPARC_REG_O3, right, 0);
        sparc_integer_branch(&overflow[i], SPARC_INTEGER_BVS);
      }
    } else {
      int left;
      if (v->right)
        right = sparc_integer_value(&p.value[v->right], SPARC_REG_O5);
      else {
        SET_REG(SPARC_REG_O5, v->imm);
        right = SPARC_REG_O5;
      }
      /* A constant left operand needs a different scratch from right. */
      left = sparc_integer_value(&p.value[v->left], SPARC_REG_O3);
      if (integer_stack_bitwise(v->kind)) {
        unsigned int op3 = v->kind == PIKE_SSA_INTEGER_AND ? 0x01 :
          v->kind == PIKE_SSA_INTEGER_OR ? 0x02 : 0x03;
        SPARC_ALU_OP(op3, SPARC_REG_O3, left, right, 0);
      } else {
        SPARC_ALU_OP(v->kind == PIKE_SSA_INTEGER_SUBTRACT ?
                     SPARC_OP3_SUBcc : SPARC_OP3_ADDcc,
                     SPARC_REG_O3, left, right, 0);
        sparc_integer_branch(&overflow[i], SPARC_INTEGER_BVS);
      }
    }
    SPARC_OR(sparc_integer_reg(v), SPARC_REG_O3, SPARC_REG_G0, 0);
  }
  if (p.global_store) {
    /* O4 retains the entry-guarded storage address through all arithmetic.
     * Store before the final snapshot borrows it for stack publication. */
    int reg;
    pikecode_ssa_integer_source(&code[p.consumed - 1 - p.global_pop],
                                store_lines);
    reg = sparc_integer_value(&p.value[p.global_store], SPARC_REG_O3);
    SPARC_INTEGER_ST(reg, SPARC_REG_O4, OFFSETOF(svalue, u.integer), 1);
  }
  /* Publish live results before the next ordinary opcode. The slow replay
   * finishes at the same stack depth with empty machine caches. */
  sparc_integer_snapshot(&p, &p.final);
  sparc_integer_count(profile, PIKE_SSA_INTEGER_FAST);
  sparc_integer_branch(&done, 0x10800000);
 overflow_exits:
  for (i = 0; i < p.count; i++) {
    if (integer_stack_bitwise(p.value[p.step[i].result].kind)) continue;
    if (p.write_only && !profile) continue;
    sparc_integer_bind(&overflow[i]);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_OVERFLOWS);
    if (p.write_only) {
      /* No local or real stack slot has changed; replay the source loads. */
      sparc_integer_branch(&slow[0], 0x10800000);
      continue;
    }
    sparc_integer_snapshot(&p, &p.step[i].before);
    sparc_integer_branch(&slow[p.step[i].first], 0x10800000);
  }
  if (profile && !p.global_input) {
    sparc_integer_bind(&guard);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    sparc_integer_branch(&slow[0], 0x10800000);
  }
  for (i = p.global_input; i < p.consumed; i++) {
    const struct p_instr_s *op = &code[i];
    unsigned int flags = instrs[op->opcode - F_OFFSET].flags;
    /* Other plans enter with different stack snapshots. The isolated
     * assignment has no stack changes and every edge preserves entry
     * IP/FP. Let ordinary replay invalidate caches as its opcodes require;
     * no side exit enters between those opcodes. */
    if (!p.write_only) sparc_flush_codegen_state();
    sparc_integer_bind(&slow[i]);
    pikecode_ssa_integer_source(op, store_lines);
    if (flags & I_HASARG2)
      ins_f_byte_with_2_args(op->opcode, op->arg, op->arg2);
    else if (flags & I_HASARG) ins_f_byte_with_arg(op->opcode, op->arg);
    else ins_f_byte(op->opcode);
  }
  sparc_flush_codegen_state();
  if (p.global_input) {
    struct pikecode_ssa_integer_stop fallback_stop = {0};
    int consumed;
    sparc_integer_branch(&done, 0x10800000);
    sparc_integer_bind(&guard);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    pikecode_ssa_integer_source(code, store_lines);
    sparc_integer_push_global();
    sparc_flush_codegen_state();
    sparc_integer_branch(&prepared, 0x10800000);
    sparc_integer_bind(&indirect);
    /* O4/O5 still hold the object/context. Prepare arguments before the
     * counter clobbers O5; the initial identity check needs no repetition. */
    SPARC_OR(SPARC_REG_O0, SPARC_REG_O4, SPARC_REG_G0, 0);
    SPARC_LDUH(SPARC_REG_O1, SPARC_REG_O5,
               OFFSETOF(inherit, identifier_level), 1);
    SET_REG(SPARC_REG_O2, code[0].arg2);
    SPARC_ADD(SPARC_REG_O1, SPARC_REG_O1, SPARC_REG_O2, 0);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    sparc_update_pc();
    low_ins_call(sparc_integer_global_indirect, 0, I_UPDATE_SP);
    sparc_flush_codegen_state();
    sparc_integer_bind(&prepared);
    /* Re-guard after any getter callback and retain the existing native
     * arithmetic for integer values from indirect/inherited contexts. */
    consumed = 1 + sparc_integer_stack_run(code + 1, p.consumed - 1,
                   store_lines,
                   integer_profile_enabled ? &fallback_stop : NULL);
    for (i = consumed; i < p.consumed; i++) {
      const struct p_instr_s *op = &code[i];
      unsigned int flags = instrs[op->opcode - F_OFFSET].flags;
      pikecode_ssa_integer_source(op, store_lines);
      if (flags & I_HASARG2)
        ins_f_byte_with_2_args(op->opcode, op->arg, op->arg2);
      else if (flags & I_HASARG) ins_f_byte_with_arg(op->opcode, op->arg);
      else ins_f_byte(op->opcode);
    }
    sparc_flush_codegen_state();
  }
  sparc_integer_bind(&done);
  if (p.global_store || p.write_only) {
    /* Entry establishes L0 on every path, and C callees preserve it.
     * Only IP is common after callbacks in ordinary replay. */
    pikecode_ssa_merge_reg(&sparc_ssa, PIKE_SSA_IP);
    sparc_codegen_state |= SPARC_CODEGEN_IP_IS_SET;
  }
  pikecode_ssa_integer_source(&code[p.consumed - 1], store_lines);
  /* Both paths must publish a PC for the final source position: the
   * slow-call emitter's last-location cache also describes skipped code. */
  if (pikecode_ssa_integer_multiline(code, p.consumed)) sparc_update_pc();
  if (sparc_ssa_stats) {
    for (i = 0; i < p.locals; i++) {
      stores += !!(p.final.dirty & (1U << i));
      write_only += !!(p.write_only & (1U << i));
    }
    sparc_stats.integer_regions++;
    sparc_stats.integer_stack_regions++;
    if (p.final.depth) sparc_stats.integer_stack_exits++;
    sparc_stats.integer_ops += p.count;
    sparc_stats.integer_values += p.values;
    sparc_stats.integer_bitwise_ops += p.bitwise;
    for (i = 0; i < p.count; i++)
      if (p.value[p.step[i].result].kind == PIKE_SSA_INTEGER_SUBTRACT)
        sparc_stats.integer_subtract_ops++;
    sparc_stats.integer_loads_avoided +=
      p.loads + write_only - p.locals - p.inputs - p.global_input;
    sparc_stats.integer_stores_avoided += p.writes - stores;
  }
  return p.consumed;
}
#endif

int sparc_integer_run(const struct p_instr_s *code, ptrdiff_t available,
                       int store_lines)
{
#ifdef SPARC_INTEGER_LD
  struct pikecode_ssa_integer_plan p;
  struct pikecode_ssa_integer_profile *profile;
  struct sparc_integer_label overflow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label slow[PIKE_SSA_INTEGER_LIMIT] = {{0}};
  struct sparc_integer_label done = {0}, guard = {0};
  unsigned int i, j, stores = 0;
  struct pikecode_ssa_integer_stop stack_stop = {0}, stop = {0};
  if (!sparc_integer_regions_enabled()) return 0;
  {
    int consumed = sparc_integer_stack_run(code, available, store_lines,
                     integer_profile_enabled ? &stack_stop : NULL);
    if (consumed) return consumed;
  }
  if (!pikecode_ssa_plan_integer(&p, code, available,
        (4095 - OFFSETOF(svalue, u.integer)) / sizeof(struct svalue),
        integer_profile_enabled ? &stop : NULL)) {
    const char *planner = "balanced";
    if (integer_profile_enabled && stack_stop.candidate) {
      stop = stack_stop;
      planner = "stack";
    }
    if (integer_profile_enabled && stop.candidate) {
      profile = pikecode_ssa_integer_profile_new(code, available, 0,
                                      stop.ops, stop.locals, planner, &stop);
      /* Ordinary opcode boundaries keep no arithmetic temporary live.
       * Publish cached state before using the counter scratch registers. */
      sparc_flush_codegen_state();
      sparc_integer_count(profile, PIKE_SSA_INTEGER_REJECTED);
    }
    return 0;
  }

  profile = pikecode_ssa_integer_profile_new(code, available, p.consumed,
                                             p.count, p.locals, "balanced",
                                      integer_profile_enabled ? &stop : NULL);
  sparc_integer_balanced_entry();
  sparc_update_pc();
  sparc_integer_count(profile, PIKE_SSA_INTEGER_ENTRIES);
  LOAD_PIKE_FP();
  PIKE_LDPTR(SPARC_REG_L5, SPARC_REG_PIKE_FP,
            OFFSETOF(pike_frame, locals), 1);
  for (i = 1; i <= p.values; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (v->kind != PIKE_SSA_INTEGER_LOAD) continue;
    if (p.normal & (1U << v->bank))
      SPARC_LDUW(SPARC_REG_O3, SPARC_REG_L5,
                 off + OFFSETOF(svalue, tu.t.type), 1);
    else
      SPARC_LDUH(SPARC_REG_O3, SPARC_REG_L5,
                 off + OFFSETOF(svalue, tu.t.type), 1);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_O3, PIKE_T_INT, 1);
    /* bne %icc */
    sparc_integer_branch(profile ? &guard : &slow[0], 0x12800000);
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
      /* A fused unit's final add follows its local load/increment. */
      unsigned int source = u->first;
      if (u->length > 1 && (u->count == 1 || j)) source++;
      pikecode_ssa_integer_source(&code[source], store_lines);
      if (v->right) right = sparc_integer_reg(&p.value[v->right]);
      else {
        SET_REG(SPARC_REG_O5, v->imm);
        right = SPARC_REG_O5;
      }
      SPARC_ALU_OP(0x10, SPARC_REG_O3,
                   sparc_integer_reg(&p.value[v->left]), right, 0); /* addcc */
      sparc_integer_branch(!profile && u->count == 1 && !u->dirty_before ?
                            &slow[i] : &overflow[i], SPARC_INTEGER_BVS);
      SPARC_OR(sparc_integer_reg(v), SPARC_REG_O3, SPARC_REG_G0, 0);
    }
  }
  sparc_integer_commit(&p, p.dirty);
  sparc_integer_count(profile, PIKE_SSA_INTEGER_FAST);
  sparc_integer_branch(&done, 0x10800000); /* ba */
  for (i = 0; i < p.units; i++) {
    const struct pikecode_ssa_integer_unit *u = &p.unit[i];
    /* No bank needs restoration and no completed local needs publishing.
     * Keep separate exits when collecting guard/overflow counters. */
    if (!profile && u->count == 1 && !u->dirty_before) continue;
    sparc_integer_bind(&overflow[i]);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_OVERFLOWS);
    if (u->count > 1)
      SPARC_OR(sparc_integer_reg(&p.value[u->result[0]]), SPARC_REG_O4,
               SPARC_REG_G0, 0);
    sparc_integer_commit(&p, u->dirty_before);
    sparc_integer_branch(&slow[i], 0x10800000);
  }
  if (profile) {
    sparc_integer_bind(&guard);
    sparc_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    sparc_integer_branch(&slow[0], 0x10800000);
  }
  for (i = 0; i < p.units; i++) {
    const struct pikecode_ssa_integer_unit *u = &p.unit[i];
    sparc_integer_bind(&slow[i]);
    for (j = 0; j < u->length; j++) {
      const struct p_instr_s *op = &code[u->first + j];
      unsigned int flags = instrs[op->opcode - F_OFFSET].flags;
      pikecode_ssa_integer_source(op, store_lines);
      if (flags & I_HASARG2)
        ins_f_byte_with_2_args(op->opcode, op->arg, op->arg2);
      else if (flags & I_HASARG) ins_f_byte_with_arg(op->opcode, op->arg);
      else ins_f_byte(op->opcode);
    }
  }
  /* Flush only on the slow fallthrough. The fast branch skips this code;
   * invalidate callback-sensitive state on both paths at the merge. */
  sparc_flush_codegen_state();
  sparc_integer_bind(&done);
  /* Entry established L0 and every returning C opcode preserves it.
   * FP/SP/locals remain invalid: replay may have invoked Pike callbacks. */
  pikecode_ssa_merge_reg(&sparc_ssa, PIKE_SSA_IP);
  sparc_codegen_state |= SPARC_CODEGEN_IP_IS_SET;
  pikecode_ssa_integer_source(&code[p.consumed - 1], store_lines);
  /* Both paths must publish a PC for the final source position: the
   * slow-call emitter's last-location cache also describes skipped code. */
  if (pikecode_ssa_integer_multiline(code, p.consumed)) sparc_update_pc();
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
  (void)store_lines;
  return 0;
#endif
}
