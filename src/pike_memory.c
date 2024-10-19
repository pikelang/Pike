/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "pike_macros.h"
#include "pike_threadlib.h"
#include "gc.h"
#include "fd_control.h"
#include "block_allocator.h"
#include "pike_cpulib.h"
#include "siphash24.h"
#include "cyclic.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <errno.h>

int page_size;

long pcharp_strlen(const PCHARP a)
{
  long len;
  for(len=0;INDEX_PCHARP(a,len);len++);
  return len;
}

/* NOTE: Second arg is a p_char2 to avoid warnings on some compilers. */
p_wchar1 *MEMCHR1(p_wchar1 *p, p_wchar2 c, ptrdiff_t e)
{
  while(--e >= 0) if(*(p++) == (p_wchar1)c) return p-1;
  return (p_wchar1 *)NULL;
}

p_wchar2 *MEMCHR2(p_wchar2 *p, p_wchar2 c, ptrdiff_t e)
{
  while(--e >= 0) if(*(p++) == (p_wchar2)c) return p-1;
  return (p_wchar2 *)NULL;
}

/*
 * This function may NOT change 'order'
 * This function is hopefully fast enough...
 */
void reorder(char *memory, INT32 nitems, INT32 size, const INT32 *order)
{
  INT32 e, aok;
  char *tmp;
  if(UNLIKELY(nitems<2)) return;
  aok = 0;
  /*
   * Prime the cache for the order array, and check the array for
   * correct ordering.  At the first order mismatch, bail out and
   * start the actual reordering beyond the ordered prefix.
   * If the order turns out to be correct already, perform an early return.
   */
  do
    if (UNLIKELY(order[aok] != aok))
      goto unordered;
  while (LIKELY(++aok < nitems));
  return;
unordered:
  order += aok;
  nitems -= aok;
  tmp=xalloc(size * nitems);
  e = 0;

#undef DOSIZE
#undef CHECK_ALIGNED

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
#define CHECK_ALIGNED(X)	0
#else
#define CHECK_ALIGNED(X)			\
 if ((ptrdiff_t)memory & ((X)-1)) goto unaligned
#endif

#define DOSIZE(X,Y)				\
 case (X):					\
 CHECK_ALIGNED(X);				\
 {						\
  Y *from=(Y *) memory;				\
  Y *to=(Y *) tmp;				\
  do						\
     to[e] = from[order[e]];			\
  while (LIKELY(++e < nitems));			\
  break;					\
 }

  switch(size)
 {
   DOSIZE(1,B1_T)
#ifdef B2_T
     DOSIZE(2,B2_T)
#endif
#ifdef B4_T
     DOSIZE(4,B4_T)
#endif
#ifdef B8_T
     DOSIZE(8,B8_T)
#endif
#undef CHECK_ALIGNED
     // Force aligned check for 128 bit values
     // GCC-7.2 messes up otherwise
#define CHECK_ALIGNED(X)			\
 if ((ptrdiff_t)memory & ((X)-1)) goto unaligned
#ifdef B16_T
     DOSIZE(16,B16_T)
#endif

  default:
unaligned:
    do
      memcpy(tmp+e*size, memory+order[e]*size, size);
    while (LIKELY(++e < nitems));
  }

  memcpy(memory + aok * size, tmp, size * nitems);
  free(tmp);
}

#if (defined(__i386__) || defined(__amd64__)) && defined(__GNUC__)
/*
 * This would look much better using the compiler intrinsics, or even the
 * assembler instructions directly.
 *
 * However, that requires a (at least in 2011) somewhat modern gcc (4.5.*).
 *
 * For reference:
 *  #define CRC32SI(H,P) H=__builtin_ia32_crc32si(H,P)
 *  #define CRC32SQ(H,P) H=__builtin_ia32_crc32qi(H,P)
 *  #define __cpuid(level,a,b,c,d) __get_cpuid(level, &a,&b,&c,&d)
 *
 * The value for the SSE4_2 is also available in cpuid.h in modern
 * gcc:s.
 */
#ifdef HAVE_CRC32_INTRINSICS
#define CRC32SI(H,P) H=__builtin_ia32_crc32si(H,*(P))
#define CRC32SQ(H,P) H=__builtin_ia32_crc32qi(H,*(P))
#else

/* GCC versions without __builtin_ia32_crc32* also lacks the support
 * in the assembler, given that the binutils are the same version.
 *
 * Adding a second test for that seems like overkill.
 */

#define CRC32SI(H,P)                                                  \
    __asm__ __volatile__(                                             \
        ".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"                          \
        :"=S"(H) :"0"(H), "c"(*(P)))

#define CRC32SQ(H,P)                                                  \
    __asm__ __volatile__(                                             \
        ".byte 0xf2, 0xf, 0x38, 0xf0, 0xf1"                           \
        :"=S"(H) :"0"(H), "c"(*(P)))
#endif

ATTRIBUTE((const)) static inline int supports_sse42( )
{
  INT32 cpuid[4];
  x86_get_cpuid (1, cpuid);
  return cpuid[3] & bit_SSE4_2;
}

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
#ifdef HAVE_CRC32_INTRINSICS
/*
The intrinsics are only available if -msse4 is specified.
However, specifying that option on the command-line makes the whole runtime-test here
pointless, since gcc will use other sse4 instructions when suitable.
*/
ATTRIBUTE((target("sse4")))
#endif
PIKE_HOT_ATTRIBUTE
static inline size_t low_hashmem_ia32_crc32( const void *s, size_t len,
					     size_t nbytes, UINT64 key )
{
  unsigned int h = len;
  const unsigned char *c = s;
  const unsigned int *p;
  size_t trailer_bytes = 8;

  if( key )
      return low_hashmem_siphash24(s,len,nbytes,key);

  if (UNLIKELY(!len)) return h;

  if( nbytes >= len )
  {
    /* Hash the whole memory area */
    nbytes = len;
    trailer_bytes = 0;
  } else if (UNLIKELY(nbytes < 32)) {
    /* Hash the whole memory area once */
    trailer_bytes = 0;
  }

  /* Hash the initial unaligned bytes (if any). */
  while (UNLIKELY(((size_t)c) & 0x03) && nbytes) {
    CRC32SQ(h, c++);
    nbytes--;
  }

  /* c is now aligned. */

  p = (const unsigned int *)c;

  /* .. all full integers in blocks of 8 .. */
  while (nbytes & ~31) {
    CRC32SI(h, &p[0]);
    CRC32SI(h, &p[1]);
    CRC32SI(h, &p[2]);
    CRC32SI(h, &p[3]);
    CRC32SI(h, &p[4]);
    CRC32SI(h, &p[5]);
    CRC32SI(h, &p[6]);
    CRC32SI(h, &p[7]);
    p += 8;
    nbytes -= 32;
  }

  /* .. all remaining full integers .. */
  while (nbytes & ~3) {
    CRC32SI(h, &p[0]);
    p++;
    nbytes -= 4;
  }

  /* any remaining bytes. */
  c = (const unsigned char *)p;
  while (nbytes--) {
    CRC32SQ( h, c++ );
  }

  if (trailer_bytes) {
    /* include 8 bytes from the end. Note that this might be a
     * duplicate of the previous bytes.
     *
     * Also note that this means we are rather likely to read
     * unaligned memory.  That is OK, however.
     */
    p = (const unsigned int *)((const unsigned char *)s+len-8);
    CRC32SI(h, p++);
    CRC32SI(h, p);
  }

#if SIZEOF_CHAR_P > 4
    /* FIXME: We could use the long long crc32 instructions that work on 64 bit values.
     * however, those are only available when compiling to amd64.
     */
  return (((size_t)h)<<32) | h;
#else
  return h;
#endif
}

#ifdef __i386__
ATTRIBUTE((fastcall))
#endif
  size_t (*low_hashmem)(const void *, size_t, size_t, UINT64);

static void init_hashmem()
{
  if( supports_sse42() )
    low_hashmem = low_hashmem_ia32_crc32;
  else
    low_hashmem = low_hashmem_siphash24;
}
#else
static void init_hashmem(){}

PIKE_HOT_ATTRIBUTE
  size_t low_hashmem(const void *a, size_t len_, size_t mlen_, UINT64 key_)
{
    return low_hashmem_siphash24(a, len_, mlen_, key_);
}
#endif

PIKE_HOT_ATTRIBUTE
  size_t hashmem(const void *a, size_t len_, size_t mlen_)
{
  return low_hashmem(a, len_, mlen_, 0);
}

PMOD_EXPORT void *debug_xalloc(size_t size)
{
  void *ret;
  if(!size)
     Pike_error("Allocating zero bytes.\n");

  ret=(void *)malloc(size);
  if(ret) return ret;

  Pike_error(msg_out_of_mem_2, size);
  return 0;
}

PMOD_EXPORT void debug_xfree(void *mem)
{
  free(mem);
}

PMOD_EXPORT void *debug_xmalloc(size_t s)
{
  return malloc(s);
}

PMOD_EXPORT void *debug_xrealloc(void *m, size_t s)
{
  void *ret = realloc(m,s);
  if (ret) return ret;
  Pike_error(msg_out_of_mem_2, s);
  return NULL;
}

PMOD_EXPORT void *debug_xcalloc(size_t n, size_t s)
{
  void *ret;
  if(!n || !s)
     Pike_error("Allocating zero bytes.\n");

  ret=(void *)calloc(n, s);
  if(ret) return ret;

  Pike_error(msg_out_of_mem_2, n*s);
  return 0;
}

PMOD_EXPORT char *debug_xstrdup(const char *src)
{
  char *dst = NULL;
  if (src) {
    int len = strlen (src) + 1;
    dst = xalloc (len);
    memcpy (dst, src, len);
  }
  return dst;
}

/*
 * mexec_*()
 *
 * Allocation of executable memory.
 */

#ifdef MEXEC_USES_MMAP

#define MEXEC_ALLOC_CHUNK_SIZE page_size

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS	MAP_ANON
#endif /* !MAP_ANONYMOUS && MAP_ANON */

#ifdef MAP_ANONYMOUS
/* Note: mmap ANON fails (EINVAL) on Solaris if the fd isn't -1. */
#define dev_zero -1
#else
static int dev_zero = -1;
#define INIT_DEV_ZERO
#define MAP_ANONYMOUS	0
#endif

#ifndef MAP_JIT
#define MAP_JIT 0
#endif

static inline void *mexec_do_alloc (void *start, size_t length)
{
  void *blk = mmap(start, length, PROT_EXEC|PROT_READ|PROT_WRITE,
		   MAP_PRIVATE|MAP_ANONYMOUS|MAP_JIT, dev_zero, 0);
  if (blk == MAP_FAILED) {
    fprintf(stderr, "mmap of %"PRINTSIZET"u bytes failed, errno=%d. "
	    "(dev_zero=%d)\n", length, errno, dev_zero);
    return NULL;
  }
  return blk;
}

#elif defined (_WIN32)

/* VirtualAlloc practically never succeeds to give us a new block
 * adjacent to an earlier allocated block if we use the page size.
 * Some testing shows that VirtualAlloc often pads alloc requests to
 * 64kb pages (at least on Windows Server 2003), so we use that as the
 * basic allocation unit instead of the page size. */
#define MEXEC_ALLOC_CHUNK_SIZE (64 * 1024)

static inline void *mexec_do_alloc (void *start, size_t length)
{
  void *blk = VirtualAlloc (start, length, MEM_RESERVE|MEM_COMMIT,
			    PAGE_EXECUTE_READWRITE);
  if (!blk) {
    blk = VirtualAlloc (NULL, length, MEM_RESERVE|MEM_COMMIT,
			PAGE_EXECUTE_READWRITE);
    if (!blk) {
      fprintf (stderr, "VirtualAlloc of %"PRINTSIZET"u bytes failed. "
	       "Error code: %d\n", length, GetLastError());
      return NULL;
    }
  }
  return blk;
}

#endif	/* _WIN32 */

#ifdef USE_MY_MEXEC_ALLOC

/* #define MY_MEXEC_ALLOC_STATS */

#ifdef MY_MEXEC_ALLOC_STATS
static size_t sep_allocs = 0, grow_allocs = 0, total_size = 0;
#endif

struct mexec_block {
  struct mexec_hdr *hdr;
  ptrdiff_t size;
#ifdef MEXEC_MAGIC
  unsigned long long magic;
#endif /* MEXEC_MAGIC */
};
struct mexec_free_block {
  struct mexec_free_block *next;
  ptrdiff_t size;
};
static struct mexec_hdr {
  struct mexec_hdr *next;
  ptrdiff_t size;
  char *bottom;
  struct mexec_free_block *free;/* Ordered according to reverse address. */
} *mexec_hdrs = NULL;		/* Ordered according to reverse address. */

