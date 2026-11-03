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

struct pikecode_ssa_state {
  unsigned int next_value;
  unsigned int memory[PIKE_SSA_SLOT_COUNT];
  unsigned int reg[PIKE_SSA_SLOT_COUNT];
};

void pikecode_ssa_reset(struct pikecode_ssa_state *state);
void pikecode_ssa_read(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot);
void pikecode_ssa_define(struct pikecode_ssa_state *state,
                         enum pikecode_ssa_slot slot);
int pikecode_ssa_store(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot);
void pikecode_ssa_clobber(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_slot slot);

#endif
