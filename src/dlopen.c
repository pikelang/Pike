/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include <global.h>
#include "fdlib.h"
#define DL_INTERNAL
#include "pike_dlfcn.h"
#include "pike_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <windows.h>
#include <memory.h>
#include <sys/stat.h>
#include <assert.h>


#define DLDEBUG 1
#define DL_VERBOSE 1

#ifdef DLDEBUG
#define DO_IF_DLDEBUG(X) X
#else
#define DO_IF_DLDEBUG(X)
#endif

#ifndef PIKE_CONCAT

#include <io.h>
#include <fcntl.h>

#define INT8 char
#define INT16 short
#define INT32 int
#define ptrdiff_t long
#define STRTOL strtol

#define RTLD_GLOBAL 1
#define RTLD_LAZY 0 /* never */
#define RTLD_NOW 0 /* always */

#define fd_open open
#define fd_close close
#define fd_read read
#define fd_RDONLY O_RDONLY
#define fd_BINARY O_BINARY

long hashmem(char *mem, ptrdiff_t size)
{
  long ret=size * 9278234247;
  while(--size>=0) ret+=(ret<<4) + *(mem++);
  return ret;
}

size_t STRNLEN(char *s, size_t maxlen)
{
  char *tmp=memchr(s,0,maxlen);
  if(tmp) return tmp-s;
  return maxlen;
}

#endif

/****************************************************************/

struct Sym
{
  struct Sym *next;
  void *sym;
  size_t len;
  char name[1];
};

struct Htable
{
  size_t size; /* size of hash table */
  size_t entries;
  struct Sym *symbols[1];
};

static struct Htable *alloc_htable(size_t size)
{
  int e;
  struct Htable *ret;
#ifdef DLDEBUG
  fprintf(stderr,"alloc_htable(%d)\n",size);
#endif
  ret=(struct Htable *)malloc(sizeof(struct Htable) +
			      sizeof(void *)*(size-1));
  ret->size=size;
  ret->entries=0;
  for(e=0;e<size;e++) ret->symbols[e]=0;
  return ret;
}

static struct Htable *htable_add_space(struct Htable *h,
				       size_t extra_space)
{
#if 0
  if(h->entries+extra_space > h->size)
  {
    size_t new_size = (h->size * 2)+1;
    if(h->entries+extra_space > new_size)
      new_size = (h->entries+extra_space) | 0x10101;
    /** rehashto new_size */

  }
#endif
  return h;
}


static struct Sym *htable_get_low(struct Htable *h,
				  char *name,
				  size_t len,
				  size_t hval)
{
  struct Sym *tmp;

#ifdef DLDEBUG
  if(name[len])
    fprintf(stderr,"htable_get_low(%c%c%c%c%c%c%c%c) len=%d hval=%x/%x\n",
	    name[0],
	    name[1],
	    name[2],
	    name[3],
	    name[4],
	    name[5],
	    name[6],
	    name[7],
	    len,
	    hval,h->size);
  else
    fprintf(stderr,"htable_get_low(%s) len=%d hval=%x/%x\n",name,len,
	    hval,h->size);
#endif

  for(tmp=h->symbols[hval];tmp;tmp=tmp->next)
  {
    
/* fprintf(stderr,"Looking %s/%d =? %s/%d\n",name,len, tmp->name,tmp->len); */
    if(tmp->len == len && !memcmp(name, tmp->name, len))
    {
      return tmp;
    }
  }
  return 0;
}

static void *htable_get(struct Htable *h, char *name, size_t l)
{
  struct Sym * tmp;
  size_t hval;
  DO_HASHMEM(hval, name, l, 128);

  if((tmp=htable_get_low(h, name, l, hval % h->size)))
    return tmp->sym;
  return 0;
}

static void htable_put(struct Htable *h, char *name, size_t l, void *ptr)
{
  struct Sym *tmp;

  size_t hval;
  DO_HASHMEM(hval, name, l, 128);
  hval %= h->size;

#ifdef DLDEBUG
  if(name[l])
  {
    fprintf(stderr,"DL: htable_put(%c%c%c%c%c%c%c%c,%p)\n",
	    name[0],
	    name[1],
	    name[2],
	    name[3],
	    name[4],
	    name[5],
	    name[6],
	    name[7],
	    ptr);
  }else{
    fprintf(stderr,"DL: htable_put(%s,%p)\n",name,ptr);
  }
#endif

  /* FIXME: Should be duplicate symbols be overloaded or what? */
  if((tmp=htable_get_low(h, name, l, hval)))
  {
    tmp->sym=ptr;
    return;
  }
  tmp=(struct Sym *)malloc(sizeof(struct Sym) + l);
  if(!tmp)
  {
    /* out of memory */
    exit(1);
  }
  memcpy(tmp->name, name, l);
  tmp->name[l]=0;
  tmp->len=l;
  tmp->next=h->symbols[hval];
  tmp->sym=ptr;
  h->symbols[hval]=tmp;
  h->entries++;
}