#ifdef PIKE_DEBUG
static void low_verify_mexec_hdr(struct mexec_hdr *hdr,
				 const char *file, INT_TYPE line)
{
  struct mexec_free_block *ptr;
  char *blk_ptr;
  if (!hdr) return;
  if (d_flag) {
    if (hdr->bottom > ((char *)hdr) + hdr->size) {
      Pike_fatal("%s:%ld:Bad bottom %p > %p\n",
		 file, (long)line,
		 hdr->bottom, ((char *)hdr) + hdr->size);
    }
    for (blk_ptr = (char *)(hdr+1); blk_ptr < hdr->bottom;) {
      struct mexec_free_block *blk = (struct mexec_free_block *)blk_ptr;
      if (blk->size <= 0) {
	Pike_fatal("%s:%ld:Bad block size: %p\n",
		   file, (long)line,
		   (void *)blk->size);
      }
      blk_ptr += blk->size;
    }
    if (blk_ptr != hdr->bottom) {
      Pike_fatal("%s:%ld:Block reaches past bottom! %p > %p\n",
		 file, (long)line,
		 blk_ptr, hdr->bottom);
    }
    if (d_flag > 1) {
      for (ptr = hdr->free; ptr; ptr = ptr->next) {
	if (ptr < (struct mexec_free_block *)(hdr+1)) {
	  Pike_fatal("%s:%ld:Free block before start of header. %p < %p\n",
		     file, (long)line,
		     ptr, hdr+1);
	}
	if (((char *)ptr) >= hdr->bottom) {
	  Pike_fatal("%s:%ld:Free block past bottom. %p >= %p\n",
		     file, (long)line,
		     ptr, hdr->bottom);
	}
      }
    }
  }
}
#define verify_mexec_hdr(HDR)	low_verify_mexec_hdr(HDR, __FILE__, __LINE__)
#else /* !PIKE_DEBUG */
#define verify_mexec_hdr(HDR)
#endif /* PIKE_DEBUG */

/* FIXME: Consider applying the following hints on Solaris:
 *
 * For 32-bit processes:
 *   * Combine 8-Kbyte requests up to a limit of 48 Kbytes.
 *   * Combine amounts over 48 Kbytes into 496-Kbyte chunks.
 *   * Combine amounts over 496 Kbytes into 4080-Kbyte chunks.
 * For 64-bit processes:
 *   * Combine amounts < 1008 Kbytes into chunks <= 1008 Kbytes.
 *   * Combine amounts over 1008 Kbytes into 4080-Kbyte chunks.
 */

static struct mexec_hdr *grow_mexec_hdr(struct mexec_hdr *base, size_t sz)
{
  struct mexec_hdr *wanted = NULL;
  struct mexec_hdr *hdr;
  /* fprintf(stderr, "grow_mexec_hdr(%p, %p)\n", base, (void *)sz); */
  if (!sz) return NULL;

  sz = (sz + sizeof(struct mexec_hdr) + (MEXEC_ALLOC_CHUNK_SIZE-1)) &
    ~(MEXEC_ALLOC_CHUNK_SIZE-1);

  if (base) {
    verify_mexec_hdr(base);
    wanted = (struct mexec_hdr *)(((char *)base) + base->size);
  }

  hdr = mexec_do_alloc (wanted, sz);
  if (!hdr) return NULL;
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(0);
#endif
  if (hdr == wanted) {
    /* We succeeded in growing. */
#ifdef MY_MEXEC_ALLOC_STATS
    grow_allocs++;
    total_size += sz;
#endif
    base->size += sz;
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
    pthread_jit_write_protect_np(1);
#endif
    verify_mexec_hdr(base);
    return base;
  }
  /* Find insertion slot in hdr list. */
  if ((wanted = mexec_hdrs)) {
    while ((size_t)wanted->next > (size_t)hdr) {
      wanted = wanted->next;
    }
    if (wanted->next) {
      if ((((char *)wanted->next)+wanted->next->size) == (char *)hdr) {
	/* We succeeded in growing some other hdr. */
#ifdef MY_MEXEC_ALLOC_STATS
	grow_allocs++;
	total_size += sz;
#endif
	wanted->next->size += sz;
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
        pthread_jit_write_protect_np(1);
#endif
	verify_mexec_hdr(wanted->next);
	return wanted->next;
      }
    }
    hdr->next = wanted->next;
    wanted->next = hdr;
  } else {
    hdr->next = NULL;
    mexec_hdrs = hdr;
  }
#ifdef MY_MEXEC_ALLOC_STATS
  sep_allocs++;
  total_size += sz;
#endif
  hdr->size = sz;
  hdr->free = NULL;
  hdr->bottom = (char *)(hdr+1);
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(1);
#endif
  verify_mexec_hdr(hdr);
  return hdr;
}

/* Assumes sz has sufficient alignment and has space for a mexec_block. */
static struct mexec_block *low_mexec_alloc(struct mexec_hdr *hdr, size_t sz)
{
  struct mexec_free_block **free;
  struct mexec_block *res;

  /* fprintf(stderr, "low_mexec_alloc(%p, %p)\n", hdr, (void *)sz); */
  if (!hdr) return NULL;
  verify_mexec_hdr(hdr);
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(0);
#endif
  free = &hdr->free;
  while (*free && ((*free)->size < (ptrdiff_t)sz)) {
    free = &((*free)->next);
  }
  if ((res = (struct mexec_block *)*free)) {
    if (res->size <= (ptrdiff_t)(sz + 2*sizeof(struct mexec_block))) {
      /* No space for a split. */
      sz = (*free)->size;
      *free = ((struct mexec_free_block *)res)->next;
    } else {
      /* Split the block. */
      struct mexec_free_block *next =
	(struct mexec_free_block *)(((char *)res) + sz);
      next->next = (*free)->next;
      next->size = (*free)->size - sz;
      *free = next;
    }
  } else if ((hdr->bottom - ((char *)hdr)) <= (hdr->size - (ptrdiff_t)sz)) {
    res = (struct mexec_block *)hdr->bottom;
    if (hdr->bottom + 2*sizeof(struct mexec_block) - ((char *)hdr) >=
	hdr->size) {
      /* No space for a split. */
      sz = hdr->size - (hdr->bottom - ((char *)hdr));
    }
    hdr->bottom += sz;
  } else {
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
    pthread_jit_write_protect_np(1);
#endif
    return NULL;
  }
  res->size = sz;
  res->hdr = hdr;
#ifdef MEXEC_MAGIC
  res->magic = MEXEC_MAGIC;
#endif /* MEXEC_MAGIC */
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(1);
#endif
  verify_mexec_hdr(hdr);
  return res;
}

PMOD_EXPORT void mexec_free(void *ptr)
{
  struct mexec_free_block *prev_prev = NULL;
  struct mexec_free_block *prev = NULL;
  struct mexec_free_block *next = NULL;
  struct mexec_free_block *blk;
  struct mexec_block *mblk;
  struct mexec_hdr *hdr;

  /* fprintf(stderr, "mexec_free(%p)\n", ptr); */

  if (!ptr) return;
  mblk = ((struct mexec_block *)ptr)-1;
  hdr = mblk->hdr;
#ifdef MEXEC_MAGIC
  if (mblk->magic != MEXEC_MAGIC) {
    Pike_fatal("mexec_free() called with non mexec pointer.\n"
	       "ptr: %p, magic: 0x%016llx, hdr: %p\n",
	       ptr, mblk->magic, hdr);
  }
#endif /* MEXEC_MAGIC */
  verify_mexec_hdr(hdr);

#ifdef VALGRIND_DISCARD_TRANSLATIONS
  VALGRIND_DISCARD_TRANSLATIONS (mblk, mblk->size);
#endif

#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(0);
#endif
  blk = (struct mexec_free_block *)mblk;

  next = hdr->free;
  while ((size_t)next > (size_t)blk) {
    prev_prev = prev;
    prev = next;
    next = next->next;
  }

  if (next && ((((char *)next) + next->size) == (char *)blk)) {
    /* Join with successor. */
    next->size += blk->size;
    blk = next;
  } else {
    blk->next = next;
  }

  if (prev) {
    if ((((char *)blk) + blk->size) == (char *)prev) {
      /* Join with prev. */
      blk->size += prev->size;
      if (prev_prev) {
	prev_prev->next = blk;
      } else {
	hdr->free = blk;
      }
    } else {
      prev->next = blk;
    }
  } else {
    if ((((char *)blk) + blk->size) == hdr->bottom) {
      /* Join with bottom. */
      hdr->bottom = (char *)blk;
      hdr->free = blk->next;
    } else {
      hdr->free = blk;
    }
  }
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(1);
#endif
  verify_mexec_hdr(hdr);
}

void *mexec_alloc(size_t sz)
{
  struct mexec_hdr *hdr;
  struct mexec_block *res;

  /* fprintf(stderr, "mexec_alloc(%p)\n", (void *)sz); */

  if (!sz) {
    /* fprintf(stderr, " ==> NULL (no size)\n"); */
    return NULL;
  }
  /* 32 byte alignment. */
  sz = (sz + sizeof(struct mexec_block) + 0x1f) & ~0x1f;
  hdr = mexec_hdrs;
  while (hdr && !(res = low_mexec_alloc(hdr, sz))) {
    hdr = hdr->next;
  }
  if (!hdr) {
    hdr = grow_mexec_hdr(NULL, sz);
    if (!hdr) {
      /* fprintf(stderr, " ==> NULL (grow failed)\n"); */
      return NULL;
    }
    verify_mexec_hdr(hdr);
    res = low_mexec_alloc(hdr, sz);
#ifdef PIKE_DEBUG
    if (!res) {
      Pike_fatal("mexec_alloc(%"PRINTSIZET"u) failed to allocate from grown hdr!\n",
		 sz);
    }
#endif /* PIKE_DEBUG */
  }
  /* fprintf(stderr, " ==> %p\n", res + 1); */
#ifdef PIKE_DEBUG
  if ((res->hdr) != hdr) {
    Pike_fatal("mexec_alloc: Resulting block is not member of hdr: %p != %p\n",
	       res->hdr, hdr);
  }
#endif /* PIKE_DEBUG */
  verify_mexec_hdr(hdr);
  return res + 1;
}

void *mexec_realloc(void *ptr, size_t sz)
{
  struct mexec_hdr *hdr;
  struct mexec_hdr *old_hdr = NULL;
  struct mexec_block *old;
  struct mexec_block *res = NULL;

  /* fprintf(stderr, "mexec_realloc(%p, %p)\n", ptr, (void *)sz); */

  if (!sz) {
    /* fprintf(stderr, " ==> NULL (no size)\n"); */
    return NULL;
  }
  if (!ptr) {
    /* fprintf(stderr, " ==> "); */
    return mexec_alloc(sz);
  }
  old = ((struct mexec_block *)ptr)-1;
  hdr = old->hdr;

#ifdef MEXEC_MAGIC
  if (old->magic != MEXEC_MAGIC) {
    Pike_fatal("mexec_realloc() called with non mexec pointer.\n"
	       "ptr: %p, magic: 0x%016llx, hdr: %p\n",
	       ptr, old->magic, hdr);
  }
#endif /* MEXEC_MAGIC */
  verify_mexec_hdr(hdr);

  sz = (sz + sizeof(struct mexec_block) + 0x1f) & ~0x1f;
  if (old->size >= (ptrdiff_t)sz) {
    /* fprintf(stderr, " ==> %p (space existed)\n", ptr); */
    return ptr;			/* FIXME: Shrinking? */
  }

  if ((((char *)old) + old->size) == hdr->bottom) {
    /* Attempt to grow the block. */
    if ((((char *)old) - ((char *)hdr)) <= (hdr->size - (ptrdiff_t)sz)) {
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
      pthread_jit_write_protect_np(0);
#endif
      old->size = sz;
      hdr->bottom = ((char *)old) + sz;
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
      pthread_jit_write_protect_np(1);
#endif
      /* fprintf(stderr, " ==> %p (succeded in growing)\n", ptr); */
#ifdef PIKE_DEBUG
      if (old->hdr != hdr) {
	Pike_fatal("mexec_realloc: "
		   "Grown block is not member of hdr: %p != %p\n",
		   old->hdr, hdr);
      }
#endif /* PIKE_DEBUG */
      verify_mexec_hdr(hdr);
      return ptr;
    }
    /* FIXME: Consider using grow_mexec_hdr to grow our hdr. */
    old_hdr = hdr;
  } else {
    res = low_mexec_alloc(hdr, sz);
  }
  if (!res) {
    hdr = mexec_hdrs;
    while (hdr && !(res = low_mexec_alloc(hdr, sz))) {
      hdr = hdr->next;
    }
    if (!hdr) {
      hdr = grow_mexec_hdr(old_hdr, sz);
      if (!hdr) {
	/* fprintf(stderr, " ==> NULL (grow failed)\n"); */
	return NULL;
      }
      if (hdr == old_hdr) {
	/* Succeeded in growing our old hdr. */
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
        pthread_jit_write_protect_np(0);
#endif
	old->size = sz;
	hdr->bottom = ((char *)old) + sz;
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
        pthread_jit_write_protect_np(1);
#endif
	/* fprintf(stderr, " ==> %p (succeded in growing hdr)\n", ptr); */
#ifdef PIKE_DEBUG
	if (old->hdr != hdr) {
	  Pike_fatal("mexec_realloc: "
		     "Grown block is not member of hdr: %p != %p\n",
		     old->hdr, hdr);
	}
#endif /* PIKE_DEBUG */
	verify_mexec_hdr(hdr);
	return ptr;
      }
      res = low_mexec_alloc(hdr, sz);
#ifdef PIKE_DEBUG
      if (!res) {
	Pike_fatal("mexec_realloc(%p, %"PRINTSIZET"u) failed to allocate from grown hdr!\n",
		   ptr, sz);
      }
#endif /* PIKE_DEBUG */
      verify_mexec_hdr(hdr);
    }
  }

  if (!res) {
    /* fprintf(stderr, " ==> NULL (low_mexec_alloc failed)\n"); */
    return NULL;
  }

  /* fprintf(stderr, " ==> %p\n", res + 1); */

#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(0);
#endif
  memcpy(res+1, old+1, old->size - sizeof(*old));
#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
  pthread_jit_write_protect_np(1);
#endif
  mexec_free(ptr);

#ifdef PIKE_DEBUG
  if (res->hdr != hdr) {
    Pike_fatal("mexec_realloc: "
	       "Resulting block is not member of hdr: %p != %p\n",
	       res->hdr, hdr);
  }
#endif /* PIKE_DEBUG */
  verify_mexec_hdr(hdr);
  return res + 1;
}

