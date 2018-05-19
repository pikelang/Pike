#ifndef GC_HEADER_H
#define GC_HEADER_H

#include "pike_memory.h"
#include "global.h"

/* Note: Keep in sync to struct marker below */
#ifdef PIKE_DEBUG
# define DEBUG_GC_MARKER_MEMBERS      \
        INT32 xrefs;                  \
        INT32 saved_refs;             \
        unsigned INT32 gc_flags
#else
# define DEBUG_GC_MARKER_MEMBERS      \
        unsigned INT16 gc_flags
#endif

#define GC_MARKER_MEMBERS             \
        INT32 refs;                   \
        INT32 gc_refs;                \
        struct gc_rec_frame *frame;   \
        INT32 weak_refs;              \
        unsigned INT16 gc_generation; \
        DEBUG_GC_MARKER_MEMBERS

/* Note: Keep in sync to GC_MARKER_MEMBERS above */
struct marker
{
  /* this should not be modified through a struct marker pointer */
  INT32 refs_do_not_touch;
  INT32 gc_refs;
  struct gc_rec_frame *frame;	/* Pointer to the cycle check rec frame. */
  /* Internal references (both weak and nonweak). Increased during
   * check pass. */
  INT32 weak_refs;
  /* Weak (implying internal) references. Increased during check pass.
   * Set to -1 during check pass if it reaches the total number of
   * references. Set to 0 during mark pass if a nonweak reference is
   * found. Decreased during zap weak pass as gc_do_weak_free() is
   * called. */
  unsigned INT16 gc_generation;
#ifdef PIKE_DEBUG
  INT32 xrefs;
  /* Known external references. Increased by gc_mark_external(). */
  INT32 saved_refs;
  /* References at beginning of gc. Set by pretouch and check passes.
   * Decreased by gc_do_weak_free() as weak references are removed. */
  unsigned INT32 gc_flags;
#else
  unsigned INT16 gc_flags;
#endif
};

#ifdef PIKE_DEBUG
#define GC_HEADER_INIT(REFS) (REFS), 0, NULL, 0, 0, 0, -1, 0
#else
#define GC_HEADER_INIT(REFS) (REFS), 0, NULL, 0, 0, 0
#endif

static inline void gc_init_marker(void *ptr) {
    struct marker *m = ptr;
    m->gc_refs = 0;
    m->frame = NULL;
    m->weak_refs = 0;
    m->gc_generation = 0;
#ifdef PIKE_DEBUG
    m->xrefs = 0;
    m->saved_refs = -1;
#endif
    m->gc_flags = 0;
}

#endif
