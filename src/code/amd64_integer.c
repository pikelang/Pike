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

/* RAX is dead at region entry, success, and cold exit labels. No call or
 * interpreter-state publication is needed for these opt-in increments. */
static void amd64_integer_count(struct pikecode_ssa_integer_profile *p,
                                enum pikecode_ssa_integer_counter counter)
{
  unsigned INT64 addr;
  if (!p) return;
  addr = (size_t)&p->count[counter];
  rex(1, 0, 0, P_REG_RAX);
  opcode(0xb8); /* movabs counter, %rax (also correct with LLP64). */
  id((unsigned INT32)addr);
  id((unsigned INT32)(addr >> 32));
  rex(1, 0, 0, P_REG_RAX);
  opcode(0xff); /* incq (%rax), under the interpreter lock. */
  modrm(0, 0, P_REG_RAX);
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

static void amd64_integer_value(
    const struct pikecode_ssa_integer_value *v, enum amd64_reg reg)
{
  if (v->kind == PIKE_SSA_INTEGER_CONSTANT) mov_imm_reg(v->imm, reg);
  else mov_reg_reg(amd64_integer_reg(v), reg);
}

/* Publish a side exit before any C call. Input tags are copied before local
 * commits, since a dirty local may still hold an input's original subtype. */
static void amd64_integer_snapshot(
    const struct pikecode_ssa_integer_stack_plan *p,
    const struct pikecode_ssa_integer_snapshot *s)
{
  unsigned int i;
  if (s->depth || p->inputs) {
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer), P_REG_RDX);
    if (p->inputs) add_reg_imm(P_REG_RDX, -(INT32)sizeof(struct svalue));
    for (i = 0; i < s->depth; i++) {
      const struct pikecode_ssa_integer_value *v = &p->value[s->stack[i]];
      ptrdiff_t off = i * sizeof(struct svalue);
      enum amd64_reg reg = amd64_integer_reg(v);
      if (v->kind == PIKE_SSA_INTEGER_CONSTANT) {
        amd64_integer_value(v, P_REG_RAX);
        reg = P_REG_RAX;
      }
      mov_reg_mem(reg, P_REG_RDX,
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
    const struct pikecode_ssa_integer_value *v;
    ptrdiff_t off = p->local[i] * sizeof(struct svalue);
    enum amd64_reg reg;
    if (!(s->dirty & (1U << i))) continue;
    v = &p->value[s->local[i]];
    reg = amd64_integer_reg(v);
    if (v->kind == PIKE_SSA_INTEGER_CONSTANT) {
      amd64_integer_value(v, P_REG_RAX);
      reg = P_REG_RAX;
    }
    mov_reg_mem(reg, PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer));
    mov_imm_mem32(PIKE_T_INT, PIKE_LOCALS_REG,
                  off + OFFSETOF(svalue, tu.t.type));
  }
}

/* Resolve the first producer once, preserving its original direct or
 * indirect semantics. A pair then replays the second producer normally;
 * the one-input extension can instead compile its arithmetic suffix. */
static void amd64_integer_global_fallback(
    const struct p_instr_s *code, int store_lines,
    struct pikecode_ssa_integer_profile *profile,
    struct label *guard, struct label *indirect, struct label *prepared,
    struct label *replay)
{
  label(guard);
  amd64_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
  if (replay) label(replay);
  pikecode_ssa_integer_source(code, store_lines);
  /* No callback has run: RBX still addresses the guarded direct value. */
  amd64_push_svaluep(P_REG_RBX);
  amd64_flush_code_generator_state();
  jmp(prepared);
  label(indirect);
  amd64_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
  /* The program comparison already supplied the object and context.
   * Read the context first: ARG2 aliases RDX on Windows. */
  mov_mem16_reg(P_REG_RDX, OFFSETOF(inherit, identifier_level), ARG3_REG);
  mov_reg_reg(P_REG_RBX, ARG2_REG);
  add_reg_imm(ARG3_REG, code[0].arg2);
  amd64_load_sp_reg();
  mov_reg_reg(sp_reg, ARG1_REG);
  amd64_prev_stored_pc = -1;
  maybe_update_pc();
  amd64_call_c_function(low_object_index_no_free);
  amd64_add_sp(1);
  amd64_flush_code_generator_state();
  label(prepared);
}