#elif defined (VALGRIND_DISCARD_TRANSLATIONS)

void *mexec_alloc (size_t sz)
{
  size_t *blk = malloc (sz + sizeof (size_t));
  if (!blk) return NULL;
  *blk = sz;
  return blk + 1;
}

void *mexec_realloc (void *ptr, size_t sz)
{
  if (ptr) {
    size_t *oldblk = ptr;
    size_t oldsz = oldblk[-1];
    size_t *newblk = realloc (oldblk - 1, sz + sizeof (size_t));
    if (!newblk) return NULL;
    if (newblk != oldblk)
      VALGRIND_DISCARD_TRANSLATIONS (oldblk, oldsz);
    else if (oldsz > sz)
      VALGRIND_DISCARD_TRANSLATIONS (newblk + sz, oldsz - sz);
    *newblk = sz;
    return newblk + 1;
  }
  return mexec_alloc (sz);
}

void mexec_free (void *ptr)
{
  size_t *blk = ptr;
  VALGRIND_DISCARD_TRANSLATIONS (blk, blk[-1]);
  free (blk - 1);
}

#else  /* !USE_MY_MEXEC_ALLOC && !VALGRIND_DISCARD_TRANSLATIONS */

void *mexec_alloc(size_t sz)
{
  return malloc(sz);
}
void *mexec_realloc(void *ptr, size_t sz)
{
  if (ptr) return realloc(ptr, sz);
  return malloc(sz);
}
void mexec_free(void *ptr)
{
  free(ptr);
}

#endif  /* !USE_MY_MEXEC_ALLOC && !VALGRIND_DISCARD_TRANSLATIONS */

/* #define DMALLOC_USE_HASHBASE */

/* #define DMALLOC_TRACE */
/* #define DMALLOC_TRACELOGSIZE	256*1024 */

/* #define DMALLOC_TRACE_MEMHDR ((struct memhdr *)0x40157218) */
/* #define DMALLOC_TRACE_MEMLOC ((struct memloc *)0x405500d8) */
#ifdef DMALLOC_TRACE_MEMLOC
#define DO_IF_TRACE_MEMLOC(X)	do { X; } while(0)
#else /* !DMALLOC_TRACE_MEMLOC */
#define DO_IF_TRACE_MEMLOC(X)
#endif /* DMALLOC_TRACE_MEMLOC */

#ifdef DMALLOC_TRACE
/* this can be used to supplement debugger data
 * to find out *how* the interpreter got to a certain
 * point, it is also useful when debuggers are not working.
 * -Hubbe
 *
 * Please note: We *should* probably make this so that
 * it remembers which thread was there too, and then we
 * would need a mutex to make sure all acceses to dmalloc_tracelogptr
 * are atomic.
 * -Hubbe
 */
char *dmalloc_tracelog[DMALLOC_TRACELOGSIZE];
size_t dmalloc_tracelogptr=0;

/*
 * This variable can be used to debug stack trashing,
 * simply insert:
 * {extern int dmalloc_print_trace; dmalloc_print_trace=1;}
 * somewhere before the code occurs, and from that point,
 * dmalloc will output all checkpoints to stderr.
 */
int dmalloc_print_trace;
#define DMALLOC_TRACE_LOG(X)
#endif

#ifdef DO_PIKE_CLEANUP
int exit_with_cleanup = 1;
int exit_cleanup_in_progress = 0;
#endif

#ifdef DEBUG_MALLOC

#undef DO_IF_DMALLOC
#define DO_IF_DMALLOC(X)
#undef DO_IF_NOT_DMALLOC
#define DO_IF_NOT_DMALLOC(X) X


#include "threads.h"

#ifdef _REENTRANT
static PIKE_MUTEX_T debug_malloc_mutex;

static void debug_malloc_atfatal(void)
{
  /* Release the debug_malloc_mutex if we already hold it
   * so that we don't end up in a dead-lock on attempting
   * to take it again when running Pike-code in Pike_fatal().
   */
  mt_trylock(&debug_malloc_mutex);
  mt_unlock(&debug_malloc_mutex);
}
#endif

#undef malloc
#undef free
#undef realloc
#undef calloc
#undef strdup
#undef main

#ifdef USE_DL_MALLOC
#undef ENCAPSULATE_MALLOC
#define calloc    dlcalloc
#define free	  dlfree
#define malloc(x) dlmalloc(x)
#define memalign  dlmemalign
#define realloc	  dlrealloc
#define valloc	  dlvalloc
#define pvalloc	  dlpvalloc
#endif

#ifdef HAVE_DLOPEN
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif


#ifdef ENCAPSULATE_MALLOC
#ifdef RTLD_NEXT

#ifdef TH_KEY_T
#define DMALLOC_REMEMBER_LAST_LOCATION

TH_KEY_T dmalloc_last_seen_location;

#endif


#define dl_setup()  ( real_malloc ? 0 : real_dl_setup() )

void *(*real_malloc)(size_t);
void (*real_free)(void *);
void *(*real_realloc)(void *,size_t);
void *(*real_calloc)(size_t,size_t);

union fake_body
{
  struct fakemallocblock *next;
  char block[1];
  char *pad;
  double pad2;
};

struct fakemallocblock
{
  size_t size;
  union fake_body body;
};

static struct fakemallocblock fakemallocarea[65536];
static struct fakemallocblock *fake_free_list;
#define ALIGNMENT OFFSETOF(fakemallocblock, body)
#define FAKEMALLOCED(X) \
  ( ((char *)(X)) >= ((char *)fakemallocarea) && ((char *)(X)) < (((char *)(fakemallocarea))+sizeof(fakemallocarea)))

void init_fake_malloc(void)
{
  fake_free_list=fakemallocarea;
  fake_free_list->size=sizeof(fakemallocarea) - ALIGNMENT;
  fake_free_list->body.next=0;
}

#if 0
#define LOWDEBUG2(X) write(2,X,strlen(X) - strlen(""));
#define LOWDEBUG(X) LOWDEBUG2( __FILE__ ":" DEFINETOSTR(__LINE__) ":" X)

void debug_output(char *x, void *p)
{
  char buf[120];
  write(2,x,strlen(x));
  sprintf(buf," (%p %d)\n",p,p);
  write(2,buf,strlen(buf));
}

#define LOWDEBUG3(X,Y) debug_output( __FILE__ ":" DEFINETOSTR(__LINE__) ":" X,Y)
#else
#define LOWDEBUG(X)
#define LOWDEBUG3(X,Y)
#endif

void *fake_malloc(size_t x)
{
  void *ret;
  struct fakemallocblock *block, **prev;

  LOWDEBUG3("fakemalloc",x);

  if(real_malloc)
  {
    ret=real_malloc(x);
    LOWDEBUG3("fakemalloc --> ",ret);
    return ret;
  }

  if(!x) return 0;

  if(!fake_free_list) init_fake_malloc();

  x+=ALIGNMENT-1;
  x&=-ALIGNMENT;

  for(prev=&fake_free_list;(block=*prev);prev=& block->body.next)
  {
    if(block->size >= x)
    {
      if(block->size > x + ALIGNMENT)
      {
	/* Split block */
	block->size-=x+ALIGNMENT;
	block=(struct fakemallocblock *)(block->body.block+block->size);
	block->size=x;
      }else{
	/* just return block */
	*prev = block->body.next;
      }
      LOWDEBUG3("fakemalloc --> ",block->body.block);
      return block->body.block;
    }
  }

  LOWDEBUG("Out of memory.\n");
  return 0;
}

PMOD_EXPORT void *malloc(size_t x)
{
  void *ret;
  LOWDEBUG3("malloc",x);
  if(!x) return 0;
  if(real_malloc)
  {
#ifdef DMALLOC_REMEMBER_LAST_LOCATION
    char * tmp=(char *)th_getspecific(dmalloc_last_seen_location);
    if(tmp)
    {
      if(LOCATION_TYPE (tmp) == 'S' && tmp[-1]=='N')
	ret=debug_malloc(x,tmp-1);
      else
	ret=debug_malloc(x,tmp);
    }else{
      ret=debug_malloc(x,DMALLOC_LOCATION());
    }
#else
    ret=debug_malloc(x,DMALLOC_LOCATION());
#endif
  }else{
    ret=fake_malloc(x);
  }
#ifndef REPORT_ENCAPSULATED_MALLOC
  dmalloc_accept_leak(ret);
#endif
  LOWDEBUG3("malloc --> ",ret);
  return ret;
}

PMOD_EXPORT void fake_free(void *x)
{
  struct fakemallocblock * block;

  if(!x) return;

  LOWDEBUG3("fake_free",x);

  if(FAKEMALLOCED(x))
  {
    block=BASEOF(x,fakemallocblock,body.block);
    block->body.next=fake_free_list;
    fake_free_list=block;
  }else{
    return real_free(x);
  }
}

PMOD_EXPORT void free(void *x)
{
  struct fakemallocblock * block;

  if(!x) return;
  LOWDEBUG3("free",x);
  if(FAKEMALLOCED(x))
  {
    block=BASEOF(x,fakemallocblock,body.block);
    block->body.next=fake_free_list;
    fake_free_list=block;
  }else{
    return debug_free(x, DMALLOC_LOCATION(), 0);
  }
}

PMOD_EXPORT void *realloc(void *x,size_t y)
{
  void *ret;
  size_t old_size;
  struct fakemallocblock * block;

  LOWDEBUG3("realloc <-",x);
  LOWDEBUG3("realloc ",y);

  if(FAKEMALLOCED(x) || !real_realloc)
  {
    old_size = x?BASEOF(x,fakemallocblock,body.block)->size :0;
    LOWDEBUG3("realloc oldsize",old_size);
    if(old_size >= y) return x;
    ret=malloc(y);
    if(!ret) return 0;
    memcpy(ret, x, old_size);
    if(x) free(x);
  }else{
    ret=debug_realloc(x, y, DMALLOC_LOCATION());
  }
#ifndef REPORT_ENCAPSULATED_MALLOC
  dmalloc_accept_leak(ret);
#endif
  LOWDEBUG3("realloc --> ",ret);
  return ret;
}

void *fake_realloc(void *x,size_t y)
{
  void *ret;
  size_t old_size;
  struct fakemallocblock * block;

  LOWDEBUG3("fake_realloc <-",x);
  LOWDEBUG3("fake_realloc ",y);

  if(FAKEMALLOCED(x) || !real_realloc)
  {
    old_size = x?BASEOF(x,fakemallocblock,body.block)->size :0;
    if(old_size >= y) return x;
    ret=malloc(y);
    if(!ret) return 0;
    memcpy(ret, x, old_size);
    if(x) free(x);
  }else{
    ret=real_realloc(x,y);
  }
#ifndef REPORT_ENCAPSULATED_MALLOC
  dmalloc_accept_leak(ret);
#endif
  LOWDEBUG3("fake_realloc --> ",ret);
  return ret;
}

