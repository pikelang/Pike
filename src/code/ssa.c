/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * A small machine-state SSA model. Backends assign a new value when a
 * register changes, and only materialize it in interpreter memory at a
 * call, branch, or merge boundary. C opcode calls can clobber slots.
 */

void pikecode_ssa_reset(struct pikecode_ssa_state *state)
{
  int slot;
  state->next_value = 0;
  for (slot = 0; slot < PIKE_SSA_SLOT_COUNT; slot++) {
    state->memory[slot] = ++state->next_value;
    state->reg[slot] = 0;
  }
  /* The interpreter pointer is passed in a persistent register. */
  state->reg[PIKE_SSA_IP] = state->memory[PIKE_SSA_IP];
}

void pikecode_ssa_read(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot)
{
  state->reg[slot] = state->memory[slot];
}

void pikecode_ssa_define(struct pikecode_ssa_state *state,
                         enum pikecode_ssa_slot slot)
{
  state->reg[slot] = ++state->next_value;
}

int pikecode_ssa_store(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot)
{
  if (!state->reg[slot] || state->reg[slot] == state->memory[slot])
    return 0;
  state->memory[slot] = state->reg[slot];
  return 1;
}

void pikecode_ssa_clobber(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_slot slot)
{
  state->memory[slot] = ++state->next_value;
  state->reg[slot] = 0;
}
