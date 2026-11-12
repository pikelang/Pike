/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Included by amd64.c. Keep a bounded region unpublished until its locals
 * uses and internal branch layout are known. Ordinary instructions remain
 * opaque byte spans; repairs, branches, and PC-relative LEAs have IR nodes.
 * No external jumps, line changes, or local debug directives enter a region.
 */
#define AMD64_SSA_REGION_NODES 256
#define AMD64_SSA_REGION_VALUES 512

enum amd64_region_kind {
  AMD64_REGION_REPAIR,
  AMD64_REGION_BASE,
  AMD64_REGION_BRANCH,
  AMD64_REGION_PC
};

struct amd64_region_node {
  enum amd64_region_kind kind;
  ptrdiff_t start, end, target;
  int size, op, live;
  int load_size;
  enum amd64_reg reg;
};

/* A use makes every materialization feeding its phi graph live. Zero is
 * an input needing no deferred materialization. No graph spans a region. */
struct amd64_region_value {
  unsigned int left, right;
  int node, used;
};

static struct {
  int active, abandoned;
  ptrdiff_t start;
  unsigned int count, values, value;
  struct amd64_region_node node[AMD64_SSA_REGION_NODES];
  struct amd64_region_value val[AMD64_SSA_REGION_VALUES];
} ssa_region;
static int ssa_no_regions;

static struct amd64_region_node *amd64_region_node(
  enum amd64_region_kind kind, ptrdiff_t start, ptrdiff_t end)
{
  struct amd64_region_node *node;
  if (!ssa_region.active || ssa_region.abandoned) return NULL;
  if (ssa_region.count == AMD64_SSA_REGION_NODES) {
    ssa_region.abandoned = 1;
    return NULL;
  }
  node = &ssa_region.node[ssa_region.count++];
  node->kind = kind;
  node->start = start;
  node->end = end;
  node->size = end - start;
  node->live = 0;
  return node;
}

static unsigned int amd64_region_value(unsigned int left,
                                      unsigned int right, int node)
{
  struct amd64_region_value *value;
  if (!ssa_region.active || ssa_region.abandoned) return 0;
  if (node < 0 && left == right) return left;
  if (ssa_region.values + 1 == AMD64_SSA_REGION_VALUES) {
    ssa_region.abandoned = 1;
    return 0;
  }
  value = &ssa_region.val[++ssa_region.values];
  value->left = left;
  value->right = right;
  value->node = node;
  value->used = 0;
  return ssa_region.values;
}

static void amd64_region_use(unsigned int id)
{
  struct amd64_region_value *value;
  if (!id || ssa_region.abandoned) return;
  value = &ssa_region.val[id];
  if (value->used) return;
  value->used = 1;
  if (value->node >= 0) {
    ssa_region.node[value->node].live = 1;
    return;
  }
  amd64_region_use(value->left);
  amd64_region_use(value->right);
}

static void amd64_region_base(ptrdiff_t start, ptrdiff_t load_end,
                              enum amd64_reg reg)
{
  struct amd64_region_node *node =
    amd64_region_node(AMD64_REGION_BASE, start, PIKE_PC);
  if (node) {
    node->load_size = load_end - start;
    node->reg = reg;
    ssa_region.value = amd64_region_value(0, 0, ssa_region.count - 1);
  }
}

static void amd64_region_repair(ptrdiff_t start)
{
  struct amd64_region_node *node =
    amd64_region_node(AMD64_REGION_REPAIR, start, PIKE_PC);
  if (node)
    ssa_region.value = amd64_region_value(0, 0, ssa_region.count - 1);
}

/* Regions contain only audited opcode implementations with local labels,
 * fixed-address C calls, and PC-relative LEAs. In particular, exclude raw
 * jump tables, catch setup, returns, and relative calls to shared helpers.
 */
int amd64_ssa_regions_enabled(void)
{
  return !ssa_no_locals_cache && !ssa_no_regions;
}

int amd64_ssa_region_opcode(unsigned int op)
{
  if (ssa_no_locals_cache || ssa_no_regions) return 0;
  switch (op) {
  case F_LOCAL:
  case F_2_LOCALS:
  case F_INC_LOCAL:
  case F_INC_LOCAL_AND_POP:
  case F_POST_INC_LOCAL:
  case F_DEC_LOCAL:
  case F_DEC_LOCAL_AND_POP:
  case F_POST_DEC_LOCAL:
  case F_ADD_LOCAL_INT:
  case F_ADD_LOCAL_INT_AND_POP:
  case F_ADD_LOCALS_AND_POP:
  case F_ASSIGN_LOCAL:
  case F_ASSIGN_LOCAL_AND_POP:
  case F_ASSIGN_LOCAL_NUMBER_AND_POP:
  case F_LOCAL_2_LOCAL:
  case F_NUMBER:
  case F_NEG_NUMBER:
  case F_CONST0:
  case F_CONST1:
  case F_CONST_1:
  case F_POP_VALUE:
    return 1;
  }
  return 0;
}

void amd64_ssa_region_begin(void)
{
  ssa_region.active = 1;
  ssa_region.abandoned = 0;
  ssa_region.start = PIKE_PC;
  ssa_region.count = ssa_region.values = ssa_region.value = 0;
}

