/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef PIKE_CODE_SSA_H
#define PIKE_CODE_SSA_H

/* Virtual interpreter-state values shared by machine-code backends. */
enum pikecode_ssa_slot {
  PIKE_SSA_IP,
  PIKE_SSA_SP,
  PIKE_SSA_FP,
  PIKE_SSA_MARK_SP,
  PIKE_SSA_SLOT_COUNT
};

enum pikecode_ssa_op_kind {
  PIKE_SSA_NONE,
  PIKE_SSA_LOAD,
  PIKE_SSA_PHI,
  PIKE_SSA_DEFINE,
  PIKE_SSA_STORE,
  PIKE_SSA_CLOBBER
};

enum pikecode_ssa_barrier_kind {
  PIKE_SSA_CALL,
  PIKE_SSA_BRANCH,
  PIKE_SSA_MERGE,
  PIKE_SSA_FUNCTION,
  PIKE_SSA_BARRIER_COUNT
};

/* State effects are kept until the backend reaches a sync point. */
struct pikecode_ssa_op {
  enum pikecode_ssa_op_kind kind;
  enum pikecode_ssa_slot slot;
  unsigned int value;
  unsigned int block;
};

struct pikecode_ssa_state {
  unsigned int next_value;
  unsigned int block;
  enum pikecode_ssa_barrier_kind last_barrier;
  unsigned int memory[PIKE_SSA_SLOT_COUNT];
  unsigned int reg[PIKE_SSA_SLOT_COUNT];
  struct pikecode_ssa_op value[PIKE_SSA_SLOT_COUNT];
  struct pikecode_ssa_op pending_store[PIKE_SSA_SLOT_COUNT];
};

void pikecode_ssa_reset(struct pikecode_ssa_state *state);
void pikecode_ssa_read(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot);
void pikecode_ssa_merge_reg(struct pikecode_ssa_state *state,
                            enum pikecode_ssa_slot slot);
void pikecode_ssa_define(struct pikecode_ssa_state *state,
                         enum pikecode_ssa_slot slot);
int pikecode_ssa_take_store(struct pikecode_ssa_state *state,
                            enum pikecode_ssa_slot slot,
                            struct pikecode_ssa_op *op);
void pikecode_ssa_clobber(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_slot slot);
void pikecode_ssa_barrier(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_barrier_kind kind);

#endif