void *calloc(size_t x, size_t y)
{
  void *ret;
  LOWDEBUG3("calloc x",x);
  LOWDEBUG3("calloc y",y);
  ret=malloc(x*y);
  if(ret) memset(ret,0,x*y);
#ifndef REPORT_ENCAPSULATED_MALLOC
  dmalloc_accept_leak(ret);
#endif
  LOWDEBUG3("calloc --> ",ret);
  return ret;
}

void *fake_calloc(size_t x, size_t y)
{
  void *ret;
  ret=fake_malloc(x*y);
  if(ret) memset(ret,0,x*y);
#ifndef REPORT_ENCAPSULATED_MALLOC
  dmalloc_accept_leak(ret);
#endif
  LOWDEBUG3("fake_calloc --> ",ret);
  return ret;
}


#define DMALLOC_USING_DLOPEN

#endif /* RTLD_NEXT */
#endif /* ENCAPSULATE_MALLOC */
#endif /* HAVE_DLOPEN */

#ifndef DMALLOC_USING_DLOPEN
#define real_malloc malloc
#define real_free free
#define real_calloc calloc
#define real_realloc realloc

#define fake_malloc malloc
#define fake_free free
#define fake_calloc calloc
#define fake_realloc realloc
#else
#define malloc(x) fake_malloc(x)
#define free fake_free
#define realloc fake_realloc
#define calloc fake_calloc
#endif


/* Don't understand what this is supposed to do, but it won't work
 * with USE_DL_MALLOC. /mast */
#ifdef WRAP
#define malloc(x) __real_malloc(x)
#define free __real_free
#define realloc __real_realloc
#define calloc __real_calloc
#define strdup __real_strdup
#endif


static struct memhdr *my_find_memhdr(void *, int);
static void dump_location_bt (LOCATION l, int indent, const char *prefix);


int verbose_debug_malloc = 0;
int debug_malloc_check_all = 0;

/* #define DMALLOC_PROFILE */
#define DMALLOC_AD_HOC
/* #define DMALLOC_VERIFY_INTERNALS */

#ifdef DMALLOC_AD_HOC
/* A gigantic size (16Mb) will help a lot in AD_HOC mode */
#define LHSIZE 4100011
#else
#define LHSIZE 1109891
#endif

#ifdef DMALLOC_VERIFY_INTERNALS
#define DO_IF_VERIFY_INTERNALS(X) X
#else
#define DO_IF_VERIFY_INTERNALS(X)
#endif

#define DSTRHSIZE 10007

#define DEBUG_MALLOC_PAD 32
#define FREE_DELAY 4096
#define MAX_UNFREE_MEM 1024*1024*32
#define RNDSIZE 1777 /* A small size will help keep it in the cache */
#define AD_HOC_CHECK_INTERVAL 620 * 10

#define BT_MAX_FRAMES 10
#define ALLOC_BT_MAX_FRAMES 15

#ifdef DMALLOC_C_STACK_TRACE
#define GET_ALLOC_BT(BT)						\
  c_stack_frame BT[ALLOC_BT_MAX_FRAMES];				\
  int PIKE_CONCAT (BT, _len) = backtrace (BT, ALLOC_BT_MAX_FRAMES)
#define BT_ARGS(BT) , BT, PIKE_CONCAT (BT, _len)
#else
#define GET_ALLOC_BT(BT)
#define BT_ARGS(BT)
#endif

static void *blocks_to_free[FREE_DELAY];
static unsigned int blocks_to_free_ptr=0;
static unsigned long unfree_mem=0;
static int exiting=0;

struct memloc
{
  struct memloc *next;
  struct memhdr *mh;
  struct memloc *hash_next;
  struct memloc **hash_prev;
  LOCATION location;
  int times;
};

/* MEM_FREE is defined by winnt.h */
#ifdef MEM_FREE
#undef MEM_FREE
#endif

#define MEM_PADDED			1
#define MEM_WARN_ON_FREE		2
#define MEM_REFERENCED			4
#define MEM_IGNORE_LEAK			8
#define MEM_FREE			16
#define MEM_LOCS_ADDED_TO_NO_LEAKS	32
#define MEM_TRACE			64
#define MEM_SCANNED			128

static struct block_allocator memloc_allocator = BA_INIT_PAGES(sizeof(struct memloc), 64);

static struct memloc * alloc_memloc() {
    return ba_alloc(&memloc_allocator);
}

static void really_free_memloc(struct memloc * ml) {
    ba_free(&memloc_allocator, ml);
}

void count_memory_in_memlocs(size_t * n, size_t * s) {
    ba_count_all(&memloc_allocator, n, s);
}

struct memhdr
{
  struct memhdr *next;
  long size;
  int flags;
#ifdef DMALLOC_USE_HASHBASE
  long hashbase;	/* Cached calculation of ((long)&next)*53 */
#define INIT_HASHBASE(X)	(((X)->hashbase) = (((long)(X))*53))
#else /* !DMALLOC_USE_HASHBASE */
#define INIT_HASHBASE(X)
#endif /* DMALLOC_USE_HASHBASE */
#ifdef DMALLOC_VERIFY_INTERNALS
  int times;		/* Should be equal to `+(@(locations->times))... */
#endif
  int gc_generation;
#ifdef DMALLOC_C_STACK_TRACE
  c_stack_frame alloc_bt[ALLOC_BT_MAX_FRAMES];
  int alloc_bt_len;
#endif
  void *data;
  struct memloc *locations;
};

static struct memloc *mlhash[LHSIZE];
static char rndbuf[RNDSIZE + DEBUG_MALLOC_PAD*2];

static struct memhdr no_leak_memlocs;
static int memheader_references_located=0;


#ifdef DMALLOC_PROFILE
static int add_location_calls=0;
static int add_location_seek=0;
static int add_location_new=0;
static int add_location_cache_hits=0;
static int add_location_duplicate=0;  /* Used in AD_HOC mode */
#endif

#if DEBUG_MALLOC_PAD - 0 > 0
char *do_pad(char *mem, long size)
{
  mem+=DEBUG_MALLOC_PAD;

  if (PIKE_MEM_CHECKER()) {
    PIKE_MEM_NA_RANGE(mem - DEBUG_MALLOC_PAD, DEBUG_MALLOC_PAD);
    PIKE_MEM_NA_RANGE(mem + size, DEBUG_MALLOC_PAD);
  }

  else {
    unsigned long q,e;
    q= (((long)mem) ^ 0x555555) + (size * 9248339);

    /*  fprintf(stderr,"Padding  %p(%d) %ld\n",mem, size, q); */
#if 1
    q%=RNDSIZE;
    memcpy(mem - DEBUG_MALLOC_PAD, rndbuf+q, DEBUG_MALLOC_PAD);
    memcpy(mem + size, rndbuf+q, DEBUG_MALLOC_PAD);
#else
    for(e=0;e< DEBUG_MALLOC_PAD; e+=4)
    {
      char tmp;
      q=(q<<13) ^ ~(q>>5);

#define BLORG(X,Y)							\
      tmp=(Y);								\
      mem[e+(X)-DEBUG_MALLOC_PAD] = tmp;				\
      mem[size+e+(X)] = tmp;

      BLORG(0, (q) | 1)
	BLORG(1, (q >> 5) | 1)
	BLORG(2, (q >> 10) | 1)
	BLORG(3, (q >> 15) | 1)
	}
#endif
  }

  return mem;
}

#define FD2PTR(X) (void *)(ptrdiff_t)((X)*4+1)
#define PTR2FD(X) (((ptrdiff_t)(X))>>2)


void check_pad(struct memhdr *mh, int freeok)
{
  static int out_biking=0;
  unsigned long q,e;
  char *mem=mh->data;
  long size=mh->size;
  DECLARE_CYCLIC();

  if(out_biking) return;

  if(!(mh->flags & MEM_PADDED)) return;

  /* NB: We need to use the LOW_CYCLIC_* variants here as
   *     we do not necessarily hold the interpreter lock.
   */
  if (LOW_BEGIN_CYCLIC(mh, NULL)) return;
  SET_CYCLIC_RET(1);

  if(size < 0)
  {
    if(!freeok)
    {
      fprintf(stderr,"Access to free block: %p (size %ld)!\n",mem, ~mh->size);
      dump_memhdr_locations(mh, 0, 0);
      debug_malloc_atfatal();
      locate_references(mem);
      describe(mem);
      abort();
    }else{
      size = ~size;
    }
  }

  if (PIKE_MEM_CHECKER()) {
    LOW_END_CYCLIC();
    return;
  }

/*  fprintf(stderr,"Checking %p(%d) %ld\n",mem, size, q);  */
#if 1
  /* optimization? */
  if(memcmp(mem - DEBUG_MALLOC_PAD, mem+size, DEBUG_MALLOC_PAD))
  {
    q= (((long)mem) ^ 0x555555) + (size * 9248339);

    q%=RNDSIZE;
    if(memcmp(mem - DEBUG_MALLOC_PAD, rndbuf+q, DEBUG_MALLOC_PAD))
    {
      out_biking=1;
      fprintf(stderr,"Pre-padding overwritten for "
	      "block at %p (size %ld)!\n", mem, size);
      describe(mem);
      abort();
    }

    if(memcmp(mem + size, rndbuf+q, DEBUG_MALLOC_PAD))
    {
      out_biking=1;
      fprintf(stderr,"Post-padding overwritten for "
	      "block at %p (size %ld)!\n", mem, size);
      describe(mem);
      abort();
    }

    out_biking=1;
    fprintf(stderr,"Padding completely screwed up for "
	    "block at %p (size %ld)!\n", mem, size);
    describe(mem);
    abort();
  }
#else
  q= (((long)mem) ^ 0x555555) + (size * 9248339);

  for(e=0;e< DEBUG_MALLOC_PAD; e+=4)
  {
    char tmp;
    q=(q<<13) ^ ~(q>>5);

#undef BLORG
#define BLORG(X,Y) 						\
    tmp=(Y);                                                    \
    if(mem[e+(X)-DEBUG_MALLOC_PAD] != tmp)			\
    {								\
      out_biking=1;						\
      fprintf(stderr,"Pre-padding overwritten for "		\
	      "block at %p (size %ld) (e=%ld %d!=%d)!\n",	\
	      mem, size, e, tmp, mem[e-DEBUG_MALLOC_PAD]);	\
      describe(mem);						\
      abort();							\
    }								\
    if(mem[size+e+(X)] != tmp)					\
    {								\
      out_biking=1;						\
      fprintf(stderr,"Post-padding overwritten for "		\
	      "block at %p (size %ld) (e=%ld %d!=%d)!\n",	\
	      mem, size, e, tmp, mem[size+e]);			\
      describe(mem);						\
      abort();							\
    }

    BLORG(0, (q) | 1)
    BLORG(1, (q >> 5) | 1)
    BLORG(2, (q >> 10) | 1)
    BLORG(3, (q >> 15) | 1)
  }
#endif
  LOW_END_CYCLIC();
}
#else
#define do_pad(X,Y) (X)
#define check_pad(M,X)
#endif


static inline unsigned long lhash(struct memhdr *m, LOCATION location)
{
  unsigned long l;
#ifdef DMALLOC_USE_HASHBASE
  l = m->hashbase;
#else /* !DMALLOC_USE_HASHBASE */
  l=(long)m;
  l*=53;
#endif /* DMALLOC_USE_HASHBASE */
  l+=(long)location;
  l%=LHSIZE;
  return l;
}

#include "block_alloc.h"

#undef INIT_BLOCK
#undef EXIT_BLOCK

#define INIT_BLOCK(X) do {				\
    X->locations = NULL;				\
    X->flags=0;						\
    INIT_HASHBASE(X);					\
    DO_IF_VERIFY_INTERNALS(				\
      X->times=0;					\
    );							\
  } while(0)
#define EXIT_BLOCK(X) do {				\
  struct memloc *ml;					\
  DO_IF_VERIFY_INTERNALS(				\
    int times = 0;					\
    ml = X->locations;					\
    while(ml)						\
    {							\
      times += ml->times;				\
      ml = ml->next;					\
    }							\
    if (times != X->times) {				\
      dump_memhdr_locations(X, 0, 0);			\
      Pike_fatal("%p: Dmalloc lost locations for block at %p? " \
		 "total:%d  !=  accumulated:%d\n",	\
		 X, X->data, X->times, times);		\
    }							\
  );							\
  while((ml=X->locations))				\
  {							\
    /* Unlink from the hash-table. */			\
    ml->hash_prev[0] = ml->hash_next;			\
    if (ml->hash_next) {				\
      ml->hash_next->hash_prev = ml->hash_prev;		\
    }							\
							\
    X->locations=ml->next;				\
    DO_IF_TRACE_MEMLOC(					\
      if (ml == DMALLOC_TRACE_MEMLOC) {			\
        fprintf(stderr, "EXIT:Freeing memloc %p location "	\
		"was %s memhdr: %p data: %p\n",		\
		ml, ml->location, ml->mh, ml->mh->data);\
    });							\
    if (X->flags & MEM_TRACE) {				\
      fprintf(stderr, "  0x%p: Freeing loc: %p:%s\n",	\
	      X, ml, ml->location);			\
    }							\
    really_free_memloc(ml);				\
  }							\
}while(0)