/* Translate an old boundary using the current node sizes. Targets cannot
 * lie inside an instruction or a removable repair. */
static ptrdiff_t amd64_region_offset(ptrdiff_t old)
{
  unsigned int i;
  ptrdiff_t result = old;
  for (i = 0; i < ssa_region.count; i++) {
    struct amd64_region_node *node = &ssa_region.node[i];
    if (node->end > old) break;
    result -= node->end - node->start - node->size;
  }
  return result;
}

void amd64_ssa_region_end(void)
{
  unsigned int i;
  int changed;
  ptrdiff_t end = PIKE_PC, src, dst;
  PIKE_OPCODE_T *code = Pike_compiler->new_program->program;
  if (!ssa_region.active) return;
  ssa_region.active = 0;
  /* A following region starts with a fresh input. Some outgoing paths may
   * lose their final repair when the base has no use in this region. */
  ssa_locals_valid = 0;
  pikecode_ssa_clobber(&ssa_state, PIKE_SSA_LOCALS);
  if (ssa_region.abandoned) {
    if (ssa_report_stats) ssa_stats.regions_abandoned++;
    return;
  }
  /* Resolve every target before moving any bytes. */
  src = ssa_region.start;
  for (i = 0; i < ssa_region.count; i++) {
    struct amd64_region_node *node = &ssa_region.node[i];
    if (node->start < src || node->end < node->start || node->end > end ||
        (node->kind == AMD64_REGION_PC && node->target > end)) {
      if (ssa_report_stats) ssa_stats.regions_abandoned++;
      return;
    }
    src = node->end;
    if (node->kind == AMD64_REGION_BRANCH) {
      node->target = node->end +
        (node->size == 2 ? (signed char)code[node->end - 1] :
         read_pointer(node->end - 4));
      if (node->target < ssa_region.start || node->target > end) {
        if (ssa_report_stats) ssa_stats.regions_abandoned++;
        return;
      }
    }
  }
  for (i = 0; i < ssa_region.count; i++) {
    struct amd64_region_node *node = &ssa_region.node[i];
    if (node->kind == AMD64_REGION_BASE && !node->live) {
      /* The first use needed only an address. With no later cache use,
       * load the destination directly and omit the copy from R11. */
      node->size = node->load_size;
      if (ssa_report_stats) ssa_stats.locals_copies_removed++;
    }
    if (node->kind == AMD64_REGION_REPAIR && !node->live) {
      node->size = 0;
      if (ssa_report_stats) {
        ssa_stats.locals_repairs_removed++;
        ssa_stats.locals_repairs--;
        ssa_stats.loads[PIKE_SSA_LOCALS]--;
      }
    }
  }
  /* Shrinking is monotonic. Repeat since one short branch can bring another
   * into range. A forward target also moves when its own branch shrinks. */
  do {
    changed = 0;
    for (i = 0; i < ssa_region.count; i++) {
      struct amd64_region_node *node = &ssa_region.node[i];
      ptrdiff_t target, dist;
      if (node->kind != AMD64_REGION_BRANCH || node->size == 2) continue;
      target = amd64_region_offset(node->target);
      if (node->target >= node->end) target -= node->size - 2;
      dist = target - (amd64_region_offset(node->start) + 2);
      if (dist >= -128 && dist <= 127) {
        node->size = 2;
        changed = 1;
      }
    }
  } while (changed);

  src = dst = ssa_region.start;
  for (i = 0; i < ssa_region.count; i++) {
    struct amd64_region_node *node = &ssa_region.node[i];
    memmove(code + dst, code + src, node->start - src);
    dst += node->start - src;
    if (node->kind == AMD64_REGION_BRANCH) {
      ptrdiff_t dist = amd64_region_offset(node->target) - (dst + node->size);
      if (node->size == 2) {
        code[dst] = node->op;
        code[dst + 1] = dist;
        if (ssa_report_stats && node->end - node->start > 2)
          ssa_stats.branches_shortened++;
      } else {
        if (node->op == 0xeb) code[dst] = 0xe9;
        else {
          code[dst] = 0x0f;
          code[dst + 1] = node->op + 0x10;
        }
        upd_pointer(dst + node->size - 4, dist);
      }
    } else if (node->size) {
      memmove(code + dst, code + node->start, node->size);
      if (node->kind == AMD64_REGION_BASE && !node->live) {
        /* mov r64,[R14+disp]: retarget REX.R and ModRM.reg. REX is
         * always present since the frame pointer is in R14. */
        code[dst] = (code[dst] & ~4) | ((node->reg & 8) ? 4 : 0);
        code[dst + 2] = (code[dst + 2] & ~0x38) |
          ((node->reg & 7) << 3);
      }
      if (node->kind == AMD64_REGION_PC)
        upd_pointer(dst + 3, amd64_region_offset(node->target) - (dst + 7));
    }
    dst += node->size;
    src = node->end;
  }
  memmove(code + dst, code + src, end - src);
  dst += end - src;
  if (amd64_prev_stored_pc >= ssa_region.start)
    amd64_prev_stored_pc = amd64_region_offset(amd64_prev_stored_pc);
  Pike_compiler->new_program->num_program = dst;
  if (ssa_report_stats) {
    ssa_stats.regions++;
    ssa_stats.region_bytes_removed += end - dst;
  }
}