static int amd64_integer_stack_run(const struct p_instr_s *code,
                                   ptrdiff_t available, int store_lines,
                                   struct pikecode_ssa_integer_stop *stop)
{
  struct pikecode_ssa_integer_stack_plan p;
  struct pikecode_ssa_integer_profile *profile;
  struct label overflow[PIKE_SSA_INTEGER_LIMIT];
  struct label slow[PIKE_SSA_INTEGER_CODE], done = { .addr = -1 };
  struct label guard = { .addr = -1 }, indirect = { .addr = -1 };
  struct label prepared = { .addr = -1 }, replay = { .addr = -1 };
  struct label entry = { .addr = -1 }, initialize = { .addr = -1 };
  int constant_entry;
  unsigned int i, stores = 0, write_only = 0;
  if (ssa_no_integer_stack ||
      !pikecode_ssa_plan_integer_stack(&p, code, available,
        (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
                sizeof(struct svalue)),
        !ssa_no_integer_stack_exits, !ssa_no_integer_subtract,
        !ssa_no_integer_bitwise, stop)) return 0;
  constant_entry = code[0].opcode == F_ASSIGN_LOCAL_NUMBER_AND_POP ||
    code[0].opcode == F_CLEAR_LOCAL;
  profile = pikecode_ssa_integer_profile_new(code, available, p.consumed,
                                             p.count, p.locals, "stack", stop);
  for (i = 0; i < p.count; i++) overflow[i] = (struct label){ .addr = -1 };
  for (i = 0; i < p.consumed; i++) slow[i] = (struct label){ .addr = -1 };
  amd64_flush_code_generator_state();
  amd64_ssa_region_begin();
  /* Keep wide branches, but do not relocate published source records. */
  if (p.global_input || constant_entry ||
      pikecode_ssa_integer_multiline(code, p.consumed))
    ssa_region.abandoned = 1;
  label(&entry);
  maybe_update_pc();
  amd64_integer_count(profile, PIKE_SSA_INTEGER_ENTRIES);
  amd64_load_fp_reg();
  if (p.global_input) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.locals + 1];
    enum amd64_reg reg = amd64_integer_reg(v);
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, current_object), P_REG_RBX);
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, context), P_REG_RDX);
    mov_mem_reg(P_REG_RBX, OFFSETOF(object, prog), P_REG_RAX);
    mov_mem_reg(P_REG_RDX, OFFSETOF(inherit, prog), reg);
    /* The active context owns a live program, so equality also excludes
     * destructed objects. The input bank is free scratch until its load. */
    cmp_reg_reg(P_REG_RAX, reg);
    jne(&indirect);
    mov_mem_reg(P_REG_RBX, OFFSETOF(object, storage), P_REG_RBX);
    add_reg_mem(P_REG_RBX, P_REG_RDX, OFFSETOF(inherit, storage_offset));
    add_reg_imm(P_REG_RBX, p.global_offset);
    mov_mem32_reg(P_REG_RBX, OFFSETOF(svalue, tu.t.type), P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(&guard);
    mov_mem_reg(P_REG_RBX, OFFSETOF(svalue, u.integer), reg);
    if (p.global_input == 2) {
      reg = amd64_integer_reg(&p.value[2]);
      mov_reg_reg(P_REG_RBX, P_REG_RDX);
      add_reg_imm(P_REG_RDX, p.global_second_offset - p.global_offset);
      mov_mem32_reg(P_REG_RDX, OFFSETOF(svalue, tu.t.type), P_REG_RAX);
      cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
      jne(&guard);
      mov_mem_reg(P_REG_RDX, OFFSETOF(svalue, u.integer), reg);
    }
  }
  if (p.global_input != 2)
    mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), PIKE_LOCALS_REG);
  for (i = 1; i <= p.locals; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (p.write_only & (1U << (i - 1))) {
      /* Assignment must release reference values through ordinary replay. */
      mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type),
                    P_REG_RAX);
      and_reg32_imm(P_REG_RAX, MIN_REF_TYPE);
      jne(profile ? &guard : &slow[0]);
      continue;
    }
    /* Ordinary integer type and subtype form a zero tag word. */
    if (p.normal & (1U << (i - 1)))
      mov_mem32_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type),
                    P_REG_RAX);
    else
      mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type),
                    P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(constant_entry && i == 1 ? &initialize :
        (profile || p.global_input) ? &guard : &slow[0]);
    mov_mem_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, u.integer),
                amd64_integer_reg(v));
  }
  if (p.inputs) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.locals + 1];
    ptrdiff_t off = -(ptrdiff_t)sizeof(struct svalue);
    mov_mem_reg(Pike_interpreter_reg,
                OFFSETOF(Pike_interpreter_struct, stack_pointer), P_REG_RDX);
    /* Require the complete ordinary tag, including subtype. Do not alter
     * the real operand before every guard and speculative operation passes. */
    mov_mem32_reg(P_REG_RDX, off + OFFSETOF(svalue, tu.t.type), P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne((profile || p.global_input) ? &guard : &slow[0]);
    mov_mem_reg(P_REG_RDX, off + OFFSETOF(svalue, u.integer),
                amd64_integer_reg(v));
  }
  for (i = 0; i < p.count; i++) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.step[i].result];
    if (v->kind == PIKE_SSA_INTEGER_CONSTANT) continue;
    pikecode_ssa_integer_source(&code[p.step[i].first], store_lines);
    amd64_integer_value(&p.value[v->left], P_REG_RAX);
    if (v->kind == PIKE_SSA_INTEGER_NEGATE) {
      rex(1, 0, 0, P_REG_RAX);
      opcode(0xf7);
      modrm(3, 3, P_REG_RAX); /* negq %rax */
      jo(&overflow[i]);
    } else if (v->kind == PIKE_SSA_INTEGER_SUM) {
      unsigned int j;
      for (j = 1; j < v->args; j++) {
        const struct pikecode_ssa_integer_value *arg =
          &p.value[v->operand[j]];
        if (arg->kind == PIKE_SSA_INTEGER_CONSTANT)
          amd64_integer_alu_imm(P_REG_RAX, arg->imm, 0);
        else add_reg_reg(P_REG_RAX, amd64_integer_reg(arg));
        jo(&overflow[i]);
      }
    } else if (integer_stack_bitwise(v->kind)) {
      enum amd64_reg right;
      if (v->right &&
          p.value[v->right].kind != PIKE_SSA_INTEGER_CONSTANT)
        right = amd64_integer_reg(&p.value[v->right]);
      else if (v->right) {
        amd64_integer_value(&p.value[v->right], P_REG_RDX);
        right = P_REG_RDX;
      }
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
        const struct pikecode_ssa_integer_value *right = &p.value[v->right];
        if (right->kind == PIKE_SSA_INTEGER_CONSTANT)
          amd64_integer_alu_imm(P_REG_RAX, right->imm,
                                v->kind == PIKE_SSA_INTEGER_SUBTRACT);
        else if (v->kind == PIKE_SSA_INTEGER_SUBTRACT)
          sub_reg_reg(P_REG_RAX, amd64_integer_reg(right));
        else add_reg_reg(P_REG_RAX, amd64_integer_reg(right));
      } else
        amd64_integer_alu_imm(P_REG_RAX, v->imm,
                              v->kind == PIKE_SSA_INTEGER_SUBTRACT);
      jo(!profile && p.global_input == 2 ? &replay :
          !profile && p.write_only ? &slow[0] : &overflow[i]);
    }
    mov_reg_reg(P_REG_RAX, amd64_integer_reg(v));
  }
  if (p.global_store) {
    const struct pikecode_ssa_integer_value *v = &p.value[p.global_store];
    enum amd64_reg reg = amd64_integer_reg(v);
    /* RBX still addresses the entry-guarded ordinary integer. No callback
     * or global write can invalidate it before this successful commit. */
    pikecode_ssa_integer_source(&code[p.consumed - 1 - p.global_pop],
                                store_lines);
    if (v->kind == PIKE_SSA_INTEGER_CONSTANT) {
      amd64_integer_value(v, P_REG_RAX);
      reg = P_REG_RAX;
    }
    if (p.global_store_second)
      add_reg_imm(P_REG_RBX, p.global_second_offset - p.global_offset);
    mov_reg_mem(reg, P_REG_RBX, OFFSETOF(svalue, u.integer));
  }
  /* Publish live results before the next ordinary opcode. The slow replay
   * finishes at the same stack depth with empty machine caches. */
  amd64_integer_snapshot(&p, &p.final);
  amd64_integer_count(profile, PIKE_SSA_INTEGER_FAST);
  jmp(&done);
  for (i = 0; i < p.count; i++) {
    if (p.value[p.step[i].result].kind == PIKE_SSA_INTEGER_CONSTANT) continue;
    if (integer_stack_bitwise(p.value[p.step[i].result].kind)) continue;
    if ((p.write_only || p.global_input == 2) && !profile) continue;
    label(&overflow[i]);
    amd64_integer_count(profile, PIKE_SSA_INTEGER_OVERFLOWS);
    if (p.global_input == 2) {
      /* The isolated add has made no call, write or stack change. Both
       * guarded ordinary integers still reside in their original storage.
       * Resolve neither context again before copying the first input. */
      jmp(&replay);
      continue;
    }
    if (p.write_only) {
      /* The separate assignment has not changed any local or real stack
       * slot. Replay its loads instead of materializing virtual operands. */
      jmp(&slow[0]);
      continue;
    }
    amd64_integer_snapshot(&p, &p.step[i].before);
    jmp(&slow[p.step[i].first]);
  }
  if (constant_entry) {
    unsigned int instr = code[0].opcode - F_OFFSET;
    label(&initialize);
    amd64_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    amd64_flush_code_generator_state();
    pikecode_ssa_integer_source(code, store_lines);
    /* The first discovered local is the initializer's destination. Release
     * its old noninteger value once, using the ordinary opcode, then retry
     * with fresh guards. Replaying the entire region here would add calls
     * (and destructor checkpoints) absent from the previous fast suffix. */
    update_arg1(code[0].arg);
    if (instrs[instr].flags & I_HASARG2) update_arg2(code[0].arg2);
    amd64_call_c_opcode(instrs[instr].address, instrs[instr].flags);
    amd64_flush_code_generator_state();
    jmp(&entry);
  }
  if (profile && !p.global_input) {
    label(&guard);
    amd64_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    jmp(&slow[0]);
  }
  if (p.global_input == 2) {
    amd64_flush_code_generator_state();
    amd64_integer_global_fallback(code, store_lines, profile,
                                    &guard, &indirect, &prepared, &replay);
  }
  for (i = p.global_input ? 1 : 0; i < p.consumed; i++) {
    unsigned int instr = code[i].opcode - F_OFFSET;
    /* Incoming exits have different stack depths. Flush fallthrough before
     * binding, so all entries reload from canonical interpreter state. */
    if (p.global_input != 2)
      amd64_flush_code_generator_state();
    label(&slow[i]);
    pikecode_ssa_integer_source(&code[i], store_lines);
    if (p.global_input == 2) {
      if (instrs[instr].flags & I_HASARG2)
        ins_f_byte_with_2_args(code[i].opcode, code[i].arg, code[i].arg2);
      else if (instrs[instr].flags & I_HASARG)
        ins_f_byte_with_arg(code[i].opcode, code[i].arg);
      else ins_f_byte(code[i].opcode);
    } else {
      if (instrs[instr].flags & I_HASARG) update_arg1(code[i].arg);
      if (instrs[instr].flags & I_HASARG2) update_arg2(code[i].arg2);
      amd64_call_c_opcode(instrs[instr].address, instrs[instr].flags);
    }
  }
  amd64_flush_code_generator_state();
  if (p.global_input == 1) {
    struct pikecode_ssa_integer_stop fallback_stop = {0};
    int consumed;
    jmp(&done);
    amd64_integer_global_fallback(code, store_lines, profile,
                                    &guard, &indirect, &prepared, NULL);
    /* Keep parent branches wide and fixed across the independent fallback. */
    amd64_ssa_region_end();
    /* A getter or inherited context can still supply an integer. Preserve
     * the preceding incoming-stack fast path, with fresh post-call guards. */
    consumed = 1 + amd64_integer_stack_run(code + 1, p.consumed - 1,
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
    amd64_flush_code_generator_state();
  }
  label(&done);
  if (p.global_input != 1) amd64_ssa_region_end();
  pikecode_ssa_integer_source(&code[p.consumed - 1], store_lines);
  /* Slow calls may publish a PC which the fast branch never executed. */
  amd64_prev_stored_pc = -1;
  if (ssa_report_stats) {
    for (i = 0; i < p.locals; i++) {
      stores += !!(p.final.dirty & (1U << i));
      write_only += !!(p.write_only & (1U << i));
    }
    ssa_stats.integer_regions++;
    ssa_stats.integer_stack_regions++;
    if (p.final.depth) ssa_stats.integer_stack_exits++;
    ssa_stats.integer_ops += p.count;
    ssa_stats.integer_values += p.values;
    ssa_stats.integer_bitwise_ops += p.bitwise;
    for (i = 0; i < p.count; i++)
      if (p.value[p.step[i].result].kind == PIKE_SSA_INTEGER_SUBTRACT)
        ssa_stats.integer_subtract_ops++;
    ssa_stats.integer_loads_avoided +=
      p.loads + write_only - p.locals - p.inputs - p.global_input;
    ssa_stats.integer_stores_avoided += p.writes - stores;
  }
  return p.consumed;
}

