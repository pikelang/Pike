#include <sys/types.h>

#ifdef HAVE_THREAD_H
# include <thread.h>
#else
# ifdef HAVE_PTHREAD_H
#  include <pthread.h>
# endif
#endif

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#else
#ifdef HAVE_LINUX_MMAN_H
# include <linux/mman.h>
#endif
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
 /*is this correct on all systems that lack MAP_FAILED?*/
#endif

#include "sharedmem.h"

#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_MMAP
#include <unistd.h>

#undef SHARED_MALLOC_DEBUG

#ifdef SHARED_MALLOC_DEBUG
/* Force realloc often. */
# define CHUNK 1<<8
#else
# define CHUNK 1<<16
#endif

static struct shared_memory_chunk {
  char *p;
  size_t len;
  size_t left;
  int refs;
  struct shared_memory_chunk *next, *prev;
} *shmem_head = NULL;

static void new_shared_chunk(int len)
{
  int fd;
  void *pointer;
  char fname[100];
  struct shared_memory_chunk *foo;

#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "new_shared_chunk(%d)\n", len);
#endif

  sprintf(fname, "/tmp/.spinnerlock_%d", (int)getuid());
  unlink(fname);
  fd = open(fname, O_RDWR|O_CREAT, 0777);
  unlink(fname);

  lseek(fd, len, SEEK_SET);
  write(fd, &fd, 2);
  lseek(fd, 0, SEEK_SET);

  if(fd == -1)
  {
    perror("shared_malloc::open");
    error("shared_malloc(): Can't open() /tmp/.spinnerlock.\n");
  }

  pointer = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  if((long)pointer==(long)MAP_FAILED)
  {
    perror("shared_malloc::mmap");
    error("shared_malloc(): Can't mmap() /tmp/.spinnerlock.\n");
  }
  close(fd);
  foo = (void *)calloc(sizeof(struct shared_memory_chunk), 1);
  foo->len = len-1;
  foo->left = len-1;
  foo->next = shmem_head;
  if(shmem_head)
    shmem_head->prev = foo;
  foo->prev = NULL;
  foo->p = pointer;
  shmem_head = foo;
#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "return %p;\n", foo);
#endif
}

static void free_shared_chunk(struct shared_memory_chunk *which)
{
#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "free_shared_chunk(%p)\n", which);
#endif
  if(which->refs)
    error("Freeing chunk with refs!\n");
  munmap(which->p, which->len);
  if(which->next)
    which->next->prev = which->prev;
  if(which->prev)
    which->prev->next = which->next;
  if(which == shmem_head)
    shmem_head = which->next;
  free(which);
}


#ifdef SHARED_MALLOC_DEBUG
static void debug_print_shmem_head()
{
  fprintf(stderr, "shmem_head(%p): len=%x; left=%x; base=%p\n", 
	  shmem_head, shmem_head->len, shmem_head->left, shmem_head->p);
}
#endif

void *shared_malloc(size_t size)
{
#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "shared_malloc(%d <%d>)\n", size, (size/8+1)*8);
#endif
  size = (size/8+1)*8; /* Align. 8 bytes should work on all CPUs. */
  if(size > CHUNK)
    new_shared_chunk((size/CHUNK+1)*CHUNK);

  if(!shmem_head || (shmem_head->left < size))
    new_shared_chunk(CHUNK);

  shmem_head->refs++;
  shmem_head->left -= size;
#ifdef SHARED_MALLOC_DEBUG
  debug_print_shmem_head();
  fprintf(stderr, "return %p;\n", shmem_head->p + shmem_head->len - shmem_head->left);
#endif
  return shmem_head->p + shmem_head->len - shmem_head->left - size;
}

void shared_free(char *pointer)
{
  struct shared_memory_chunk *which = shmem_head;

#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "shared_free(%p)\n", pointer);
#endif
  while(which)
  {
    if(which->p <= pointer && pointer < (which->p + which->len))
      break;
    which = which->next;
  }

  if(!which)
  {
    error("shared_free(): Cannot find chunk to free from!\n");
  }

  if(!--which->refs)
    free_shared_chunk(which);
}

#else
/* Should look for shmop as well, probably */
void *shared_malloc(size_t size)
{
#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "fake_shared_malloc(%d)\n", size);
#endif
  return (void *)malloc(size); /* Incorrect, really, since this is not
			        * shared _at_all_ */
}

void shared_free(char *pointer)
{
#ifdef SHARED_MALLOC_DEBUG
  fprintf(stderr, "fake_shared_free(%p)\n", pointer);
#endif
  free(pointer);
}
#endif