static void htable_free(struct Htable *h, void(*hfree)(void *))
{
  size_t hval;
  struct Sym *tmp,*next;
  if(hfree)
  {
    for(hval=0; hval<h->size; hval++)
    {
      for(next=h->symbols[hval];tmp=next;)
      {
	next=tmp->next;
	hfree(tmp->sym);
	free((char *)tmp);
      }
    }
  }else{
    for(hval=0; hval<h->size; hval++)
    {
      for(next=h->symbols[hval];tmp=next;)
      {
	next=tmp->next;
	free((char *)tmp);
      }
    }
  }
  free((char *)h);
}

/****************************************************************/

static int filesize(char *filename)
{
  struct stat st;
  if(stat(filename, &st)<0) return -1;
  return st.st_size;
}

static char *read_file(char *name, size_t *len)
{
  ptrdiff_t tmp;
  char *buffer;
  int fd;
  int l=filesize(name);
  if(l<0) return 0;

  len[0]=(size_t)l;
  do{
#ifdef DLDEBUG
  fprintf(stderr,"Opening file: %s\n",name);
#endif
    fd=fd_open(name,fd_RDONLY | fd_BINARY, 0);
  }while(fd<0 && errno == EINTR);

  if(fd < 0) return 0;

  buffer=(unsigned char *)malloc(l);
  
  for(tmp=0;tmp<l;)
  {
    ptrdiff_t x;
    do{
      x=fd_read(fd, buffer + tmp, l - tmp);
    } while(x < 0 && errno == EINTR);
#ifdef DLDEBUG
      fprintf(stderr,"Reading ... tmp=%d len=%d x=%d errno=%d\n",tmp,l,x,errno);
#endif
    if(x > 0) tmp+=x;
    if(x<0)
    {
      free(buffer);
      return 0;
    }
  }
  while(fd_close(fd) < 0 && errno==EINTR);
#ifdef DLDEBUG
  fprintf(stderr,"Done reading\n");
#endif
  return buffer;
}

/****************************************************************/

static struct DLHandle *first;

struct DLLList
{
  struct DLLList *next;
  HINSTANCE dll;
};

struct DLHandle
{
  int refs;
  char *filename;
  int flags;
  struct DLHandle *next;

  size_t memsize;
  void *memory;
  struct Htable *htable;
  struct DLLList *dlls;
};

static void *lookup_dlls(struct DLLList *l, char *name)
{
  void *ret;
  if(name[0]=='_') name++;
#ifdef DLDEBUG
  fprintf(stderr,"DL: lookup_dlls(%s)\n",name);
#endif
  while(l)
  {
    if((ret=GetProcAddress(l->dll,name))) return ret;
    l=l->next;
  }
  return 0;
}

static void dlllist_free(struct DLLList *l)
{
  if(l)
  {
    FreeLibrary(l->dll);
    dlllist_free(l->next);
    free((char *)l);
  }
}

static int append_dlllist(struct DLLList **l,
			  char *name)
{
  struct DLLList *n;
  HINSTANCE tmp=LoadLibrary(name);
  if(!tmp) return 0;
  n=(struct DLLList *)malloc(sizeof(struct DLLList));
  n->dll=tmp;
#ifdef DLDEBUG
  fprintf(stderr,"append_dlllist(%s)->%p\n",name,n->dll);
#endif
  n->next=0;
  while( *l ) l= & (*l)->next;
  *l=n;
  return 1;
}

#define i1(X) (data->buffer[(X)])
#define i2(X) (i1((X))|(i1((X)+1)<<8))
#define i4(X) (i1((X))|(i1((X)+1)<<8)|\
                (i1((X)+2)<<16)|(i1((X)+3)<<24))
/* Truncated */
#define i8(X) (i1((X))|(i1((X)+1)<<8)|\
                (i1((X)+2)<<16)|(i1((X)+3)<<24))

#define COFF_SECT_NOLOAD (1<<1)
#define COFF_SECT_MEM_DISCARDABLE (1<<25)
#define COFF_SECT_MEM_LNK_REMOVE (1<<11)
#define COFF_SECT_LNK_INFO 0x200

#define COFF_SYMBOL_EXTERN 2

struct COFF
{
  INT16 machine;
  INT16 num_sections;
  INT32 timestamp;
  INT32 symboltable;
  INT32 num_symbols;
  INT16 sizeof_optheader;
  INT16 characteristics;
};

union COFFname
{
  INT32 ptr[2];
  char text[8];
};

struct COFFsection
{
  union COFFname name;

  INT32 virtual_size;
  INT32 virtual_addr;

  INT32 raw_data_size;
  INT32 ptr2_raw_data;
  INT32 ptr2_relocs;
  INT32 ptr2_linenums;
  INT16 num_relocs;
  INT16 num_linenums;
  INT32 characteristics;
};

struct COFFSymbol
{
  union COFFname name;
  INT32 value;
  INT16 secnum;
  INT16 type;
  INT8  class;
  INT8  aux;
};


#define COFFReloc_type_dir32 6
#define COFFReloc_type_dir32nb 7
#define COFFReloc_type_sect 10
#define COFFReloc_type_sectrel 11 
#define COFFReloc_type_rel32 20

/* This structure is correct, but should not be used because of
 * portability issues
 */