#undef BLOCK_ALLOC_HSIZE_SHIFT
#define BLOCK_ALLOC_HSIZE_SHIFT 1

PTR_HASH_ALLOC_FILL_PAGES(memhdr, 128)

#undef INIT_BLOCK
#undef EXIT_BLOCK

#define INIT_BLOCK(X)
#define EXIT_BLOCK(X)



static struct memhdr *my_find_memhdr(void *p, int already_gone)
{
  struct memhdr *mh;

#if DEBUG_MALLOC_PAD - 0 > 0
  if(debug_malloc_check_all)
  {
    unsigned long h;
    for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
    {
      for(mh=memhdr_hash_table[h]; mh; mh=mh->next)
      {
	check_pad(mh,1);
      }
    }
  }
#endif

  if((mh=find_memhdr(p))) {
#ifdef DMALLOC_VERIFY_INTERNALS
    if (mh->data != p) {
      Pike_fatal("find_memhdr() returned memhdr for different block\n");
    }
#endif
    if(!already_gone)
      check_pad(mh,0);
  }

  return mh;
}


static struct memloc *find_location(struct memhdr *mh, LOCATION location)
{
  struct memloc *ml;
  unsigned long l=lhash(mh,location);

  for (ml = mlhash[l]; ml; ml = ml->hash_next) {
    if (ml->mh==mh && ml->location==location) {
      if (ml != mlhash[l]) {
#ifdef DMALLOC_TRACE_MEMLOC
	if (ml == DMALLOC_TRACE_MEMLOC) {
	  fprintf(stderr,
		  "find_loc: Found memloc %p location %s memhdr: %p data: %p\n",
		  ml, ml->location, ml->mh, ml->mh->data);
	}
#endif /* DMALLOC_TRACE_MEMLOC */
	/* Relink the hash bucket. */
	if ((ml->hash_prev[0] = ml->hash_next)) {
	  ml->hash_next->hash_prev = ml->hash_prev;
	}
	ml->hash_prev = &mlhash[l];
	ml->hash_next = mlhash[l];
	mlhash[l]->hash_prev = &ml->hash_next;
	mlhash[l]=ml;
      }
      return ml;
    }
  }

  return NULL;
}

static void add_location(struct memhdr *mh,
			 LOCATION location)
{
  struct memloc *ml;
  unsigned long l=lhash(mh,location);

#ifdef DMALLOC_TRACE
  if(dmalloc_print_trace)
    fprintf(stderr,"%p: %s\n", mh, location);
  DMALLOC_TRACE_LOG(location);
#endif

#if DEBUG_MALLOC - 0 < 2
  if(find_location(&no_leak_memlocs, location)) return;
#endif

#ifdef DMALLOC_VERIFY_INTERNALS
  mh->times++;
#endif

  if (mh->flags & MEM_TRACE) {
    fprintf(stderr, "add_location(0x%p, %s)\n", mh, location);
  }

#ifdef DMALLOC_PROFILE
  add_location_calls++;
#endif

  if ((ml = find_location(mh, location))) {
    ml->times++;
    return;
  }

  if (mh->flags & MEM_TRACE) {
    fprintf(stderr, "Creating a new entry.\n");
  }

#ifdef DMALLOC_PROFILE
  add_location_new++;
#endif
  ml=alloc_memloc();
  ml->times=1;
  ml->location=location;
  ml->next=mh->locations;
  ml->mh=mh;
  mh->locations=ml;

  ml->hash_prev = &mlhash[l];
  if ((ml->hash_next = mlhash[l])) {
    mlhash[l]->hash_prev = &ml->hash_next;
  }
  mlhash[l]=ml;
#ifdef DMALLOC_TRACE_MEMLOC
  if (ml == DMALLOC_TRACE_MEMLOC) {
    fprintf(stderr, "add_loc: Allocated memloc %p location %s memhdr: %p data: %p\n",
	    ml, ml->location, ml->mh, ml->mh->data);
  }
#endif /* DMALLOC_TRACE_MEMLOC */
  return;
}

PMOD_EXPORT LOCATION dmalloc_default_location=0;

static struct memhdr *low_make_memhdr(void *p, int s, LOCATION location
#ifdef DMALLOC_C_STACK_TRACE
				      , c_stack_frame *bt, int bt_len
#endif
				     )
{
  struct memhdr *mh = get_memhdr(p);
  struct memloc *ml = alloc_memloc();
  unsigned long l;

  if (mh->locations) {
    dump_memhdr_locations(mh, NULL, 0);
    Pike_fatal("New block at %p already has locations.\n"
	       "location: %s\n", p, location);
  }

  l = lhash(mh,location);
  mh->size=s;
  mh->flags=0;
#ifdef DMALLOC_VERIFY_INTERNALS
  mh->times=1;
#endif
  ml->next = mh->locations;
  mh->locations = ml;
  mh->gc_generation=gc_generation * 1000 + Pike_in_gc;
#ifdef DMALLOC_C_STACK_TRACE
  if (bt_len > 0) {
    memcpy (mh->alloc_bt, bt, bt_len * sizeof (c_stack_frame));
    mh->alloc_bt_len = bt_len;
  }
  else
    mh->alloc_bt_len = 0;
#endif
  ml->location=location;
  ml->times=1;
  ml->mh=mh;
  ml->hash_prev = &mlhash[l];
  if ((ml->hash_next = mlhash[l])) {
    mlhash[l]->hash_prev = &ml->hash_next;
  }
  mlhash[l]=ml;

#ifdef DMALLOC_TRACE_MEMHDR
  if (mh == DMALLOC_TRACE_MEMHDR) {
    fprintf(stderr, "Allocated memhdr 0x%p\n", mh);
    mh->flags |= MEM_TRACE;
  }
#endif /* DMALLOC_TRACE_MEMHDR */

#ifdef DMALLOC_TRACE_MEMLOC
  if (ml == DMALLOC_TRACE_MEMLOC) {
    fprintf(stderr, "mk_mhdr: Allocated memloc %p location %s memhdr: %p data: %p\n",
	    ml, ml->location, ml->mh, ml->mh->data);
  }
#endif /* DMALLOC_TRACE_MEMLOC */

  if(dmalloc_default_location)
    add_location(mh, dmalloc_default_location);
  return mh;
}

PMOD_EXPORT void dmalloc_trace(void *p)
{
  struct memhdr *mh = my_find_memhdr(p, 0);
  if (mh) {
    mh->flags |= MEM_TRACE;
  }
}

PMOD_EXPORT void dmalloc_register(void *p, int s, LOCATION location)
{
  struct memhdr *mh;
  GET_ALLOC_BT (bt);
  mt_lock(&debug_malloc_mutex);
  mh = my_find_memhdr(p, 0);
  if (!mh) {
    low_make_memhdr(p, s, location BT_ARGS (bt));
  }
  mt_unlock(&debug_malloc_mutex);
  if (mh) {
    dump_memhdr_locations(mh, NULL, 0);
    Pike_fatal("dmalloc_register(%p, %d, \"%s\"): Already registered\n",
	       p, s, location);
  }
}

PMOD_EXPORT void dmalloc_accept_leak(void *p)
{
  if(p)
  {
    struct memhdr *mh;
    mt_lock(&debug_malloc_mutex);
    if((mh=my_find_memhdr(p,0)))
      mh->flags |= MEM_IGNORE_LEAK;
    mt_unlock(&debug_malloc_mutex);
  }
}

static void low_add_marks_to_memhdr(struct memhdr *to,
				    struct memhdr *from)
{
  struct memloc *l;
  if(!from) return;
  for(l=from->locations;l;l=l->next)
    add_location(to, l->location);
}

void add_marks_to_memhdr(struct memhdr *to, void *ptr)
{
  mt_lock(&debug_malloc_mutex);

  low_add_marks_to_memhdr(to,my_find_memhdr(ptr,0));

  mt_unlock(&debug_malloc_mutex);
}

static void unregister_memhdr(struct memhdr *mh, int already_gone)
{
  if(mh->size < 0) mh->size=~mh->size;
  if(!already_gone) check_pad(mh,0);
  if(!(mh->flags & MEM_LOCS_ADDED_TO_NO_LEAKS))
    low_add_marks_to_memhdr(&no_leak_memlocs, mh);
  if (mh->flags & MEM_TRACE) {
    fprintf(stderr, "Removing memhdr %p\n", mh);
  }
  if (!remove_memhdr(mh->data)) {
#ifdef DMALLOC_VERIFY_INTERNALS
    Pike_fatal("remove_memhdr(%p) returned false.\n", mh->data);
#endif
  }
}

static int low_dmalloc_unregister(void *p, int already_gone)
{
  struct memhdr *mh=find_memhdr(p);
  if(mh)
  {
    unregister_memhdr(mh, already_gone);
    return 1;
  }
  return 0;
}

PMOD_EXPORT int dmalloc_unregister(void *p, int already_gone)
{
  int ret;
  mt_lock(&debug_malloc_mutex);
  ret=low_dmalloc_unregister(p,already_gone);
  mt_unlock(&debug_malloc_mutex);
  return ret;
}

static void dmalloc_ba_walk_unregister_cb(struct ba_iterator *it,
					  void *UNUSED(ignored))
{
  do {
    low_dmalloc_unregister(ba_it_val(it), 0);
  } while (ba_it_step(it));
}

PMOD_EXPORT void dmalloc_unregister_all(struct block_allocator *a)
{
  mt_lock(&debug_malloc_mutex);
  ba_walk(a, dmalloc_ba_walk_unregister_cb, NULL);
  mt_unlock(&debug_malloc_mutex);
}

static int low_dmalloc_mark_as_free(void *p, int UNUSED(already_gone))
{
  struct memhdr *mh=find_memhdr(p);
  if(mh)
  {
    if (mh->flags & MEM_TRACE) {
      fprintf(stderr, "Marking memhdr %p as free\n", mh);
    }
    if(!(mh->flags & MEM_FREE))
    {
      mh->size=~mh->size;
      mh->flags |= MEM_FREE | MEM_IGNORE_LEAK | MEM_LOCS_ADDED_TO_NO_LEAKS;
      low_add_marks_to_memhdr(&no_leak_memlocs, mh);
    }
    return 1;
  }
  return 0;
}

PMOD_EXPORT int dmalloc_mark_as_free(void *p, int already_gone)
{
  int ret;
  mt_lock(&debug_malloc_mutex);
  ret=low_dmalloc_mark_as_free(p,already_gone);
  mt_unlock(&debug_malloc_mutex);
  return ret;
}

static void flush_blocks_to_free(void)
{
  int i;

  if(verbose_debug_malloc)
    fprintf(stderr, "flush_blocks_to_free()\n");

  for (i=0; i < FREE_DELAY; i++) {
    void *p;
    if ((p = blocks_to_free[i])) {
      struct memhdr *mh = my_find_memhdr(p, 1);
      if (!mh) {
	fprintf(stderr, "Lost track of a freed memory block: %p!\n", p);
	abort();
      }

      blocks_to_free[i] = 0;

      PIKE_MEM_RW_RANGE((char *) p - DEBUG_MALLOC_PAD,
			(mh->size > 0 ? mh->size : ~mh->size) + 2 * DEBUG_MALLOC_PAD);
#ifdef DMALLOC_TRACK_FREE
      unregister_memhdr(mh,0);
#else /* !DMALLOC_TRACK_FREE */
      remove_memhdr(p);
#endif /* DMALLOC_TRACK_FREE */
      real_free( ((char *)p) - DEBUG_MALLOC_PAD );
    }
  }
}

MALLOC_FUNCTION
PMOD_EXPORT void * system_malloc(size_t n) {
    return real_malloc(n);
}

PMOD_EXPORT void system_free(void * p) {
    real_free(p);
}

PMOD_EXPORT void *debug_malloc(size_t s, LOCATION location)
{
  char *m;

  /* Complain on attempts to allocate more than 16M svalues
   * (typically 128MB on ILP32 and 256MB on LP64).
   *
   * Note that Tools.Shoot.Foreach3 assumes that it is
   * possible to allocate an array with 10000000 svalues.
   */
  if (s > (size_t)(0x01000000 * sizeof(struct svalue))) {
    Pike_fatal("malloc(0x%08lx) -- Huge malloc!\n", (unsigned long)s);
  }

  mt_lock(&debug_malloc_mutex);

  m=(char *)real_malloc(s + DEBUG_MALLOC_PAD*2);
  if(m)
  {
    GET_ALLOC_BT (bt);
    m=do_pad(m, s);
    low_make_memhdr(m, s, location BT_ARGS (bt))->flags|=MEM_PADDED;
  } else {
    flush_blocks_to_free();
    if ((m=(char *)real_malloc(s + DEBUG_MALLOC_PAD*2))) {
      GET_ALLOC_BT (bt);
      m=do_pad(m, s);
      low_make_memhdr(m, s, location BT_ARGS (bt))->flags|=MEM_PADDED;
    }
  }

  if(verbose_debug_malloc)
    fprintf(stderr, "malloc(%ld) => %p  (%s)\n",
            (long)s, m, LOCATION_NAME(location));

  mt_unlock(&debug_malloc_mutex);
  return m;
}

