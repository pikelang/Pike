#include "global.h"
#include "types.h"
#include "memory.h"

#define MARKER_CHUNK_SIZE 4096
#define REHASH_LIMIT 16
#define REHASH_FORMULA(X) ((X)*4-1)

struct marker
{
  struct marker *next;
  void *marked;
  INT32 refs;
};

struct marker_chunk
{
  struct marker_chunk *next;
  struct marker markers[MARKER_CHUNK_SIZE];
};

static struct marker_chunk *chunk=0;
static int markers_left_in_chunk=0;

static struct marker *new_marker()
{
  if(!markers_left_in_chunk)
  {
    struct marker_chunk *m;
    m=(struct marker_chunk *)xalloc(sizeof(struct marker_chunk));
    m->next=chunk;
    chunk=m;
    markers_left_in_chunk=MARKER_CHUNK_SIZE;
  }
  markers_left_in_chunk--;

  return chunk->markers + markers_left_in_chunk;
}

static struct marker **hash=0;
static int hashsize=0;
static int hashed=0;

INT32 checked(void *a,INT32 delta)
{
  int hashval;
  struct marker *m;

  if(!hash) return 0;

  hashval=((long)a)%hashsize;

  for(m=hash[hashval];m;m=m->next)
  {
    if(m->marked == a)
    {
      m->refs+=delta;
      return m->refs;
    }
  }
  if(!delta) return 0;

  m=new_marker();
  m->marked=a;
  m->next=hash[hashval];
  m->refs=delta;
  hash[hashval]=m;

  hashed++;
  if(hashed / REHASH_LIMIT > hashsize)
  {
    struct marker **new_hash,*next;
    int new_hashsize;
    int e;

    new_hashsize=REHASH_FORMULA(hashsize);
    new_hash=(struct marker **)xalloc(sizeof(struct marker **)*new_hashsize);
    memset((char *)new_hash,0,sizeof(struct marker **)*new_hashsize);

    for(e=0;e<hashsize;e++)
    {
      for(m=hash[e];m;m=next)
      {
	next=m->next;
	m->next=new_hash[((long)m->marked)%new_hashsize];
	new_hash[((long)m->marked)%new_hashsize]=m;
      }
    }

    free((char *)hash);
    hash=new_hash;
    hashsize=new_hashsize;
  }

  return m->refs;
}

void init_checked()
{
  /* init hash*/
  hashsize=4711;
  hashed=0;
  hash=(struct marker **)xalloc(sizeof(struct marker **)*hashsize);
  memset((char *)hash,0,sizeof(struct marker **)*hashsize);
  markers_left_in_chunk=0;
}

void exit_checked()
{
  struct marker_chunk *m;

  if(!hash) return;
  free((char *)hash);
  while(m=chunk)
  {
    chunk=m->next;
    free((char *)m);
  }
  hash=0;
}