struct COFFReloc
{
  INT32 location;
  INT32 symbol;
  INT16 type;
};


struct DLTempData
{
  unsigned char *buffer;
  ptrdiff_t buflen;
  int flags;
};


struct DLObjectTempData
{
  unsigned char *buffer;
  size_t buflen;
  int flags;

  struct COFF *coff;
  struct COFFSymbol *symbols;
  char *stringtable;
  struct COFFsection *sections;

  /* temporary storage */
  char **symbol_addresses;
  char **section_addresses;
};


static struct DLLList *global_dlls=0;
static struct Htable *global_symbols=0;
static char *dlerr=0;

static void *low_dlsym(struct DLHandle *handle,
		       char *name,
		       size_t len)
{
  void *ptr;
  char *tmp;
  if(len > 6 && !memcmp(name,"__imp_",6))
  {
    name+=6;
    len-=6;
  }
  tmp=name;
#ifdef DLDEBUG
  if(name[len])
    fprintf(stderr,"low_dlsym(%c%c%c%c%c%c%c%c)\n",
	    name[0],
	    name[1],
	    name[2],
	    name[3],
	    name[4],
	    name[5],
	    name[6],
	    name[7]);
  else
    fprintf(stderr,"low_dlsym(%s)\n",name);
#endif
  ptr=htable_get(handle->htable,name,len);
  if(!ptr) ptr=htable_get(global_symbols,name,len);
  if(!ptr)
  {
    if(name[len])
    {
      tmp=(char *)alloca(len+1);
      MEMCPY(tmp, name, len);
      tmp[len]=0;
    }
    ptr=lookup_dlls(handle->dlls, tmp);
    if(!ptr) ptr=lookup_dlls(global_dlls, tmp);
  }
#ifdef DL_VERBOSE
  if(!ptr)
    fprintf(stderr,"Failed to find identifier %s\n",tmp);
#endif
  return ptr;
}
		       
void *dlsym(struct DLHandle *handle, char *name)
{
  return low_dlsym(handle, name, strlen(name));
}

const char *dlerror(void)
{
  return dlerr;
}

static parse_link_info(struct DLHandle *ret,
		       struct DLObjectTempData *data,
		       char *info,
		       int len)
{
  int ptr=0;
  char buffer[1024];

#ifdef DLDEBUG
  {
    int z;
    fprintf(stderr,"DLINFO(%d): ",len);
    for(z=0;z<len;z++) fprintf(stderr,"%c",info[z]);
    fprintf(stderr,"\n");
  }
#endif

  while(ptr < len)
  {
    int l,x=ptr;
    char *end;

#ifdef DLDEBUG    
    fprintf(stderr,"Parse link info ptr=%d\n",ptr,l);
#endif

    end=MEMCHR(info+ptr,' ',len-ptr);
    if(end) 
      l=end - (info+x);
    else
      l=len-ptr;

#ifdef DLDEBUG    
    fprintf(stderr,"Parse link info ptr=%d len=%d\n",ptr,l);
#endif

    if(info[x] == '-')
    {
      x++;
      if(info[x]=='?') x++;
      if(!memcmp(info+x,"lib:",4) || !memcmp(info+x,"LIB:",4))
      {
	x+=4;
	memcpy(buffer,info+x,l-x);
	buffer[l-x]=0;
	append_dlllist(&ret->dlls, buffer);
      }
    }
    ptr+=l+1;
  }
}

