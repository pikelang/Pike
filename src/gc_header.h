#ifndef GC_HEADER_H
#define GC_HEADER_H

#include "pike_memory.h"
#include "global.h"

struct marker
{
  struct gc_rec_frame *frame;	/* Pointer to the cycle check rec frame. */
  INT32 refs;
  /* Internal references (both weak and nonweak). Increased during
   * check pass. */
  INT32 weak_refs;
  /* Weak (implying internal) references. Increased during check pass.
   * Set to -1 during check pass if it reaches the total number of
   * references. Set to 0 during mark pass if a nonweak reference is
   * found. Decreased during zap weak pass as gc_do_weak_free() is
   * called. */
  unsigned INT32 gc_generation;
#ifdef PIKE_DEBUG
  INT32 xrefs;
  /* Known external references. Increased by gc_mark_external(). */
  INT32 saved_refs;
  /* References at beginning of gc. Set by pretouch and check passes.
   * Decreased by gc_do_weak_free() as weak references are removed. */
  unsigned INT32 flags;
#else
  unsigned INT16 flags;
#endif
};

#ifdef PIKE_DEBUG
#define GC_HEADER_INIT() { NULL, 0, 0, 0, 0, -1, 0 }
#else
#define GC_HEADER_INIT() { NULL, 0, 0, 0, 0 }
#endif

static inline void gc_init_marker(struct marker *m) {

    memset(m, 0, sizeof(struct marker));
#ifdef PIKE_DEBUG
    m->saved_refs = -1;
#endif
}

#endif
