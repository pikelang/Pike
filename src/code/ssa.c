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
  state->block = 0;
  state->last_barrier = PIKE_SSA_MERGE;
  for (slot = 0; slot < PIKE_SSA_SLOT_COUNT; slot++) {
    state->memory[slot] = ++state->next_value;
    state->reg[slot] = 0;
    state->value[slot].kind = PIKE_SSA_NONE;
    state->pending_store[slot].kind = PIKE_SSA_NONE;
  }
  /* The interpreter pointer is passed in a persistent register. */
  state->reg[PIKE_SSA_IP] = state->memory[PIKE_SSA_IP];
}

void pikecode_ssa_read(struct pikecode_ssa_state *state,
                       enum pikecode_ssa_slot slot)
{
#ifdef PIKE_DEBUG
  if (state->pending_store[slot].kind != PIKE_SSA_NONE)
    Pike_fatal("SSA loaded stale interpreter state.\n");
#endif
  state->reg[slot] = state->memory[slot];
  state->value[slot].kind = PIKE_SSA_LOAD;
  state->value[slot].slot = slot;
  state->value[slot].value = state->reg[slot];
  state->value[slot].block = state->block;
}

void pikecode_ssa_merge_reg(struct pikecode_ssa_state *state,
                            enum pikecode_ssa_slot slot)
{
  state->reg[slot] = state->memory[slot];
  state->value[slot].kind = PIKE_SSA_PHI;
  state->value[slot].slot = slot;
  state->value[slot].value = state->reg[slot];
  state->value[slot].block = state->block;
}

void pikecode_ssa_define(struct pikecode_ssa_state *state,
                         enum pikecode_ssa_slot slot)
{
  state->reg[slot] = ++state->next_value;
  state->value[slot].kind = PIKE_SSA_DEFINE;
  state->value[slot].slot = slot;
  state->value[slot].value = state->reg[slot];
  state->value[slot].block = state->block;
  state->pending_store[slot].kind = PIKE_SSA_STORE;
  state->pending_store[slot].slot = slot;
  state->pending_store[slot].value = state->reg[slot];
  state->pending_store[slot].block = state->block;
}

int pikecode_ssa_take_store(struct pikecode_ssa_state *state,
                            enum pikecode_ssa_slot slot,
                            struct pikecode_ssa_op *op)
{
  if (state->pending_store[slot].kind != PIKE_SSA_STORE)
    return 0;
#ifdef PIKE_DEBUG
  if (state->pending_store[slot].block != state->block)
    Pike_fatal("SSA store crossed a control-flow boundary.\n");
  if (state->value[slot].kind != PIKE_SSA_DEFINE ||
      state->value[slot].value != state->pending_store[slot].value)
    Pike_fatal("SSA store does not match the current value.\n");
#endif
  *op = state->pending_store[slot];
  state->pending_store[slot].kind = PIKE_SSA_NONE;
  state->memory[slot] = op->value;
  return 1;
}

void pikecode_ssa_clobber(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_slot slot)
{
#ifdef PIKE_DEBUG
  if (state->pending_store[slot].kind != PIKE_SSA_NONE)
    Pike_fatal("SSA clobbered an unmaterialized value.\n");
#endif
  state->memory[slot] = ++state->next_value;
  state->reg[slot] = 0;
  state->value[slot].kind = PIKE_SSA_CLOBBER;
  state->value[slot].slot = slot;
  state->value[slot].value = state->memory[slot];
  state->value[slot].block = state->block;
  state->pending_store[slot].kind = PIKE_SSA_NONE;
}

void pikecode_ssa_barrier(struct pikecode_ssa_state *state,
                          enum pikecode_ssa_barrier_kind kind)
{
#ifdef PIKE_DEBUG
  int slot;
  for (slot = 0; slot < PIKE_SSA_SLOT_COUNT; slot++)
    if (state->pending_store[slot].kind != PIKE_SSA_NONE)
      Pike_fatal("SSA state not materialized at a barrier.\n");
#endif
  state->last_barrier = kind;
  state->block++;
}