static int dl_load_coff_files(struct DLHandle *ret,
			      struct DLObjectTempData *tmp,
			      int num)
{
  int e=0,s,r;
  size_t ptr;
  size_t num_exports=0;

#define data (tmp+e)
#define SYMBOLS(X) (*(struct COFFSymbol *)(18 * (X) + (char *)data->symbols))

#ifdef DLDEBUG
  fprintf(stderr,"dl_load_coff_files(%p,%p,%d)\n",ret,tmp,num);
#endif

  if(!num) return 0;

  ret->memsize=0;

  /* Initialize tables and count how much memory is needed */
#ifdef DLDEBUG
  fprintf(stderr,"DL: counting\n");
#endif
  for(e=0;e<num;e++)
  {
    data->coff=(struct COFF *)data->buffer;
    data->symbols=(struct COFFSymbol *)(data->buffer +
					data->coff->symboltable);

    data->stringtable=(unsigned char *)( ((char *)data->symbols) + 
					 18 * data->coff->num_symbols);

    data->sections=(struct COFFsection *)( ((char *)data->coff) +
					   sizeof(struct COFF)+ 
					   data->coff->sizeof_optheader);
#ifdef DLDEBUG
  fprintf(stderr,"DL: %x %d %d %d\n",
	  data->coff,
	  sizeof(data->coff[0]),
	  data->coff->sizeof_optheader,
	  sizeof(struct COFFsection));
#endif

    for(s=0;s<data->coff->num_sections;s++)
    {
      size_t align;
      if(data->sections[s].characteristics & COFF_SECT_LNK_INFO)
      {
	parse_link_info(ret,data,
			(char *)data->buffer + data->sections[s].ptr2_raw_data,
			data->sections[s].raw_data_size);
	continue;
      }

      if(data->sections[s].characteristics & 
	 (COFF_SECT_NOLOAD | COFF_SECT_MEM_DISCARDABLE | COFF_SECT_MEM_LNK_REMOVE))
	continue;

      align=(data->sections[s].characteristics>>20) & 0xf;
      align=(1<<(align-1))-1;
#ifdef DLDEBUG
      fprintf(stderr,"DL: section[%d,%d], %d bytes, align=%d (0x%x)\n",e,s,data->sections[s].raw_data_size,align+1,data->sections[s].characteristics);
#endif
      ret->memsize+=align;
      ret->memsize&=~align;
      ret->memsize+=data->sections[s].raw_data_size;
    }

    /* Count export symbols */
    for(s=0;s<data->coff->num_symbols;s++)
    {
      if(SYMBOLS(s).class == COFF_SYMBOL_EXTERN &&
	 SYMBOLS(s).secnum > 0)
	num_exports++;
    }

    if(data->coff->num_symbols)
    {
      /* Assumes 32 bit pointers! */
      ret->memsize+=data->coff->num_symbols * 4 + 3;
      ret->memsize&=~3;
    }
  }

#ifdef DLDEBUG
  fprintf(stderr,"DL: allocating %d bytes\n",ret->memsize);
#endif

  /* Allocate executable memory */
  ret->memory=VirtualAlloc(0,
			   ret->memsize,
			   MEM_COMMIT,
			   PAGE_EXECUTE_READWRITE);

  if(!ret->memory)
  {
#ifdef DLDEBUG
    fprintf(stderr,"Failed to allocate %d bytes RWX-memory.\n",ret->memsize);
#endif
    dlerr="Failed to allocate memory";
    return -1;
  }

#ifdef DLDEBUG
  fprintf(stderr,"DL: Got %d bytes RWX memory at %p\n",ret->memsize,ret->memory);
#endif


  /* Create a hash table for exported symbols */
  ret->htable=alloc_htable(num_exports);

  if(data->flags & RTLD_GLOBAL)
    global_symbols = htable_add_space(global_symbols, num_exports);

#ifdef DLDEBUG
  fprintf(stderr,"DL: moving code\n");
#endif

  /* Copy code into executable memory */
  ptr=0;
  for(e=0;e<num;e++)
  {
    data->section_addresses =
      (char **)calloc(sizeof(char *),data->coff->num_sections);

    for(s=0;s<data->coff->num_sections;s++)
    {
      size_t align;
      if(data->sections[s].characteristics & 
       (COFF_SECT_NOLOAD | COFF_SECT_MEM_DISCARDABLE | COFF_SECT_MEM_LNK_REMOVE))
	continue;

      align=(data->sections[s].characteristics>>20) & 0xf;
      align=(1<<(align-1))-1;
      ptr+=align;
      ptr&=~align;

      data->section_addresses[s]=(char *)ret->memory + ptr;
#ifdef DLDEBUG
      fprintf(stderr,"DL: section[%d]=%p\n",s,data->section_addresses[s]);
#endif

      if(data->sections[s].ptr2_raw_data)
      {
	memcpy((char *)ret->memory + ptr,
	       data->buffer + data->sections[s].ptr2_raw_data,
	       data->sections[s].raw_data_size);
      }else{
	memset((char *)ret->memory + ptr,
	       0,
	       data->sections[s].raw_data_size);
      }
      ptr+=data->sections[s].raw_data_size;
    }

    if(data->coff->num_symbols)
    {
      /* Assumes 32 bit pointers! */
      ptr+=3;
      ptr&=~3;
      data->symbol_addresses=(char **)( ((char *)ret->memory) + ptr);
#ifdef DLDEBUG
      fprintf(stderr,"DL: data->symbol_addresses=%p\n",
	      data->symbol_addresses);
#endif
      ptr+=data->coff->num_symbols * 4;
    }


#ifdef DLDEBUG
    fprintf(stderr,"DL: exporting symbols (%d)\n",data->coff->num_symbols);
#endif

    /* Export symbols */
    for(s=0;s<data->coff->num_symbols;s++)
    {
#if defined(DLDEBUG) && 0
      fprintf(stderr,"DL: xporting, class=%d secnum=%d\n",
	      SYMBOLS(s).class,
	      SYMBOLS(s).secnum);
#endif
	
      if(SYMBOLS(s).class == COFF_SYMBOL_EXTERN &&
	 SYMBOLS(s).secnum > 0)
      {
	char *value=data->section_addresses[SYMBOLS(s).secnum-1]+
		      SYMBOLS(s).value;

	char *name;
	size_t len;

	if(!SYMBOLS(s).name.ptr[0])
	{
	  name=data->stringtable + SYMBOLS(s).name.ptr[1];
	  len=strlen(name);
#ifdef DLDEBUG
	  fprintf(stderr,"DL: exporting %s -> %p\n",name,value);
#endif
	}else{
	  name=SYMBOLS(s).name.text;
	  len=STRNLEN(name,8);
#ifdef DLDEBUG
	  fprintf(stderr,"DL: exporting %c%c%c%c%c%c%c%c -> %p\n",
		  name[0],
		  name[1],
		  name[2],
		  name[3],
		  name[4],
		  name[5],
		  name[6],
		  name[7],value);
#endif
	}
	
	htable_put(ret->htable, name, len, value);
	if(data->flags & RTLD_GLOBAL)
	  htable_put(global_symbols, name, len, value);
      }
    }
  }


#ifdef DLDEBUG
  fprintf(stderr,"DL: resolving\n");
#endif

  /* Do resolve and relocations */
  for(e=0;e<num;e++)
  {
    for(s=0;s<data->coff->num_sections;s++)
    {
      struct COFFReloc *relocs;
      if(data->sections[s].characteristics & 
	 (COFF_SECT_NOLOAD | COFF_SECT_MEM_DISCARDABLE | COFF_SECT_MEM_LNK_REMOVE))
	continue;

      /* relocate all symbols in this section */
      relocs=(struct COFFReloc *)(data->buffer +
				  data->sections[s].ptr2_relocs);

      dlerr=0;
      for(r=0;r<data->sections[s].num_relocs;r++)
      {

/* This should work fine on x86 and other processors which handles
 * unaligned memory access
 */
#define RELOCS(X) (*(struct COFFReloc *)( 10*(X) + (char *)relocs ))

	char *loc=data->section_addresses[s] + RELOCS(r).location;
	char *ptr;
	ptrdiff_t sym=RELOCS(r).symbol;

#ifdef DLDEBUG
	fprintf(stderr,"DL: Reloc[%d] sym=%d loc=%d type=%d\n",
		r,
		sym,
		RELOCS(r).location,
		RELOCS(r).type);
#endif

	if(!(ptr=data->symbol_addresses[sym]))
	{
	  /* FIXME: check storage class of symbol */
	  
	  if(SYMBOLS(sym).secnum > 0)
	  {
	    ptr=data->symbol_addresses[sym]=
	      data->section_addresses[SYMBOLS(sym).secnum-1]+
	      SYMBOLS(sym).value;
	  }
	  else if(!SYMBOLS(sym).value)
	  {
	    char *name;
	    size_t len;

	    if(!SYMBOLS(sym).name.ptr[0])
	    {
	      name=data->stringtable + SYMBOLS(sym).name.ptr[1];
	      len=strlen(name);
	    }else{
	      name=SYMBOLS(sym).name.text;
	      len=STRNLEN(name,8);
	    }
	       
#ifdef DLDEBUG
	    fprintf(stderr,"DL: resolving symbol[%d], type=%d class=%d secnum=%d aux=%d value=%d\n",
		    sym,
		    SYMBOLS(sym).type,
		    SYMBOLS(sym).class,
		    SYMBOLS(sym).secnum,
		    SYMBOLS(sym).aux,
		    SYMBOLS(sym).value);
#endif

	    if(!(ptr=low_dlsym(ret, name, len)))
	    {
	      dlerr="Symbol not found";
#ifndef DL_VERBOSE
	      return -1;
#endif
	    }
	  }else{
#ifdef DLDEBUG
	    fprintf(stderr,"Gnapp??\n");
#endif
	    return -1;
	  }

	  data->symbol_addresses[sym]=ptr;
	}

	switch(RELOCS(r).type)
	{
	  default:
	    dlerr="Unknown relocation type.";
	    return -1;

	    /* We may need to support more types here */
	  case COFFReloc_type_dir32:
	    if(SYMBOLS(sym).type >> 4 == 2)
	    {
	      /* Indirect function pointer (#&$#*&#$ */
	      ((char **)loc)[0]=(char *)(data->symbol_addresses + sym);
#ifdef DLDEBUG
	    fprintf(stderr,"DL: reloc indirect: loc %p = %p, *%p = %p + %d\n",
		    loc,ptr,
		    *(char **)loc,data->symbol_addresses,sym);
#endif
	    }else{
	      ((INT32 *)loc)[0]+=(INT32)ptr;
#ifdef DLDEBUG
	    fprintf(stderr,"DL: reloc absolute: loc %p = %p\n", loc,ptr);
#endif
	    }
	       
	    break;

          case COFFReloc_type_rel32:
#ifdef DLDEBUG
	    fprintf(stderr,"DL: reloc relative: loc %p = %p = %d\n",
		    loc,
		    ptr,
		    ptr - (loc+sizeof(INT32)));
#endif
	    ((INT32*)loc)[0]+=ptr - (loc+sizeof(INT32));
	    break;
	}
      }

#ifdef DL_VERBOSE
      if(dlerr)
	return -1;
#endif

    }
  }

  return 0; /* Done (I hope) */
#undef data
}