/* Source records inside multiline runs require fixed native offsets. */
int amd64_integer_run(const struct p_instr_s *code, ptrdiff_t available,
                       int store_lines)
{
  struct pikecode_ssa_integer_plan p;
  struct pikecode_ssa_integer_profile *profile;
  struct label overflow[PIKE_SSA_INTEGER_LIMIT];
  struct label slow[PIKE_SSA_INTEGER_LIMIT], done = { .addr = -1 };
  struct label guard = { .addr = -1 };
  unsigned int i, j;
  unsigned int stores = 0;
  struct pikecode_ssa_integer_stop stack_stop = {0}, stop = {0};
  if (!amd64_integer_regions_enabled()) return 0;
  {
    int consumed = amd64_integer_stack_run(code, available, store_lines,
                     integer_profile_enabled ? &stack_stop : NULL);
    if (consumed) return consumed;
  }
  if (!pikecode_ssa_plan_integer(&p, code, available,
      (INT32)((0x7fffffff - OFFSETOF(svalue, u.integer)) /
              sizeof(struct svalue)), integer_profile_enabled ? &stop : NULL)) {
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
      amd64_flush_code_generator_state();
      amd64_integer_count(profile, PIKE_SSA_INTEGER_REJECTED);
    }
    return 0;
  }
  profile = pikecode_ssa_integer_profile_new(code, available, p.consumed,
                                             p.count, p.locals, "balanced",
                                      integer_profile_enabled ? &stop : NULL);
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
  amd64_integer_count(profile, PIKE_SSA_INTEGER_ENTRIES);
  amd64_load_fp_reg();
  mov_mem_reg(fp_reg, OFFSETOF(pike_frame, locals), PIKE_LOCALS_REG);
  for (i = 1; i <= p.values; i++) {
    struct pikecode_ssa_integer_value *v = &p.value[i];
    ptrdiff_t off = v->local * sizeof(struct svalue);
    if (v->kind != PIKE_SSA_INTEGER_LOAD) continue;
    if (p.normal & (1U << v->bank))
      mov_mem32_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type),
                    P_REG_RAX);
    else
      mov_mem8_reg(PIKE_LOCALS_REG, off + OFFSETOF(svalue, tu.t.type),
                    P_REG_RAX);
    cmp_reg32_imm(P_REG_RAX, PIKE_T_INT);
    jne(profile ? &guard : &slow[0]);
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
  amd64_integer_count(profile, PIKE_SSA_INTEGER_FAST);
  jmp(&done);
  for (i = 0; i < p.units; i++) {
    struct pikecode_ssa_integer_unit *u = &p.unit[i];
    label(&overflow[i]);
    amd64_integer_count(profile, PIKE_SSA_INTEGER_OVERFLOWS);
    if (u->count > 1)
      mov_reg_reg(P_REG_RDX, amd64_integer_reg(&p.value[u->result[0]]));
    amd64_integer_commit(&p, u->dirty_before);
    jmp(&slow[i]);
  }
  if (profile) {
    label(&guard);
    amd64_integer_count(profile, PIKE_SSA_INTEGER_GUARDS);
    jmp(&slow[0]);
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
