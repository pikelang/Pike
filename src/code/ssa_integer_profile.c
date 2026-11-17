/* -*- mode: C; c-basic-offset: 2; -*- */
/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* Included by ssa_integer.c on backends with integer regions. Native code
 * increments these counters while holding the interpreter lock. Records own
 * their source names and outlive reclaimed programs, since generated code
 * contains their addresses. Profiling is opt-in and adds no default code.
 */
enum pikecode_ssa_integer_counter {
  PIKE_SSA_INTEGER_ENTRIES,
  PIKE_SSA_INTEGER_GUARDS,
  PIKE_SSA_INTEGER_OVERFLOWS,
  PIKE_SSA_INTEGER_FAST,
  PIKE_SSA_INTEGER_REJECTED,
  PIKE_SSA_INTEGER_COUNTERS
};

struct pikecode_ssa_integer_profile {
  volatile unsigned INT64 count[PIKE_SSA_INTEGER_COUNTERS];
  struct pikecode_ssa_integer_profile *next;
  unsigned long id;
  INT32 program;
  INT_TYPE line, last_line;
  unsigned int ops, locals, consumed, stop_offset;
  INT_TYPE stop_line;
  const char *planner, *stop, *reason, *rejection, *stop_opcode;
  char file[1];
};

static struct pikecode_ssa_integer_profile *integer_profiles;
static int integer_profile_enabled = -1;
static unsigned long integer_profile_count;

static void integer_profile_fork_child(struct callback *UNUSED(cb),
                                       void *UNUSED(arg), void *UNUSED(data))
{
  struct pikecode_ssa_integer_profile *p;
  unsigned int i;
  for (p = integer_profiles; p; p = p->next)
    for (i = 0; i < PIKE_SSA_INTEGER_COUNTERS; i++) p->count[i] = 0;
}

static void integer_profile_report(void)
{
  struct pikecode_ssa_integer_profile *p;
  unsigned INT64 total[PIKE_SSA_INTEGER_COUNTERS] = {0};
  unsigned INT64 fast_ops = 0;
  unsigned int i;
  for (p = integer_profiles; p; p = p->next) {
    for (i = 0; i < PIKE_SSA_INTEGER_COUNTERS; i++)
      total[i] += p->count[i];
    fast_ops += p->count[PIKE_SSA_INTEGER_FAST] * p->ops;
  }
  fprintf(stderr, "SSA integer runtime: pid=%ld compiled=%lu "
          "entries=%" PRINTINT64 "u guards=%" PRINTINT64 "u "
          "overflows=%" PRINTINT64 "u fast=%" PRINTINT64 "u "
          "fast_ops=%" PRINTINT64 "u rejected=%" PRINTINT64 "u\n",
          (long)getpid(), integer_profile_count,
          total[PIKE_SSA_INTEGER_ENTRIES], total[PIKE_SSA_INTEGER_GUARDS],
          total[PIKE_SSA_INTEGER_OVERFLOWS], total[PIKE_SSA_INTEGER_FAST],
          fast_ops, total[PIKE_SSA_INTEGER_REJECTED]);
  for (p = integer_profiles; p; p = p->next) {
    unsigned INT64 rejected = p->count[PIKE_SSA_INTEGER_REJECTED];
    if (p->count[PIKE_SSA_INTEGER_ENTRIES] || rejected)
      fprintf(stderr, "SSA integer %s\t%ld\t%lu\t%d\t%s\t"
              "%" PRINTINT64 "d\t%" PRINTINT64 "d\t%s\t%u\t%u\t%s\t"
              "%" PRINTINT64 "u\t%" PRINTINT64 "u\t"
              "%" PRINTINT64 "u\t%" PRINTINT64 "u\t"
              "%s\t%u\t%s\t%" PRINTINT64 "d\t%s\t%u\n",
              rejected ? "rejection" : "region",
              (long)getpid(), p->id, p->program, p->file,
              (INT64)p->line, (INT64)p->last_line,
              p->planner, p->ops, p->locals, p->stop,
              rejected ? rejected : p->count[PIKE_SSA_INTEGER_ENTRIES],
              p->count[PIKE_SSA_INTEGER_GUARDS],
              p->count[PIKE_SSA_INTEGER_OVERFLOWS],
              p->count[PIKE_SSA_INTEGER_FAST], p->reason, p->stop_offset,
              p->stop_opcode, (INT64)p->stop_line, p->rejection, p->consumed);
  }
  /* Keep addresses valid even for embedding-host exit callbacks that run
   * JIT code after this reporter. The OS reclaims these process records. */
}