static int dl_loadarchive(struct DLHandle *ret,
			  struct DLTempData *data)
{
  ptrdiff_t pos,len,o;
  struct DLTempData save=*data;
  int retcode;

  struct DLObjectTempData *tmp;
  INT32 object_files=0;


#ifdef DLDEBUG
  fprintf(stderr,"dl_loadarchive\n");
#endif

  /* Count how many object files there are in the loop */
  for(pos=8;pos < save.buflen;pos+=60+(len+1)&~1)
  {
    len=STRTOL(save.buffer+pos+48,0,10);
    if(save.buffer[pos]!='/') object_files++;
#ifdef DLDEBUG
    fprintf(stderr,"DLARCH: pos=%d len=%d\n",pos,len);
#endif
  }
#ifdef DLDEBUG
  fprintf(stderr,"DLARCH: object files in archive: %d\n",object_files);
#endif


  tmp=(struct DLObjectTempData *)malloc(sizeof(struct DLObjectTempData) * object_files);
  o=0;

  /* Initialize data objects for these */
  for(pos=8;pos < save.buflen;pos+=60+(len+1)&~1)
  {
    len=STRTOL(save.buffer+pos+48,0,10);
    if(save.buffer[pos]!='/')
    {
      tmp[o].buflen=len;
      tmp[o].buffer=data->buffer+pos+60;
      o++;
    }
  }