PMOD_EXPORT void *debug_calloc(size_t a, size_t b, LOCATION location)
{
  void *m=debug_malloc(a*b,location);
  if(m)
    memset(m, 0, a*b);

  if(verbose_debug_malloc)
    fprintf(stderr, "calloc(%ld, %ld) => %p  (%s)\n",
            (long)a, (long)b, m, LOCATION_NAME(location));

  return m;
}

PMOD_EXPORT void *debug_realloc(void *p, size_t s, LOCATION location)
{
  char *m,*base;
  struct memhdr *mh = 0;
  mt_lock(&debug_malloc_mutex);

  if (p && (mh = my_find_memhdr(p,0))) {
    base = (char *) p - DEBUG_MALLOC_PAD;
    PIKE_MEM_RW_RANGE(base, mh->size + 2 * DEBUG_MALLOC_PAD);
  }
  else
    base = p;
  m=fake_realloc(base, s+DEBUG_MALLOC_PAD*2);

  if(m) {
    m=do_pad(m, s);
    if (mh) {
      mh->size = s;
      add_location(mh, location);
      move_memhdr(mh, m);
    }
    else {
      GET_ALLOC_BT (bt);
      low_make_memhdr(m, s, location BT_ARGS (bt))->flags|=MEM_PADDED;
    }
  }
  if(verbose_debug_malloc)
    fprintf(stderr, "realloc(%p, %ld) => %p  (%s)\n",
            p, (long)s, m, LOCATION_NAME(location));
  mt_unlock(&debug_malloc_mutex);
  return m;
}

PMOD_EXPORT void debug_free(void *p, LOCATION location, int mustfind)
{
  struct memhdr *mh;
  if(!p) return;
  mt_lock(&debug_malloc_mutex);

#ifdef WRAP
  mustfind=1;
#endif

  mh=my_find_memhdr(p,0);

  if(verbose_debug_malloc || (mh && (mh->flags & MEM_WARN_ON_FREE)))
    fprintf(stderr, "free(%p) (%s)\n", p, LOCATION_NAME(location));

  if(!mh && mustfind && p)
  {
    fprintf(stderr,"Lost track of a mustfind memory block: %p!\n",p);
    abort();
  }

  if(!exiting && mh)
  {
    void *p2;
    if (PIKE_MEM_CHECKER())
      PIKE_MEM_NA_RANGE(p, mh->size);
    else
      memset(p, 0x55, mh->size);
    if(mh->size < MAX_UNFREE_MEM/FREE_DELAY)
    {
      add_location(mh, location);
      mh->size = ~mh->size;
      mh->flags|=MEM_FREE | MEM_IGNORE_LEAK;
      blocks_to_free_ptr++;
      blocks_to_free_ptr%=FREE_DELAY;
      p2=blocks_to_free[blocks_to_free_ptr];
      blocks_to_free[blocks_to_free_ptr]=p;
      if((p=p2))
      {
	mh=my_find_memhdr(p,1);
	if(!mh)
	{
	  fprintf(stderr,"Lost track of a freed memory block: %p!\n",p);
	  abort();
	}
      }else{
	mh=0;
      }
    }
  }

  if(mh)
  {
    PIKE_MEM_RW_RANGE((char *) p - DEBUG_MALLOC_PAD,
		      (mh->size > 0 ? mh->size : ~mh->size) + 2 * DEBUG_MALLOC_PAD);
#ifdef DMALLOC_TRACK_FREE
    unregister_memhdr(mh,0);
#else /* !DMALLOC_TRACK_FREE */
    remove_memhdr(p);
#endif /* DMALLOC_TRACK_FREE */
    real_free( ((char *)p) - DEBUG_MALLOC_PAD );
  }
  else
  {
    fake_free(p); /* may be a fakemalloc block */
  }
  mt_unlock(&debug_malloc_mutex);
}

/* Return true if a block is allocated. If must_be_freed is set,
 * return true if the block is allocated and leaking it is not
 * accepted. */
PMOD_EXPORT int dmalloc_check_allocated (void *p, int must_be_freed)
{
  int res;
  struct memhdr *mh;
  mt_lock(&debug_malloc_mutex);
  mh=my_find_memhdr(p,0);
  res = mh && mh->size>=0;
  if (res && must_be_freed)
    res = !(mh->flags & MEM_IGNORE_LEAK);
  mt_unlock(&debug_malloc_mutex);
  return res;
}

PMOD_EXPORT void dmalloc_free(void *p)
{
  debug_free(p, DMALLOC_LOCATION(), 0);
}

PMOD_EXPORT char *debug_strdup(const char *s, LOCATION location)
{
  char *m;
  long length;
  length=strlen(s);
  m=(char *)debug_malloc(length+1,location);
  memcpy(m,s,length+1);

  if(verbose_debug_malloc)
    fprintf(stderr, "strdup(\"%s\") => %p  (%s)\n", s, m, LOCATION_NAME(location));

  return m;
}

#ifdef WRAP
void *__wrap_malloc(size_t size)
{
  return debug_malloc(size, "Smalloc");
}

void *__wrap_realloc(void *m, size_t size)
{
  return debug_realloc(m, size, "Srealloc");
}

void *__wrap_calloc(size_t size,size_t num)
{
  return debug_calloc(size,num,"Scalloc");
}

void __wrap_free(void * size)
{
  return debug_free(size, "Sfree", 0);
}

void *__wrap_strdup(const char *s)
{
  return debug_strdup(s, "Sstrdup");
}
#endif

struct parsed_location
{
  const char *file;
  size_t file_len;
  INT_TYPE line;
  const char *extra;
};

static void parse_location (struct memloc *l, struct parsed_location *pl)
{
  const char *p;
  pl->file = LOCATION_NAME (l->location);

  p = strchr (pl->file, ' ');
  if (p)
    pl->extra = p;
  else
    pl->extra = strchr (pl->file, 0);

  p = strchr (pl->file, ':');
  if (p && p < pl->extra) {
    const char *pp;
    while ((pp = strchr (p + 1, ':')) && pp < pl->extra) p = pp;
    pl->line = strtol (p + 1, NULL, 10);
    pl->file_len = p - pl->file;
  }
  else {
    pl->line = -1;
    pl->file_len = pl->extra - pl->file;
  }
}

static void sort_locations (struct memhdr *hdr)
{
  /* A silly bubble sort, but it shouldn't matter much in this case. */

  struct memloc *end;

  for (end = NULL; hdr->locations != end;) {
    struct memloc **bubble_ptr = &hdr->locations;
    struct memloc *bubble = *bubble_ptr;
    struct memloc *next, *new_end = bubble;
    struct parsed_location parsed_bubble;
    parse_location (bubble, &parsed_bubble);

    for (next = bubble->next; next != end; next = bubble->next) {
      int cmp;
      struct parsed_location parsed_next;
      parse_location (next, &parsed_next);

      cmp = strcmp (parsed_next.extra, parsed_bubble.extra);
      if (!cmp)
	cmp = strncmp (parsed_bubble.file, parsed_next.file,
		       parsed_bubble.file_len > parsed_next.file_len ?
		       parsed_bubble.file_len : parsed_next.file_len);
      if (!cmp)
	cmp = parsed_bubble.line - parsed_next.line;

      if (cmp > 0) {
	bubble->next = next->next;
	*bubble_ptr = next;
	bubble_ptr = &next->next;
	new_end = next->next = bubble;
      }
      else {
	bubble_ptr = &bubble->next;
	bubble = next;
	parsed_bubble = parsed_next;
      }
    }

    end = new_end;
  }
}

void dump_memhdr_locations(struct memhdr *from,
			   struct memhdr *notfrom,
			   int indent)
{
  struct memloc *l;
  if(!from) return;

  fprintf(stderr,"%*sLocations that handled %p: (gc generation: %d/%d  gc pass: %d/%d)\n",
	  indent,"",
	  from->data,
	  from->gc_generation / 1000,
	  gc_generation,
	  from->gc_generation % 1000,
	  Pike_in_gc);

  if (notfrom)
    fprintf (stderr, "%*sIgnoring locations that also handled %p.\n",
	     indent, "", notfrom->data);

  sort_locations (from);

  for(l=from->locations;l;l=l->next)
  {
    if(notfrom && find_location(notfrom, l->location))
      continue;


    fprintf(stderr,"%*s%s %s (%d times)%s\n",
	    indent,"",
	    LOCATION_IS_DYNAMIC(l->location) ? "-->" : "***",
	    LOCATION_NAME(l->location),
	    l->times,
	    find_location(&no_leak_memlocs, l->location) ? "" :
	    ( from->flags & MEM_REFERENCED ? " *" : " !*!")
	    );
    dump_location_bt (l->location, indent + 4, "| ");

    /* Allow linked memhdrs */
/*    dump_memhdr_locations(my_find_memhdr(l,0),notfrom,indent+2); */
  }
}

static void low_dmalloc_describe_location(struct memhdr *mh, int offset, int indent);


static void find_references_to(void *block, int indent, int depth, int flags)
{
  unsigned long h;
  struct memhdr *m;
  int warned=0;

  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
  {
    /* Avoid infinite recursion */
    ptrdiff_t num_to_check=0;
    void *to_check[1000];

    /* FIXME: Why store m->data in to_check, and not m itself?
     *		/grubba 2003-03-16
     *
     * This entire loop seems strange. Why is it split into
     * two parts?
     *
     * Hmm... Could it be that describe_location() can do find_memhdr()?
     */
    for(m=memhdr_hash_table[h];m;m=m->next)
    {
      if(num_to_check >= (ptrdiff_t) NELEM(to_check))
      {
	warned=1;
	fprintf(stderr,"  <We might miss some references!!>\n");
	break;
      }
      to_check[num_to_check++]=m->data;
    }

    while(--num_to_check >= 0)
    {
      unsigned int e;
      struct memhdr *tmp;
      void **p;

      p=to_check[num_to_check];
      m=find_memhdr(p);
      if(!m)
      {
	if(!warned)
	{
	  warned=1;
	  fprintf(stderr,"  <We might miss some references!!>\n");
	}
	continue;
      }

      if( ! ((sizeof(void *)-1) & (long) p ))
      {
	if(m->size > 0)
	{
	  for(e=0;e<m->size/sizeof(void *);e++)
	  {
	    if(p[e] == block)
	    {
/*	      fprintf(stderr,"  <from %p word %d>\n",p,e); */
	      describe_location(p,PIKE_T_UNKNOWN,p+e, indent,depth,flags);

/*	      low_dmalloc_describe_location(m, e * sizeof(void *), indent); */

	      m->flags |= MEM_WARN_ON_FREE;
	    }
	  }
	}
      }
    }
  }

  memheader_references_located=1;
}

void dmalloc_find_references_to(void *block)
{
  mt_lock(&debug_malloc_mutex);
  find_references_to(block, 2, 1, 0);
  mt_unlock(&debug_malloc_mutex);
}

void *dmalloc_find_memblock_base(void *ptr)
{
  unsigned long h;
  struct memhdr *m;
  char *lookfor=(char *)ptr;

  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
  {
    for(m=memhdr_hash_table[h];m;m=m->next)
    {
      unsigned int e;
      struct memhdr *tmp;
      char *p=(char *)m->data;

      if( ! ((sizeof(void *)-1) & (long) p ))
      {
	if( p <= lookfor && lookfor < p + m->size)
	{
	  mt_unlock(&debug_malloc_mutex);
	  return m->data;
	}
      }
    }
  }

  return 0;
}

/* FIXME: lock the mutex */
PMOD_EXPORT void debug_malloc_dump_references(void *x, int indent, int depth, int flags)
{
  struct memhdr *mh=my_find_memhdr(x,0);
  if(!mh) return;

#ifdef DMALLOC_C_STACK_TRACE
  if (mh->alloc_bt_len) {
    int i;
    fprintf (stderr, "%*sStack at allocation:\n", indent, "");
    for (i = 0; i < mh->alloc_bt_len; i++) {
      fprintf (stderr, "%*s| ", indent, "");
      backtrace_symbols_fd (mh->alloc_bt + i, 1, 2);
    }
  }
#endif

  dump_memhdr_locations(mh,0, indent);
  if(memheader_references_located)
  {
    if(mh->flags & MEM_IGNORE_LEAK)
    {
      fprintf(stderr,"%*s<<<This leak has been ignored>>>\n",indent,"");
    }
    else if(mh->flags & MEM_REFERENCED)
    {
      fprintf(stderr,"%*s<<<Possibly referenced>>>\n",indent,"");
      if(!(flags & 2))
	find_references_to(x,indent+2,depth-1,flags);
    }
    else
    {
      fprintf(stderr,"%*s<<<=- No known references to this block -=>>>\n",indent,"");
    }
  }
}