static void pikecode_ssa_integer_profile_init(void)
{
  if (integer_profile_enabled >= 0) return;
  integer_profile_enabled = getenv("PIKE_SSA_INTEGER_PROFILE") != NULL;
  if (integer_profile_enabled) {
    atexit(integer_profile_report);
    dmalloc_accept_leak(add_to_callback(&fork_child_callback,
                         integer_profile_fork_child, NULL, NULL));
  }
}

static struct pikecode_ssa_integer_profile *
pikecode_ssa_integer_profile_new(const struct p_instr_s *code,
                               ptrdiff_t available, unsigned int consumed,
                               unsigned int ops, unsigned int locals,
                               const char *planner,
                               const struct pikecode_ssa_integer_stop *stop)
{
  struct pikecode_ssa_integer_profile *p;
  struct pike_string *file = code[0].file;
  size_t len = file ? (size_t)file->len : 9;
  size_t i, bytes = 9;
  char *out;
  pikecode_ssa_integer_profile_init();
  if (!integer_profile_enabled) return NULL;
  /* Each source character needs at most ten ASCII bytes (\Uxxxxxxxx). */
  if (len > ((size_t)-1 - sizeof(*p)) / 10)
    Pike_error("Integer profile source name is too long.\n");
  if (file) {
    bytes = 0;
    for (i = 0; i < len; i++) {
      unsigned INT32 c = INDEX_CHARP(file->str, i, file->size_shift);
      bytes += c >= 0x20 && c <= 0x7e && c != '\\' ? 1 : c <= 0xff ? 4 : 10;
    }
  }
#ifdef DEBUG_MALLOC
  p = system_malloc(sizeof(*p) + bytes);
#else
  p = malloc(sizeof(*p) + bytes);
#endif
  if (!p) Pike_error("Out of memory allocating integer profile.\n");
  memset(p, 0, sizeof(*p));
  p->id = ++integer_profile_count;
  p->program = Pike_compiler->new_program->id;
  p->line = code[0].line;
  p->last_line = code[consumed ? consumed - 1 :
                            stop->offset ? stop->offset - 1 : 0].line;
  p->consumed = consumed;
  p->reason = stop->reason;
  p->rejection = stop->rejection;
  p->stop_offset = stop->offset;
  p->stop_opcode = "end";
  p->stop_line = p->last_line;
  if ((ptrdiff_t)stop->offset < available) {
    int op = code[stop->offset].opcode;
    p->stop_line = code[stop->offset].line;
    p->stop_opcode = op >= F_OFFSET && op < F_MAX_INSTR &&
      instrs[op - F_OFFSET].name ? instrs[op - F_OFFSET].name : "unknown";
  }
  p->ops = ops;
  p->locals = locals;
  p->planner = planner;
  p->stop = "end";
  if ((ptrdiff_t)consumed < available) {
    int op = code[consumed].opcode;
    p->stop = op >= F_OFFSET && op < F_MAX_INSTR &&
      instrs[op - F_OFFSET].name ? instrs[op - F_OFFSET].name : "unknown";
  }
  out = p->file;
  if (!file) strcpy(out, "<unknown>");
  else {
    for (i = 0; i < len; i++) {
      unsigned INT32 c = INDEX_CHARP(file->str, i, file->size_shift);
      if (c >= 0x20 && c <= 0x7e && c != '\\') *out++ = c;
      else if (c <= 0xff) out += sprintf(out, "\\x%02x", c);
      else out += sprintf(out, "\\U%08x", c);
    }
    *out = 0;
  }
  p->next = integer_profiles;
  integer_profiles = p;
  return p;
}