  retcode=dl_load_coff_files(ret, tmp, object_files);
  free((char *)tmp);
  return retcode;
}

static int dl_loadcoff(struct DLHandle *ret,
		       struct DLTempData *data)
{
  struct DLObjectTempData tmp;

#ifdef DLDEBUG
  fprintf(stderr,"dl_loadcoff\n");
#endif

  tmp.buflen=data->buflen;
  tmp.buffer=data->buffer;
  tmp.flags = data->flags;
  return dl_load_coff_files(ret, &tmp, 1);
}

static int dl_loadpe(struct DLHandle *ret,
		     struct DLTempData *data)
{
  dlerr="PE images not supported yet.";
  return -1;
}

static int dl_load_file(struct DLHandle *ret,
		       struct DLTempData *data)
{
  INT32 tmp;
#ifdef DLDEBUG
  fprintf(stderr,"dl_load_file\n");
#endif

  if(data->buflen > 8 && ! memcmp(data->buffer,"!<arch>\n",8))
    return dl_loadarchive(ret,data);

  tmp=i4(0x3c);
  if(tmp >=0 && tmp<data->buflen-4 &&!memcmp(data->buffer+tmp,"PE\0\0",4))
    return dl_loadpe(ret, data);

  return dl_loadcoff(ret, data);
}

static void init_dlopen(void);

struct DLHandle *dlopen(char *name, int flags)
{
  struct DLHandle *ret;
  struct DLTempData tmpdata;
  int retcode;
  tmpdata.flags=flags;

  if(!global_symbols) init_dlopen();

#ifdef DLDEBUG
  fprintf(stderr,"dlopen(%s,%d)\n",name,flags);
#endif
  abort();

  for(ret=first;ret;ret=ret->next)
  {
    if(!strcmp(name, ret->filename))
    {
      ret->refs++;
      return ret;
    }
  }

  /* LD_LIBRARY_PATH ? */
  tmpdata.buflen=filesize(name);
  if(tmpdata.buflen==-1) return 0;

  ret=(struct DLHandle *)calloc(sizeof(struct DLHandle),1);
  ret->refs=1;
  ret->filename=strdup(name);
  ret->flags=flags;

  tmpdata.buffer = (unsigned char *)read_file(name, &tmpdata.buflen);
  if(!tmpdata.buffer)
  {
    dlerr="Failed to read file.";
    dlclose(ret);
    return 0;
  }

  
  retcode=dl_load_file(ret, &tmpdata);
  free(tmpdata.buffer);

  if(retcode < 0)
  {
    dlclose(ret);
    return 0;
  }

 /* register module for future dlopens */
    
  return ret;
}

int dlclose(struct DLHandle *h)
{
#ifdef DLDEBUG
  fprintf(stderr,"Dlclose(%s)\n",h->filename);
#endif
  if(! --h->refs)
  {
    if(h->filename) free(h->filename);
    if(h->htable) htable_free(h->htable,0);
    if(h->dlls) dlllist_free(h->dlls);
    if(h->memory) VirtualFree(h->memory,0,MEM_RELEASE);
    free((char *)h);
  }
  return 0;
}