PMOD_EXPORT void list_open_fds(void)
{
  unsigned long h;
  mt_lock(&debug_malloc_mutex);

  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
  {
    struct memhdr *m;
    for(m=memhdr_hash_table[h];m;m=m->next)
    {
      struct memhdr *tmp;
      struct memloc *l;
      void *p=m->data;
      if(m->flags & MEM_FREE) continue;

      if( 1 & (long) p )
      {
	if( FD2PTR( PTR2FD(p) ) == p)
	{
	  fprintf(stderr,"Filedescriptor %ld\n", (long) PTR2FD(p));

	  dump_memhdr_locations(m, 0, 0);
	}
      }
    }
  }
  mt_unlock(&debug_malloc_mutex);
}

static void low_search_all_memheaders_for_references(void)
{
  unsigned long h;
  struct memhdr *m;
  size_t counter = 0;

  if (PIKE_MEM_CHECKER()) return;

  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
    for(m=memhdr_hash_table[h];m;m=m->next)
      m->flags &= ~(MEM_REFERENCED|MEM_SCANNED);

  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
  {
    for(m=memhdr_hash_table[h];m;m=m->next)
    {
      unsigned int e;
      struct memhdr *tmp;
      void **p=m->data;

      if (m->flags & MEM_SCANNED) {
	fprintf(stderr, "Found memhdr %p: (%p) several times in hash-table!\n",
		m, m->data);
	dump_memhdr_locations(m, 0, 0);
	continue;
      }
      m->flags |= MEM_SCANNED;

      counter++;
      if (counter > num_memhdr) {
	Pike_fatal("Found too many memhdrs!\n");
      }
      if( ! ((sizeof(void *)-1) & (size_t)p ))
      {
      }
    }
  }

  memheader_references_located=1;
}

void search_all_memheaders_for_references(void)
{
  mt_lock(&debug_malloc_mutex);
  low_search_all_memheaders_for_references();
  mt_unlock(&debug_malloc_mutex);
}

static void cleanup_memhdrs(void)
{
  unsigned long h;
  mt_lock(&debug_malloc_mutex);
  for(h=0;h<FREE_DELAY;h++)
  {
    void *p;
    if((p=blocks_to_free[h]))
    {
      struct memhdr *mh = find_memhdr(p);
      if(mh)
      {
	PIKE_MEM_RW_RANGE((char *) p - DEBUG_MALLOC_PAD,
			  ~mh->size + 2 * DEBUG_MALLOC_PAD);
	unregister_memhdr(mh,0);
	real_free( ((char *)p) - DEBUG_MALLOC_PAD );
      }else{
	fake_free(p);
      }
      blocks_to_free[h]=0;
    }
  }
  exiting=1;

  if (exit_with_cleanup)
  {
    int first=1;
    low_search_all_memheaders_for_references();

    for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
    {
      struct memhdr *m;
      for(m=memhdr_hash_table[h];m;m=m->next)
      {
	int referenced=0;
	struct memhdr *tmp;
	void *p=m->data;

	if(m->flags & MEM_IGNORE_LEAK) continue;

	mt_unlock(&debug_malloc_mutex);
	if(first)
	{
	  fprintf(stderr,"\n");
	  first=0;
	}

	if(m->flags & MEM_REFERENCED)
	  fprintf(stderr, "possibly referenced memory: (%p) %ld bytes\n",p, m->size);
	else
	  fprintf(stderr, "==LEAK==: (%p) %ld bytes\n",p, m->size);

	if( 1 & (long) p )
	{
	  if( FD2PTR( PTR2FD(p) ) == p)
	  {
	    fprintf(stderr," Filedescriptor %ld\n", (long) PTR2FD(p));
	  }
	}else{
#ifdef PIKE_DEBUG
	  describe_something(p, attempt_to_identify(p, NULL), 0,2,8, NULL);
#endif
	}
	mt_lock(&debug_malloc_mutex);

	/* Now we must reassure 'm' */
	for(tmp=memhdr_hash_table[h];tmp;tmp=tmp->next)
	  if(m==tmp)
	    break;

	if(!tmp)
	{
	  fprintf(stderr,"**BZOT: Memory header was freed in mid-flight.\n");
	  fprintf(stderr,"**BZOT: Restarting hash bin, some entries might be duplicated.\n");
	  h--;
	  break;
	}

	find_references_to(p,0,0,0);
	dump_memhdr_locations(m, 0,0);
      }
    }


#ifdef DMALLOC_PROFILE
    {
      INT32 num,mem;
      long free_memhdr=0, ignored_memhdr=0, total_memhdr=0;

      fprintf(stderr,"add_location %d cache %d (%3.3f%%) new: %d (%3.3f%%)\n",
	      add_location_calls,
	      add_location_cache_hits,
	      add_location_cache_hits*100.0/add_location_calls,
	      add_location_new,
	      add_location_new*100.0/add_location_calls);
      fprintf(stderr,"           seek: %d  %3.7f  duplicates: %d\n",
	      add_location_seek,
	      ((double)add_location_seek)/(add_location_calls-add_location_cache_hits),
	      add_location_duplicate);

      count_memory_in_memhdrs(&num,&mem);
      fprintf(stderr,"memhdrs:  %8ld, %10ld bytes\n",(long)num,(long)mem);

      for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
      {
	struct memhdr *m;
	for(m=memhdr_hash_table[h];m;m=m->next)
	{
	  total_memhdr++;
	  if(m->flags & MEM_FREE)
	    free_memhdr++;
	  else if(m->flags & MEM_IGNORE_LEAK)
	    ignored_memhdr++;
	}
      }
      fprintf(stderr,"memhdrs, hashed %ld, free %ld, ignored %ld, other %ld\n",
	      total_memhdr,
	      free_memhdr,
	      ignored_memhdr,
	      total_memhdr - free_memhdr - ignored_memhdr);
    }
#endif

  }
  mt_unlock(&debug_malloc_mutex);
}

#ifdef _REENTRANT
static void lock_da_lock(void)
{
  mt_lock(&debug_malloc_mutex);
}

static void unlock_da_lock(void)
{
  mt_unlock(&debug_malloc_mutex);
}
#endif

static void initialize_dmalloc(void)
{
  long e;
  static int initialized=0;
  if(!initialized)
  {
    initialized=1;
#ifdef DMALLOC_REMEMBER_LAST_LOCATION
    th_key_create(&dmalloc_last_seen_location, 0);
#endif
    init_memhdr_hash();
    memset(mlhash, 0, sizeof(mlhash));

    for(e=0;e<(long)NELEM(rndbuf);e++) rndbuf[e]= (rand() % 511) | 1;

#if DEBUG_MALLOC_PAD & 3
    fprintf(stderr,"DEBUG_MALLOC_PAD not dividable by four!\n");
    exit(99);
#endif

#ifdef _REENTRANT
#ifdef mt_init_recursive
    mt_init_recursive(&debug_malloc_mutex);
#else
    mt_init(&debug_malloc_mutex);
#endif
    pike_atfatal(debug_malloc_atfatal);

    th_atfork(lock_da_lock, unlock_da_lock,  unlock_da_lock);
#endif
#ifdef DMALLOC_USING_DLOPEN
    {
      real_free=dlsym(RTLD_NEXT,"free");
      real_realloc=dlsym(RTLD_NEXT,"realloc");
      real_malloc=dlsym(RTLD_NEXT,"malloc");
      real_calloc=dlsym(RTLD_NEXT,"calloc");
    }
#endif
  }
}

PMOD_EXPORT void * debug_malloc_update_location(void *p,LOCATION location)
{
  if(p)
  {
    struct memhdr *mh;
    mt_lock(&debug_malloc_mutex);
#ifdef DMALLOC_REMEMBER_LAST_LOCATION
    th_setspecific(dmalloc_last_seen_location, location);
#endif
    if((mh=my_find_memhdr(p,0)))
      add_location(mh, location);

    mt_unlock(&debug_malloc_mutex);
  }
  return p;
}

PMOD_EXPORT void * debug_malloc_update_location_ptr(void *p,
						    ptrdiff_t offset,
						    LOCATION location)
{
  if(p)
    debug_malloc_update_location(*(void **)(((char *)p)+offset), location);
  return p;
}


/* another shared-string table... */
struct dmalloc_string
{
  struct dmalloc_string *next;
  unsigned long hval;
  char str[1];
};

static struct dmalloc_string *dstrhash[DSTRHSIZE];

static LOCATION low_dynamic_location(char type, const char *file,
				     INT_TYPE line,
				     const char *name,
				     const unsigned char *bin_data,
				     unsigned int bin_data_len)
{
  struct dmalloc_string **prev, *str;
  size_t len=strlen(file), name_len;
  unsigned long h,hval=hashmem((const unsigned char *) file,len,64)+line;

  if (name) {
    name_len = strlen (name);
    hval += hashmem ((const unsigned char *) name, name_len, 64);
  }

  if (bin_data) hval += hashmem (bin_data, bin_data_len, 64);

  h=hval % DSTRHSIZE;

  mt_lock(&debug_malloc_mutex);

  for(prev = dstrhash + h; (str=*prev); prev = &str->next)
  {
    if(hval == str->hval &&
       !strncmp(str->str+1, file, len) &&
       str->str[len+1]==':' &&
       LOCATION_TYPE (str->str) == type &&
       strtol(str->str+len+2, NULL, 10) == line)
    {

      if (name) {
	char *s = strchr (str->str + len + 2, ' ');
	if (!s) continue;
	s++;
	if (strcmp (s, name)) continue;
      }

      if (bin_data) {
	/* Can assume that str also carries a bin_data blob since it
	 * has the same type. */
	unsigned char *str_bin_base =
	  (unsigned char *) str->str + len + strlen (str->str + len) + 1;
        unsigned int str_bin_len = get_unaligned16 (str_bin_base);
	str_bin_base += 2;
	if (str_bin_len != bin_data_len ||
	    memcmp (bin_data, str_bin_base, str_bin_len))
	  continue;
      }

      *prev=str->next;
      str->next=dstrhash[h];
      dstrhash[h]=str;
      break;
    }
  }

  if(!str)
  {
    char line_str[30];
    size_t l;

    sprintf (line_str, "%ld", (long)line);
    l = len + strlen (line_str) + 2;
    if (name) l += name_len + 1;
    str=malloc (sizeof (struct dmalloc_string) + l +
		(bin_data ? bin_data_len + 2 : 0));

    if (name)
      sprintf(str->str, "%c%s:%s %s", type, file, line_str, name);
    else
      sprintf(str->str, "%c%s:%s", type, file, line_str);

    if (bin_data) {
      unsigned INT16 bl = (unsigned INT16) bin_data_len;
      if (bl != bin_data_len)
	Pike_fatal ("Too long bin_data blob: %u\n", bin_data_len);
      ((unsigned char *) str->str)[l + 1] = ((unsigned char *) &bl)[0];
      ((unsigned char *) str->str)[l + 2] = ((unsigned char *) &bl)[1];
      memcpy (str->str + l + 3, bin_data, bin_data_len);
    }

    str->hval=hval;
    str->next=dstrhash[h];
    dstrhash[h]=str;
  }

  mt_unlock(&debug_malloc_mutex);

  return str->str;
}

PMOD_EXPORT LOCATION dynamic_location(const char *file, INT_TYPE line)
{
  return low_dynamic_location('D',file,line, NULL, NULL, 0);
}


PMOD_EXPORT void * debug_malloc_name(void *p,const char *file, INT_TYPE line)
{
  if(p)
  {
    struct memhdr *mh;
    LOCATION loc=dynamic_location(file, line);

    mt_lock(&debug_malloc_mutex);

    if((mh=my_find_memhdr(p,0)))
      add_location(mh, loc);

    mt_unlock(&debug_malloc_mutex);
  }
  return p;
}

/*
 * This copies all dynamic locations from
 * one pointer to another. Used by clone() to copy
 * the name(s) of the program.
 */
PMOD_EXPORT int debug_malloc_copy_names(void *p, void *p2)
{
  int names=0;
  if(p)
  {
    struct memhdr *mh,*from;
    mt_lock(&debug_malloc_mutex);

    if((from=my_find_memhdr(p2,0)) && (mh=my_find_memhdr(p,0)))
    {
      struct memloc *l;
      for(l=from->locations;l;l=l->next)
      {
	if(LOCATION_IS_DYNAMIC(l->location))
	{
	  add_location(mh, l->location);
	  names++;
	}
      }
    }

    mt_unlock(&debug_malloc_mutex);
  }
  return names;
}

