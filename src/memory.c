/*\
||| This file a part of uLPC, and is copyright by Fredrik Hubinette
||| uLPC is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
#include "memory.h"
#include "error.h"

char *xalloc(SIZE_T size)
{
  char *ret;
  if(!size) return 0;

  ret=(char *)malloc(size);
  if(ret) return ret;

  error("Out of memory.\n");
  return 0;
}

#if 0
/*
 * This function may NOT change 'order'
 * This function is probably too slow, but it does not use
 * any extra space, so it could still be used if no extra space
 * could be allocated.
 */
void reorder(char *memory,INT32 nitems,INT32 size,INT32 *order)
{
  INT32 e, w;

  switch(size)
  {
  case 4:
    for(e=0; e<nitems-1; e++)
    {
      INT32 tmp;
      for(w = order[e]; w < e; w = order[w]);
      if(w == e) continue;
      tmp=((INT32 *)memory)[e];
      ((INT32 *)memory)[e]=((INT32 *)memory)[w];
      ((INT32 *)memory)[w]=tmp;
    }
    break;

  case 8:
    for(e=0; e<nitems-1; e++)
    {
      typedef struct eight_bytes { char t[8]; } eight_bytes;
      eight_bytes tmp;
      for(w = order[e]; w < e; w = order[w]);
      if(w == e) continue;
      tmp=((eight_bytes *)memory)[e];
      ((eight_bytes *)memory)[e]=((eight_bytes *)memory)[w];
      ((eight_bytes *)memory)[w]=tmp;
    }
    break;

  default:
    for(e=0; e<nitems-1; e++)
    {
      INT32 z;
      for(w = order[e]; w < e; w = order[w]);
      if(w == e) continue;
      
      for(z=0; z<size; z++)
      {
	char tmp;
	tmp=memory[e*size+z];
	memory[e*size+z]=memory[w*size+z];
	memory[e*size+z]=tmp;
      }
    }
  }
}

#else
/*
 * This function may NOT change 'order'
 * This function is probably too slow, but it does not use
 * any extra space, so it could still be used if no extra space
 * could be allocated.
 * (actually, it _does_ change 'order' but restores it afterwards)
 */
void reorder(char *memory, INT32 nitems, INT32 size,INT32 *order)
{
  INT32 e,d,c;

#ifdef DEBUG
  for(e=0;e<nitems;e++)
  {
    for(d=0;d<nitems;d++) if(order[d]==e) break;
    if(d==nitems)
      fatal("Missing number %ld in reorder() (nitems = %ld)\n",
	    (long)e,
	    (long)nitems);
  }
#endif


  switch(size)
  {
#ifdef DEBUG
    case 0:
      fatal("FEL FEL FEL\n");
      break;
#endif

    case 1:
    {
      char a,*m;
      m=memory;

      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          a=memory[c=e];
          do
	  {
	    c=order[d=c];
	    m[d]=m[c];
	    order[d]=~c;
	  } while(c!=e);
          memory[d]=a;
        }
        order[e] =~ order[e];
      }
      break;
    }

    case 2:
    {
      typedef struct TMP2 { char t[2]; } tmp2;
      tmp2 a,*m;
      m=(tmp2 *)memory;
      
      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          a=m[c=e];
          do
	  {
	    c=order[d=c];
	    m[d]=m[c];
	    order[d]=~c;
	  }
	  while(c!=e);
          m[d]=a;
        }
        order[e] =~ order[e];
      }
      break;
    }

    case 4:
    {
      typedef struct TMP4 { char t[4]; } tmp4;
      tmp4 a,*m;
      m=(tmp4 *)memory;
      
      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          a=m[c=e];
          do
	  {
	    c=order[d=c];
	    m[d]=m[c];
	    order[d]=~c;
	  } while(c!=e);
          m[d]=a;
        }
        order[e] =~ order[e];
      }
      break;
    }

    case 8:
    {
      typedef struct TMP8 { char t[8]; } tmp8;
      tmp8 a,*m;
      m=(tmp8 *)memory;
      
      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          a=m[c=e];
          do {
            c=order[d=c];
            m[d]=m[c];
            order[d]= ~c;
          }while(c!=e);
    
          m[d]=a;
        }
        order[e] =~ order[e];
      }
      break;
    }

    case 16:
    {
      typedef struct TMP16 { char t[16]; } tmp16;
      tmp16 a,*m;
      m=(tmp16 *)memory;
      
      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          a=m[c=e];
          do {
            c=order[d=c];
            m[d]=m[c];
            order[d]= ~c;
          }while(c!=e);
    
          m[d]=a;
        }
        order[e] =~ order[e];
      }
      break;
    }

    default:
    {
      char *a;
    
      a=(char *)alloca(size);
    
      for(e=0;e<nitems;e++)
      {
        if(order[e]==e) continue;
        if(order[e]>=0)
        {
          MEMCPY(a, memory+e*size, size);
    
          c=e;
          do {
            c=order[d=c];
             MEMCPY(memory+d*size, memory+c*size, size);
            order[d]= ~c;
          }while(d!=e);
    
          MEMCPY(memory+d*size, a, size);
        }
        order[e] =~ order[e];
      }
    }
  }
}

#endif

unsigned INT32 hashmem(const unsigned char *a,INT32 len,INT32 mlen)
{
  unsigned INT32 ret;

  ret=9248339*len;
  if(len<mlen) mlen=len;
  switch(mlen&7)
  {
    case 7: ret^=*(a++);
    case 6: ret^=(ret<<4)+*(a++);
    case 5: ret^=(ret<<7)+*(a++);
    case 4: ret^=(ret<<6)+*(a++);
    case 3: ret^=(ret<<3)+*(a++);
    case 2: ret^=(ret<<7)+*(a++);
    case 1: ret^=(ret<<5)+*(a++);
  }

#ifdef HANDLES_UNALIGNED_MEMORY_ACCESS
  {
    unsigned int *b;
    b=(unsigned int *)a;

    for(mlen>>=3;--mlen>=0;)
    {
      ret^=(ret<<7)+*(b++);
      ret^=(ret>>6)+*(b++);
    }
  }
#else
  for(mlen>>=3;--mlen>=0;)
  {
    ret^=(ret<<7)+((((((*(a++)<<3)+*(a++))<<4)+*(a++))<<5)+*(a++));
    ret^=(ret>>6)+((((((*(a++)<<3)+*(a++))<<4)+*(a++))<<5)+*(a++));
  }
#endif
  return ret;
}

unsigned INT32 hashstr(const unsigned char *str,INT32 maxn)
{
  unsigned INT32 ret,c;
  
  ret=str++[0];
  for(; maxn>=0; maxn--)
  {
    c=str++[0];
    if(!c) break;
    ret ^= ( ret << 4 ) + c ;
    ret &= 0x7fffffff;
  }

  return ret;
}