static void init_dlopen(void)
{
  int tmp;
  extern char ** ARGV;
  INT32 offset;
  struct DLObjectTempData objtmp;
  HINSTANCE h;

#ifdef DLDEBUG
  fprintf(stderr,"dlopen_init(%s)\n",ARGV[0]);
#endif

  h=LoadLibrary(ARGV[0]);

#undef data
#define data (&objtmp)
  assert(sizeof(union COFFname) == 8);
  assert(sizeof(struct COFFsection) == 40);

  data->buffer=(char *)h; /* portable? nope */
  offset=i4(0x3c);
  
  if(!memcmp(data->buffer+offset, "PE\0\0", 4))
  {
    int s;
    unsigned char *buf;
    size_t len;
    objtmp.coff=(struct COFF *)(data->buffer+offset+4);
#ifdef DLDEBUG
    fprintf(stderr,"Found PE header.\n");
    fprintf(stderr,"num_sections=%d num_symbols=%d sizeof(optheader)=%d\n",
	    objtmp.coff->num_sections,
	    objtmp.coff->num_symbols,
	    objtmp.coff->sizeof_optheader);
#endif
    
    data->sections=(struct COFFsection *)( ((char *)data->coff) +
					   sizeof(struct COFF)+ 
					   data->coff->sizeof_optheader);
    
    
    buf= (unsigned char *)read_file(ARGV[0], &len);
    
    data->symbols=(struct COFFSymbol *)(buf +
					data->coff->symboltable);
    
    data->stringtable=(unsigned char *)( ((char *)data->symbols) + 
					 18 * data->coff->num_symbols);
    
    
    global_symbols=alloc_htable(data->coff->num_symbols);
    
#ifdef DLDEBUG
    fprintf(stderr,"buffer=%p\n",data->buffer);
    fprintf(stderr,"buf=%p\n",buf);
    fprintf(stderr,"symbols=%p\n",data->symbols);
    fprintf(stderr,"stringtable=%p\n",data->stringtable);
    fprintf(stderr,"sections=%p\n",data->sections);
    
    
    for(s=0;s<data->coff->num_sections;s++)
    {
      fprintf(stderr,"sect[%d]=%p\n",
	      s,
	      data->buffer + data->sections[s].virtual_addr);
    }
    
    fprintf(stderr,"data->coff->num_symbols=%d\n",data->coff->num_symbols);
#endif
    
    
    for(s=0;s<data->coff->num_symbols;s++)
    {
      char *name;
      int len;
      if(SYMBOLS(s).class != 2) continue;
      if(SYMBOLS(s).secnum <= 0) continue;
      
      if(!SYMBOLS(s).name.ptr[0])
      {
	name=data->stringtable + SYMBOLS(s).name.ptr[1];
	if(name[0]=='?') continue; /* C++ garbled */
	len=strlen(name);
#ifdef DLDEBUG
	fprintf(stderr,"Sym[%04d] %s : ",s,name);
#endif
      }else{
	name=SYMBOLS(s).name.text;
	if(name[0]=='?') continue; /* C++ garbled */
	len=STRNLEN(name,8);
#ifdef DLDEBUG
	fprintf(stderr,"Sym[%04d] %c%c%c%c%c%c%c%c = ",s,
		name[0],
		name[1],
		name[2],
		name[3],
		name[4],
		name[5],
		name[6],
		name[7]);
#endif
      }
#ifdef DLDEBUG
      fprintf(stderr,"sect=%d value=%d class=%d type=%d",
	      SYMBOLS(s).secnum,
	      SYMBOLS(s).value,
	      SYMBOLS(s).class,
	      SYMBOLS(s).type);
      
      fprintf(stderr," addr=%p+%x-%x+%x = %p",
	      data->buffer,
	      data->sections[SYMBOLS(s).secnum-1].virtual_addr,
	      data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data,
	      SYMBOLS(s).value,
	      data->buffer +
	      data->sections[SYMBOLS(s).secnum-1].virtual_addr -
	      data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
	      SYMBOLS(s).value);
      
      fprintf(stderr,"\n");
#endif
      
      htable_put(global_symbols, name, len,
		 data->buffer +
		 data->sections[SYMBOLS(s).secnum-1].virtual_addr -
		 data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
		 SYMBOLS(s).value);		   
      
    }
    free(buf);
  }else{
#ifdef DLDEBUG
    fprintf(stderr,"Couldn't find PE header.\n");
#endif
    append_dlllist(&global_dlls, ARGV[0]);
    global_symbols=alloc_htable(997);
#define EXPORT(X) \
  DO_IF_DLDEBUG( fprintf(stderr,"EXP: %s\n",#X); ) \
  htable_put(global_symbols,"_" #X,sizeof(#X)-sizeof("")+1, &X)
    
    
    fprintf(stderr,"Fnord, rand()=%d\n",rand());
    EXPORT(fprintf);
    EXPORT(_iob);
    EXPORT(abort);
    EXPORT(rand);
    EXPORT(srand);
  }

#ifdef DLDEBUG
  fprintf(stderr,"DL: init done\n");
#endif
}

/****************************************************************/

#ifdef TEST

__declspec(dllexport) void func1(void)
{
  fprintf(stderr,"func1()\n");
  fprintf(stdout,"func1()\n");
}

__declspec(dllexport) void func3(void)
{
  fprintf(stderr,"func3()\n");
}

__declspec(dllexport) void func4(void)
{
  fprintf(stderr,"func4()\n");
}

__declspec(dllexport) void func5(void)
{
  fprintf(stderr,"func5()\n");
}

int main(int argc, char ** argv)
{
  int tmp;
  assert(sizeof(union COFFname) == 8);
  assert(sizeof(struct COFFsection) == 40);
#if 0
  HINSTANCE l;
  void *addr;

  fprintf(stderr,"....\n");
  l=LoadLibrary(argv[0]);
  fprintf(stderr,"LoadLibrary(%s) => %p\n",argv[0],(long)l);
  addr = GetProcAddress(l, "func1");
  fprintf(stderr,"GetProcAddress(\"func1\") => %p (&func1 = %p)\n",addr, func1);
#else

  {
    HINSTANCE h=LoadLibrary(argv[0]);
    INT32 offset;
    struct DLObjectTempData objtmp;
#undef data
#define data (&objtmp)

    data->buffer=(char *)h; /* portable? nope */
    offset=i4(0x3c);

    if(!memcmp(data->buffer+offset, "PE\0\0", 4))
    {
      int s;
      unsigned char *buf;
      size_t len;
      objtmp.coff=(struct COFF *)(data->buffer+offset+4);
#ifdef DLDEBUG
      fprintf(stderr,"Found PE header.\n");
      fprintf(stderr,"num_sections=%d num_symbols=%d sizeof(optheader)=%d\n",
	      objtmp.coff->num_sections,
	      objtmp.coff->num_symbols,
	      objtmp.coff->sizeof_optheader);
#endif

      data->sections=(struct COFFsection *)( ((char *)data->coff) +
					     sizeof(struct COFF)+ 
					     data->coff->sizeof_optheader);


      buf= (unsigned char *)read_file(argv[0], &len);

      data->symbols=(struct COFFSymbol *)(buf +
					  data->coff->symboltable);
      
      data->stringtable=(unsigned char *)( ((char *)data->symbols) + 
					   18 * data->coff->num_symbols);
      

      global_symbols=alloc_htable(data->coff->num_symbols);

#ifdef DLDEBUG
      fprintf(stderr,"buffer=%p\n",data->buffer);
      fprintf(stderr,"buf=%p\n",buf);
      fprintf(stderr,"symbols=%p\n",data->symbols);
      fprintf(stderr,"stringtable=%p\n",data->stringtable);
      fprintf(stderr,"sections=%p\n",data->sections);


      for(s=0;s<data->coff->num_sections;s++)
      {
	fprintf(stderr,"sect[%d]=%p\n",
		s,
		data->buffer + data->sections[s].virtual_addr);
      }

      fprintf(stderr,"data->coff->num_symbols=%d\n",data->coff->num_symbols);
#endif


      for(s=0;s<data->coff->num_symbols;s++)
      {
	char *name;
	int len;
	if(SYMBOLS(s).class != 2) continue;
	if(SYMBOLS(s).secnum <= 0) continue;

	if(!SYMBOLS(s).name.ptr[0])
	{
	  name=data->stringtable + SYMBOLS(s).name.ptr[1];
	  if(name[0]=='?') continue; /* C++ garbled */
	  len=strlen(name);
#ifdef DLDEBUG
	  fprintf(stderr,"Sym[%04d] %s : ",s,name);
#endif
	}else{
	  name=SYMBOLS(s).name.text;
	  if(name[0]=='?') continue; /* C++ garbled */
	  len=STRNLEN(name,8);
#ifdef DLDEBUG
	  fprintf(stderr,"Sym[%04d] %c%c%c%c%c%c%c%c = ",s,
		  name[0],
		  name[1],
		  name[2],
		  name[3],
		  name[4],
		  name[5],
		  name[6],
		  name[7]);
#endif
	}
#ifdef DLDEBUG
	fprintf(stderr,"sect=%d value=%d class=%d type=%d",
		SYMBOLS(s).secnum,
		SYMBOLS(s).value,
		SYMBOLS(s).class,
		SYMBOLS(s).type);

	fprintf(stderr," addr=%p+%x-%x+%x = %p",
		data->buffer,
		data->sections[SYMBOLS(s).secnum-1].virtual_addr,
		data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data,
		SYMBOLS(s).value,
		data->buffer +
		data->sections[SYMBOLS(s).secnum-1].virtual_addr -
		data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
		SYMBOLS(s).value);

	fprintf(stderr,"\n");
#endif

	  htable_put(global_symbols, name, len,
		     data->buffer +
		     data->sections[SYMBOLS(s).secnum-1].virtual_addr -
		     data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
		     SYMBOLS(s).value);		   

      }
      free(buf);
    }else{
#ifdef DLDEBUG
      fprintf(stderr,"Couldn't find PE header.\n");
#endif
      append_dlllist(&global_dlls, argv[0]);
      global_symbols=alloc_htable(997);
#define EXPORT(X) \
  DO_IF_DLDEBUG( fprintf(stderr,"EXP: %s\n",#X); ) \
  htable_put(global_symbols,"_" #X,sizeof(#X)-sizeof("")+1, &X)
      
      
      fprintf(stderr,"Fnord, rand()=%d\n",rand());
      EXPORT(fprintf);
      EXPORT(_iob);
      EXPORT(abort);
      EXPORT(rand);
      EXPORT(srand);
    }
  }
  
  /* FIXME: open argv[0] and check what dlls is it is linked
   *        against and add those to the list
   */
  {
    struct DLHandle *h;
    void *f;
    h=dlopen("./foo.o",0);
    fprintf(stderr,"dlopen(./fnord) => %p\n",h);
    if(h)
    {
      f=dlsym(h,"_func2");
      fprintf(stderr,"dsym(_func2) => %p\n",f);
      if(f)
      {
	fprintf(stderr,"Calling %p (func1=%p)\n",f,func1);
	fflush(stderr);
	((void (*)(void))f)();
      }
    }
  }
#endif
  exit(0);
}


#endif