const char *dmalloc_find_name(void *p)
{
  const char *name=0;
  if(p)
  {
    struct memhdr *mh;
    mt_lock(&debug_malloc_mutex);

    if((mh=my_find_memhdr(p,0)))
    {
      struct memloc *l;
      for(l=mh->locations;l;l=l->next)
      {
	if(LOCATION_IS_DYNAMIC(l->location))
	{
	  name=LOCATION_NAME(l->location);
	  break;
	}
      }
    }

    mt_unlock(&debug_malloc_mutex);
  }
  return name;
}

PMOD_EXPORT void *debug_malloc_update_location_bt (void *p, const char *file,
						   INT_TYPE line,
						   const char *name)
{
  LOCATION l;
#ifdef DMALLOC_C_STACK_TRACE
  c_stack_frame bt[BT_MAX_FRAMES];
  int n = backtrace (bt, BT_MAX_FRAMES);
  if (n > 1)
    l = low_dynamic_location ('B', file, line, name,
			      /* Shave off one entry for our own frame. */
			      (unsigned char *) (bt + 1),
			      (n - 1) * sizeof (c_stack_frame));
  else
#endif
    l = low_dynamic_location ('D', file, line, name, NULL, 0);
  return debug_malloc_update_location (p, l);
}

#ifdef DMALLOC_C_STACK_TRACE
static void dump_location_bt (LOCATION location, int indent, const char *prefix)
{
  if (LOCATION_TYPE (location) == 'B') {
    c_stack_frame bt[BT_MAX_FRAMES];
    int i, frames;
    unsigned char *bin_base =
      (unsigned char *) location + strlen (location) + 1;
    unsigned int bin_len = get_unaligned16 (bin_base);
    memcpy (bt, bin_base + 2, bin_len);
    frames = bin_len / sizeof (c_stack_frame);

    for (i = 0; i < frames; i++) {
      fprintf (stderr, "%*s%s", indent, "", prefix);
      backtrace_symbols_fd (bt + i, 1, 2);
    }
  }
}
#else
static void dump_location_bt (LOCATION UNUSED(location), int UNUSED(indent),
                              const char *UNUSED(prefix))
{ }
#endif

PMOD_EXPORT int debug_malloc_touch_fd(int fd, LOCATION location)
{
  if(fd==-1) return fd;
  debug_malloc_update_location( FD2PTR(fd), location);
  return fd;
}

PMOD_EXPORT int debug_malloc_register_fd(int fd, LOCATION location)
{
  if(fd==-1) return fd;
  dmalloc_unregister( FD2PTR(fd), 1);
  dmalloc_register( FD2PTR(fd), 0, location);
  return fd;
}

PMOD_EXPORT void debug_malloc_accept_leak_fd(int fd)
{
  dmalloc_accept_leak(FD2PTR(fd));
}

PMOD_EXPORT int debug_malloc_close_fd(int fd, LOCATION UNUSED(location))
{
  if(fd==-1) return fd;
#ifdef DMALLOC_TRACK_FREE
  dmalloc_mark_as_free( FD2PTR(fd), 1 );
#else /* !DMALLOC_TRACK_FREE */
  remove_memhdr(FD2PTR(fd));
#endif /* DMALLOC_TRACK_FREE */
  return fd;
}

void debug_malloc_dump_fd(int fd)
{
  fprintf(stderr,"DMALLOC dumping locations for fd %d\n",fd);
  if(fd==-1) return;
  debug_malloc_dump_references(FD2PTR(fd),2,0,0);
}

void reset_debug_malloc(void)
{
  unsigned long h;
  for(h=0;h<(unsigned long)memhdr_hash_table_size;h++)
  {
    struct memhdr *m;
    for(m=memhdr_hash_table[h];m;m=m->next)
    {
      struct memloc *l;
      for(l=m->locations;l;l=l->next)
      {
	l->times=0;
      }
    }
  }
}

struct memory_map
{
  char name[128];
  INT32 refs;
  struct memory_map *next;
  struct memory_map_entry *entries;
};

struct memory_map_entry
{
  struct memory_map_entry *next;
  char name[128];
  int offset;
  int size;
  int count;
  int recur_offset;
  struct memory_map *recur;
};

static struct block_allocator memory_map_allocator = BA_INIT_PAGES(sizeof(struct memory_map), 8);

static struct memory_map * alloc_memory_map() {
    return ba_alloc(&memory_map_allocator);
}

void count_memory_in_memory_maps(size_t * n, size_t * s) {
    ba_count_all(&memory_map_allocator, n, s);
}

static struct block_allocator memory_map_entry_allocator
    = BA_INIT_PAGES(sizeof(struct memory_map_entry), 16);

static struct memory_map_entry * alloc_memory_map_entry() {
    return ba_alloc(&memory_map_entry_allocator);
}

void count_memory_in_memory_map_entrys(size_t * n, size_t * s) {
    ba_count_all(&memory_map_entry_allocator, n, s);
}

void dmalloc_set_mmap(void *ptr, struct memory_map *m)
{
  m->refs++;
  debug_malloc_update_location(ptr, m->name+1);
}

void dmalloc_set_mmap_template(void *ptr, struct memory_map *m)
{
  m->refs++;
  debug_malloc_update_location(ptr,m->name);
}

void dmalloc_set_mmap_from_template(void *p, void *p2)
{
  int names=0;
  if(p)
  {
    struct memhdr *mh,*from;
    mt_lock(&debug_malloc_mutex);

    if((from=my_find_memhdr(p2,0)) && (mh=my_find_memhdr(p,0)))
    {
      struct memloc *l;
      for(l=from->locations;l;l=l->next)
      {
	if(LOCATION_TYPE (l->location) == 'T')
	{
	  add_location(mh, l->location+1);
	  names++;
	}
      }
    }

    mt_unlock(&debug_malloc_mutex);
  }
}

static void very_low_dmalloc_describe_location(struct memory_map *m,
					       int offset,
					       int indent)
{
  struct memory_map_entry *e;
  fprintf(stderr,"%*s ** In memory description %s:\n",indent,"",m->name+2);
  for(e=m->entries;e;e=e->next)
  {
    if(e->offset <= offset && e->offset + e->size * e->count > offset)
    {
      int num=(offset - e->offset)/e->size;
      int off=offset-e->size*num-e->offset;
      fprintf(stderr,"%*s    Found in member: %s[%d] + %d (offset=%d size=%d)\n",
	      indent,"",
	      e->name,
	      num,
	      off,
	      e->offset,
	      e->size);

      if(e->recur)
      {
	very_low_dmalloc_describe_location(e->recur,
					   off-e->recur_offset,
					   indent+2);
      }
    }
  }
}

static void low_dmalloc_describe_location(struct memhdr *mh, int offset, int indent)
{
  struct memloc *l;
  for(l=mh->locations;l;l=l->next)
  {
    if(LOCATION_TYPE (l->location) == 'M')
    {
      struct memory_map *m = (struct memory_map *)(l->location - 1);
      very_low_dmalloc_describe_location(m, offset, indent);
    }
  }
}

void dmalloc_describe_location(void *p, int offset, int indent)
{
  if(p)
  {
    struct memhdr *mh;

    if((mh=my_find_memhdr(p,0)))
      low_dmalloc_describe_location(mh, offset, indent);
  }
}

struct memory_map *dmalloc_alloc_mmap(char *name, INT_TYPE line)
{
  struct memory_map *m;
  mt_lock(&debug_malloc_mutex);
  m=alloc_memory_map();
  strncpy(m->name+2,name,sizeof(m->name)-2);
  m->name[sizeof(m->name)-1]=0;
  m->name[0]='T';
  m->name[1]='M';
  m->refs=0;

  if(strlen(m->name)+12<sizeof(m->name))
    sprintf(m->name+strlen(m->name), ":%ld", (long)line);

  m->entries=0;
  mt_unlock(&debug_malloc_mutex);
  return m;
}

void dmalloc_add_mmap_entry(struct memory_map *m,
			    char *name,
			    int offset,
			    int size,
			    int count,
			    struct memory_map *recur,
			    int recur_offset)
{
  struct memory_map_entry *e;
  mt_lock(&debug_malloc_mutex);
  e=alloc_memory_map_entry();
  strncpy(e->name,name,sizeof(e->name));
  e->name[sizeof(e->name)-1]=0;
  e->offset=offset;
  e->size=size;
  e->count=count?count:1;
  e->next=m->entries;
  e->recur=recur;
  e->recur_offset=recur_offset;
  m->entries=e;
  mt_unlock(&debug_malloc_mutex);
}

int dmalloc_is_invalid_memory_block(void *block)
{
  struct memhdr *mh=my_find_memhdr(block,1);
  if(!mh) return -1; /* no such known block */
  if(mh->size < 0) return -2; /* block has been freed */
  return 0; /* block is valid */
}

static void cleanup_debug_malloc(void)
{
  size_t i;

  ba_destroy(&memloc_allocator);
  ba_destroy(&memory_map_allocator);
  exit_memhdr_hash();

  for (i = 0; i < DSTRHSIZE; i++) {
    struct dmalloc_string *str = dstrhash[i], *next;
    while (str) {
      next = str->next;
      free(str);
      str = next;
    }
  }

  mt_destroy(&debug_malloc_mutex);
}

#endif	/* DEBUG_MALLOC */

#if defined(__GNUC__) && defined(__SSE__) && defined(HAVE_EMMINTRIN_H)
#include <emmintrin.h>
#define SSE2
#endif

static inline void low_zero(void *p, size_t n)
{
    volatile char * _p = (char *)p;
    while (n--) *_p++ = 0;
}

PMOD_EXPORT void secure_zero(void *p, size_t n)
{
#ifdef SSE2
  if( n > 256 )
  {
    char *ptr = (char *)p;
    char *end = ptr + n;
    int left = ((long)ptr) & 63;

    if( left )
    {
      low_zero(ptr, left);
      ptr += left;
    }

    /* Clear memory without evicting CPU cache. */
    for( ; ptr <= (end-64); ptr += 64 )
    {
      __m128i i = _mm_set_epi8(0, 0, 0, 0,
                               0, 0, 0, 0,
                               0, 0, 0, 0,
                               0, 0, 0, 0);
      _mm_stream_si128((__m128i *)&ptr[0], i);
      _mm_stream_si128((__m128i *)&ptr[16], i);
      _mm_stream_si128((__m128i *)&ptr[32], i);
      _mm_stream_si128((__m128i *)&ptr[48], i);
    }

    if( end-ptr )
      low_zero(ptr, end-ptr);
  }
  else
#endif
    low_zero(p, n);
}

void init_pike_memory (void)
{
  init_hashmem();
#ifdef HAVE_GETPAGESIZE
  page_size = getpagesize();
#elif defined (HAVE_SYSCONF) && defined (_SC_PAGESIZE)
  page_size = (int) sysconf (_SC_PAGESIZE);
#elif defined (HAVE_SYSCONF) && defined (_SC_PAGE_SIZE)
  page_size = (int) sysconf (_SC_PAGE_SIZE);
#elif defined (HAVE_GETSYSTEMINFO)
  {
    SYSTEM_INFO sysinfo;
    GetSystemInfo (&sysinfo);
    page_size = sysinfo.dwPageSize;
  }
#else
  /* A reasonable default... */
  page_size = 8192;
#endif /* HAVE_GETPAGESIZE */

#ifdef INIT_DEV_ZERO
  /* Neither MAP_ANONYMOUS nor MAP_ANON.
   * Initialize a /dev/zero fd for use with mmap.
   */
  if (dev_zero < 0) {
    if ((dev_zero = open("/dev/zero", O_RDONLY)) < 0) {
      fprintf(stderr, "Failed to open /dev/zero.\n");
      return NULL;
    }
    dmalloc_accept_leak_fd(dev_zero);
    set_close_on_exec(dev_zero, 1);
  }
#endif

#ifdef DEBUG_MALLOC
  initialize_dmalloc();
#endif
}

void exit_pike_memory (void)
{
#ifdef DEBUG_MALLOC
  cleanup_memhdrs();
  cleanup_debug_malloc();
#endif

#if defined (USE_MY_MEXEC_ALLOC) && defined (MY_MEXEC_ALLOC_STATS)
  fprintf (stderr, "mexec stats: "
	   "%"PRINTSIZET"u separate allocs, %"PRINTSIZET"u grow allocs, "
	   "%"PRINTSIZET"u bytes, alloc chunks %d bytes.\n",
	   sep_allocs, grow_allocs, total_size, MEXEC_ALLOC_CHUNK_SIZE);
#endif
}
