/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: dlopen.c,v 1.74 2004/09/18 20:50:48 nilsson Exp $
*/

#include <global.h>
#include "fdlib.h"
#define DL_INTERNAL
#include "pike_dlfcn.h"
#include "pike_memory.h"
#include "pike_error.h"
#include "pike_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#if !defined(__FreeBSD__) && !defined(__OpenBSD__)
/* FreeBSD and OpenBSD has <malloc.h>, but it just contains a warning... */
#include <malloc.h>
#endif /* !__FreeBSD__ && !__OpenBSD__ */
#include <windows.h>
#include <memory.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>
#include <tchar.h>
#include <interpret.h>
#include <callback.h>

/* In case we're compiling with NDEBUG */
_CRTIMP void __cdecl _assert(void*, void*, unsigned);

#ifdef DEBUG_MALLOC
#define ACCEPT_MEMORY_LEAK(X) dmalloc_accept_leak(X)
#else
#define ACCEPT_MEMORY_LEAK(X)
#endif

static char *dlerr=0;


/* Todo:
 *  Make image debugable if possible
 *  Separate RWX, RW and R memory sections.
 */

#ifdef _WIN64
#define USE_PDB_SYMBOLS
#endif

/* Enable debug output if compiled on win64. */
#ifdef _WIN64
#define DLDEBUG 1
#endif /* _WIN64 */

#define DL_VERBOSE 1

/*
 * This define makes dlopen create a logfile which maps
 * addresses to symbols. This can be very helpful when
 * debugging since regular debug info does not work with
 * code loaded by dlopen.
 */
/* #define DL_SYMBOL_LOG */


/*
 * This puts unused memory after each writable data section
 * In the future, this padded memory will be filled with
 * a pattern that can be verified as not being changed later.
 * This is for debug only of course
 */
#define PAD_DATA 0


/*
 * If you enable this, calls to
 * fopen, fseek etc. from dynamic modules will be dumped to
 * stderr (only fopen and fseek implemented so far, see below)
 */
/* #define DL_WRAP_STDIO */

/*
 * If EFENCE_BOTTOM is defined, each writable symbol will have it's
 * own data page and be located at the end of that page with a guard
 * page right afterwards.
 */
/* #define EFENCE_BOTTOM */

/*
 * If EFENCE_TOP is defined, each writable symbol will have it's
 * own data page and be located at the beginning of that page with a guard
 * page right afterwards.
 */
/* #define EFENCE_TOP */

#define REALLY_FLUSH() /* do{ fflush(stderr); Sleep(500); }while(0) */

#ifdef DLDEBUG
#define FLUSH() REALLY_FLUSH()
#define DO_IF_DLDEBUG(X) X
#else
#define FLUSH()
#define DO_IF_DLDEBUG(X)
#endif

#ifdef DL_SYMBOL_LOG
FILE *dl_symlog;
#endif

/* Windows/IA64 does not prepend symbols with "_". */
#ifdef _WIN64
#define SYMBOL_PREFIX	""
#else /* !_WIN64 */
#define SYMBOL_PREFIX	"_"
#endif /* _WIN64 */


#if defined(EFENCE_BOTTOM) || defined(EFENCE_TOP)
#define EFENCE
DWORD dlopen_page_size;

#define EFENCE_ADD(X) 				\
  X+=dlopen_page_size*2-1;			\
  X&=-dlopen_page_size;

static char *EFENCE_ALIGN(char *x, int size, int align)
{
  if(!VirtualAlloc(x,size,MEM_COMMIT,PAGE_EXECUTE_READWRITE) && size)
  {
    fprintf(stderr,"dlopen:EFENCE ALIGN failed to allocate %d bytes at %p\n",size,x);
  }
  
  if(!VirtualAlloc(x + ( (size + dlopen_page_size -1) & -dlopen_page_size),
		   dlopen_page_size,MEM_COMMIT,PAGE_NOACCESS))
  {
    fprintf(stderr,"dlopen:EFENCE ALIGN failed to protect a page!\n");
  }
  

#ifdef EFENCE_BOTTOM
  {
    char *ret=x + ( ((dlopen_page_size - (size & (dlopen_page_size-1))) & ~align));
#ifdef DLDEBUG
     fprintf(stderr,"ALIGN: %p@%x/%x -> %p\n",x,size,align,ret);
#endif
    return ret;
  }
	    
#else
  return x;
#endif
}


#else /* EFENCE */
#define EFENCE_ADD(X)
#define EFENCE_ALIGN(X,SIZE,AL) X
#define EFENCE_PROT_PREV_PAGE(X)
#endif

#ifndef PAD_DATA
#define PAD_DATA 0
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

#else /* PIKE_CONCAT */

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
  if(size<1) size=1;
#ifdef DLDEBUG
  fprintf(stderr,"alloc_htable(%d)\n",size);
  FLUSH();
#endif
  ret=(struct Htable *)malloc(sizeof(struct Htable) +
			      sizeof(void *)*(size-1));
  ACCEPT_MEMORY_LEAK(ret);
  ret->size=size;
  ret->entries=0;
  for(e=0;e<size;e++) ret->symbols[e]=0;
  return ret;
}

static struct Htable *htable_add_space(struct Htable *h,
				       size_t extra_space)
{
#if 1
  if(h->entries+extra_space > h->size)
  {
    struct Htable *ret;
    size_t i, new_size = (h->size * 2)+1;
    if(h->entries+extra_space > new_size)
      new_size = (h->entries+extra_space) | 0x10101;
    /** rehashto new_size */
    ret = alloc_htable(new_size);
    for(i=0; i<h->size; i++) {
      struct Sym *next, *curr = h->symbols[i];
      while(curr) {
	size_t hval;
	DO_HASHMEM(hval, curr->name, curr->len, 128);
	hval %= new_size;
	next = curr->next;
	curr->next = ret->symbols[hval];
	ret->symbols[hval] = curr;
	curr = next;
      }
    }
    ret->entries = h->entries;
    free(h);
    h = ret;
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

static void htable_put(struct Htable *h, char *name, size_t l, void *ptr,
		       int replace)
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
    if(replace)
      tmp->sym=ptr;
    return;
  }
  tmp=(struct Sym *)malloc(sizeof(struct Sym) + l);
  if(!tmp)
  {
    /* out of memory */
    exit(1);
  }
  ACCEPT_MEMORY_LEAK(tmp);
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

static int filesize(const char *filename)
{
  struct stat st;
#ifdef DLDEBUG
  fprintf(stderr,"filesize(%s)\n",filename);
#endif
  if(stat(filename, &st)<0) return -1;
  return st.st_size;
}

static char *read_file(const char *name, size_t *len)
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
#ifndef TEST
    check_threads_etc();
#endif
  }while(fd<0 && errno == EINTR);

  if(fd < 0) return 0;

  buffer=(unsigned char *)malloc(l);
  
  for(tmp=0;tmp<l;)
  {
    ptrdiff_t x;
    do{
      x=fd_read(fd, buffer + tmp, l - tmp);
#ifndef TEST
      check_threads_etc();
#endif
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
  while(fd_close(fd) < 0 && errno==EINTR) {
#ifndef TEST
    check_threads_etc();
#endif
  }
#ifdef DLDEBUG
  fprintf(stderr,"Done reading\n");
  fflush(stderr);
  Sleep(500);
#endif
  return buffer;
}

/****************************************************************/

struct DLHandle
{
  int refs;
  char *filename;
  int flags;
  struct DLHandle *next;

  size_t codesize;
  size_t memsize;
  void *code;
  void *memory;
  struct Htable *htable;
  struct DLLList *dlls;
};

struct DLLList
{
  struct DLLList *next;
  HINSTANCE dll;
};

static struct DLHandle *first;
static struct DLHandle global_dlhandle;
static INT32 global_imagebase = 0x00400000;

static HMODULE low_LoadLibrary(LPCTSTR lpFileName)
{
  if(_tcschr(lpFileName, '/')) {
    _TCHAR *p, *tmp = alloca((_tcslen(lpFileName)+1)*sizeof(_TCHAR));
    _tcscpy(tmp, lpFileName);
    for(p=tmp; (p=_tcschr(p, '/')); p++)
      *p = '\\';
    return LoadLibrary(tmp);
  } else return LoadLibrary(lpFileName);
}

static void *lookup_dlls(struct DLLList *l, char *name)
{
  void *ret;
  char *tmp,*tmp2;
  if(name[0]=='_') name++;
#ifdef DLDEBUG
  fprintf(stderr,"DL: lookup_dlls(%s)\n",name);
#endif
#if 1
  if((tmp=STRCHR(name,'@')))
  {
    tmp2=(char *)alloca(tmp - name + 1);
    MEMCPY(tmp2,name,tmp-name);
    tmp2[tmp-name]=0;
#ifdef DLDEBUG
    fprintf(stderr,"DL: Ordinal cutoff: %s -> %s\n",name,tmp2);
#endif
    name=tmp2;
  }
#endif
    
  while(l)
  {
#ifdef DLDEBUG
    char modname[4711];
    GetModuleFileName(l->dll, modname, 4710);
    fprintf(stderr,"Looking for %s in %s ...\n",
	    name,
	    modname);
#endif
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
  HINSTANCE tmp;
#ifdef DLDEBUG
  fprintf(stderr,"append_dlllist(%s)\n",name);
  FLUSH();
#endif
  tmp=low_LoadLibrary(name);
  if(!tmp) return 0;
  n=(struct DLLList *)malloc(sizeof(struct DLLList));
  if(!n)
  {
    dlerr="Out of memory";
    return 0;
  }
  n->dll=tmp;
#ifdef DLDEBUG
  fprintf(stderr,"append_dlllist(%s)->%p\n",name,n->dll);
  FLUSH();
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
#define COFF_SECT_LNK_INFO 0x200
#define COFF_SECT_MEM_LNK_REMOVE (1<<11)
#define COFF_SECT_MEM_DISCARDABLE (1<<25)
#define COFF_SECT_MEM_EXECUTE (1<<29)
#define COFF_SECT_MEM_READ (1<<30)
#define COFF_SECT_MEM_WRITE (1<<31)

#define COFF_SYMBOL_EXTERN 2
#define COFF_SYMBOL_WEAK_EXTERN 105

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


#define COFFReloc_I386_dir32 6
#define COFFReloc_I386_dir32nb 7
#define COFFReloc_I386_sect 10
#define COFFReloc_I386_sectrel 11 
#define COFFReloc_I386_rel32 20

#define COFFReloc_IA64_imm14 1		/* A4 */
#define COFFReloc_IA64_imm22 2		/* A5 */
#define COFFReloc_IA64_imm64 3		/* X2 */
#define COFFReloc_IA64_dir32 4
#define COFFReloc_IA64_dir64 5
#define COFFReloc_IA64_pcrel21b 6	/* M22, M23, B1, B2, B3, B6 */
#define COFFReloc_IA64_pcrel21m 7	/* I20, M20, M21  */
#define COFFReloc_IA64_pcrel21f 8	/* I19, M37, B9, F14, F15 */
#define COFFReloc_IA64_gprel22 9	/* A5 */
#define COFFReloc_IA64_ltoff22 10	/* A5 */
#define COFFReloc_IA64_sect 11		/* */
#define COFFReloc_IA64_secrel22 12	/* A5 */
#define COFFReloc_IA64_secrel64i 13	/* */
#define COFFReloc_IA64_secrel32 14	/* */
#define COFFReloc_IA64_ltoff64 15	/* X2 */
#define COFFReloc_IA64_dir32nb 16	/* */
#define COFFReloc_IA64_addend 31	/* */


/* This structure is correct, but should not be used because of
 * portability issues
 */
struct COFFReloc
{
  INT32 location;
  INT32 symbol;
  INT16 type;
};

union PEAOUT {
  struct {
    struct {
      INT16 magic;
      INT16 version;
      INT32 text_size;
      INT32 data_size;
      INT32 bss_size;
      INT32 entry;
      INT32 text_start;
      INT32 data_start;
    } aout;
    INT32 image_base;
    INT32 section_alignment;
    INT32 file_alignment;
    INT16 major_os_version;
    INT16 minor_os_version;
    INT16 major_image_version;
    INT16 minor_image_version;
    INT16 major_subsys_version;
    INT16 minor_subsys_version;
    INT32 reserved1;
    INT32 size_of_image;
    INT32 size_of_headers;
    INT32 checksum;
    INT16 subsys;
    INT16 dll_flags;
    INT32 size_of_stack_reserve;
    INT32 size_of_stack_commit;
    INT32 size_of_heap_reserve;
    INT32 size_of_heap_commit;
    INT32 loader_flags;
    INT32 number_of_rva_and_sizes;
    INT32 data_directory[16][2];
  } pe32;
  struct {
    struct {
      INT16 magic;
      INT16 version;
      INT32 text_size;
      INT32 data_size;
      INT32 bss_size;
      INT32 entry;
      INT32 text_start;
    } aout;
    INT64 image_base;
    INT32 section_alignment;
    INT32 file_alignment;
    INT16 major_os_version;
    INT16 minor_os_version;
    INT16 major_image_version;
    INT16 minor_image_version;
    INT16 major_subsys_version;
    INT16 minor_subsys_version;
    INT32 reserved1;
    INT32 size_of_image;
    INT32 size_of_headers;
    INT32 checksum;
    INT16 subsys;
    INT16 dll_flags;
    INT64 size_of_stack_reserve;
    INT64 size_of_stack_commit;
    INT64 size_of_heap_reserve;
    INT64 size_of_heap_commit;
    INT32 loader_flags;
    INT32 number_of_rva_and_sizes;
    INT32 data_directory[16][2];
  } pe32plus;
};
#define PEAOUT_GET(P,E) ((P).pe32.aout.magic==0x20b? (P).pe32plus.E : (P).pe32.E)


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

  union PEAOUT *peaout;

  /* temporary storage */
  char **symbol_addresses;
  char **section_addresses;
};


static void *low_dlsym(struct DLHandle *handle,
		       char *name,
		       size_t len,
		       int self)
{
  void *ptr;
  char *tmp=name;
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
  if(!self)
    ptr=htable_get(handle->htable,name,len);
  else
    ptr=0;

  if(!ptr)
  {
    if(name[len])
    {
      tmp=(char *)alloca(len+1);
      MEMCPY(tmp, name, len);
      tmp[len]=0;
    }
    ptr=lookup_dlls(handle->dlls, tmp);
  }
#ifdef DLDEBUG
  if(!ptr)
  {
    fprintf(stderr,"Failed to find identifier %s\n",tmp);
  } else {
    fprintf(stderr, "Found identifier %s at 0x%p\n", tmp, ptr);
  }
#endif
  return ptr;
}
		       
void *dlsym(struct DLHandle *handle, char *name)
{
  return low_dlsym(handle, name, strlen(name), 0);
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
    FLUSH();
  }
#endif

  while(ptr < len)
  {
    int l,x=ptr;
    char *end;

#ifdef DLDEBUG    
    fprintf(stderr,"Parse link info ptr=%d\n",ptr);
    FLUSH();
#endif

    end=MEMCHR(info+ptr,' ',len-ptr);
    if(end) 
      l = DO_NOT_WARN((int)(end - (info+x)));
    else
      l=len-ptr;

#ifdef DLDEBUG    
    fprintf(stderr,"Parse link info ptr=%d len=%d '%c%c%c%c%c%c%c%c'\n",
	    ptr,l,
	    info[x],
	    info[x+1],
	    info[x+2],
	    info[x+3],
	    info[x+4],
	    info[x+5],
	    info[x+6],
	    info[x+7]);
    FLUSH();
#endif

    if((info[x] == '-') || (info[x] == '/'))
    {
      x++;
      if(info[x]=='?') x++;
      if(!memcmp(info+x,"lib:",4) || !memcmp(info+x,"LIB:",4))
      {
	x+=4;
#ifdef DLDEBUG    
	fprintf(stderr,"Found lib: ptr=%d len=%d '%c%c%c%c%c%c%c%c'\n",
		x,
		l-(x-ptr),
		info[x],
		info[x+1],
		info[x+2],
		info[x+3],
		info[x+4],
		info[x+5],
		info[x+6],
		info[x+7]);
	FLUSH();
#endif
	memcpy(buffer,info+x,l-(x-ptr));
	buffer[l-(x-ptr)]=0;
	append_dlllist(&ret->dlls, buffer);
      }
    }
    ptr+=l+1;
  }
#ifdef DLDEBUG    
  fprintf(stderr,"Parse link info done.\n");
  FLUSH();
#endif
}

static int dl_load_coff_files(struct DLHandle *ret,
			      struct DLObjectTempData *tmp,
			      int num)
{
  int e=0,s,r;
  size_t memptr,codeptr;
  size_t num_exports=0;
  int data_protection_mode = PAGE_READWRITE;
#ifdef _M_IA64
  size_t gp_size = 0;
  void **gp = NULL;
  size_t gp_pos = 0;
#endif /* _M_IA64 */

#define data (tmp+e)
#define SYMBOLS(X) (*(struct COFFSymbol *)(18 * (X) + (char *)data->symbols))

#ifdef DLDEBUG
  fprintf(stderr,"dl_load_coff_files(%p,%p,%d)\n",ret,tmp,num);
  FLUSH();
#endif

  if(!num) return 0;

  ret->memsize=0;
  ret->codesize=0;

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

    if(data->coff->sizeof_optheader)
      data->peaout=(union PEAOUT *)(data->coff + 1);
    else
      data->peaout = NULL;

#ifdef DLDEBUG
  fprintf(stderr,"DL: %p %d %d %d\n",
	  data->coff,
	  sizeof(data->coff[0]),
	  data->coff->sizeof_optheader,
	  sizeof(struct COFFsection));
  fprintf(stderr, "DL: num_sections:%d, num_symbols: %d\n",
	  data->coff->num_sections, data->coff->num_symbols);
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
      if(align)
	align=(1<<(align-1))-1;
#ifdef DLDEBUG
      fprintf(stderr, "DL: section[%d,%d], %d bytes, align=%d (0x%x)\n",
	      e, s, data->sections[s].raw_data_size, align+1,
	      data->sections[s].characteristics);
#endif
      if(data->sections[s].characteristics & COFF_SECT_MEM_WRITE)
      {
	if(data->sections[s].characteristics & COFF_SECT_MEM_EXECUTE)
	{
	  /* Data section needs to be executable */
	  data_protection_mode=PAGE_EXECUTE_READWRITE;
	}
	ret->memsize+=align;
	ret->memsize&=~align;
	ret->memsize+=MAXIMUM(data->sections[s].raw_data_size,
			      data->sections[s].virtual_size);
	EFENCE_ADD(ret->memsize);
	ret->memsize+=PAD_DATA;
      }else{
	/* We place all non-writable sections in the code segment
	 * to protect it from accidental writing.
	 */
	ret->codesize+=align;
	ret->codesize&=~align;
	ret->codesize+=MAXIMUM(data->sections[s].raw_data_size,
			       data->sections[s].virtual_size);
      }
    }
    if(data->coff->num_symbols)
    {
      /* symbol table in code section as well */
      ret->codesize += (data->coff->num_symbols + 1) * sizeof(void *) - 1;
      ret->codesize &= ~(sizeof(void *) - 1);
    }

#ifdef _M_IA64
    /* To support ltoff relocations, we need to calculate the size
     * needed for the global offset table (gp).
     *
     * We're conservative, and allocate an entry for every ltoff relocation.
     */

    for(s=0;s<data->coff->num_sections;s++)
    {
      struct COFFReloc *relocs;
      if(data->sections[s].characteristics & 
	 (COFF_SECT_NOLOAD | COFF_SECT_MEM_DISCARDABLE | COFF_SECT_MEM_LNK_REMOVE))
	continue;

      /* relocate all symbols in this section */
      relocs=(struct COFFReloc *)(data->buffer +
				  data->sections[s].ptr2_relocs);

      for(r=0;r<data->sections[s].num_relocs;r++)
      {
/* This should work fine on x86 and other processors which handle
 * unaligned memory access
 */
#define RELOCS(X) (*(struct COFFReloc *)( 10*(X) + (char *)relocs ))

	if (RELOCS(r).type == COFFReloc_IA64_ltoff22) {
	  /* FIXME: Ought to check if the symbol already has an entry. */
	  gp_size++;
	}
#undef RELOCS
      }
    }
#endif /* _M_IA64 */

    /* Count export symbols */
    for(s=0;s<data->coff->num_symbols;s++)
    {
      size_t align;
      if(SYMBOLS(s).class == COFF_SYMBOL_EXTERN ||
	 SYMBOLS(s).class == COFF_SYMBOL_WEAK_EXTERN)
	num_exports++;

      /* Count memory in common symbols,
       * I wonder what the alignment should be?
       */
      if((SYMBOLS(s).class == COFF_SYMBOL_EXTERN ||
	  SYMBOLS(s).class == COFF_SYMBOL_WEAK_EXTERN) &&
	 !SYMBOLS(s).secnum &&
	 SYMBOLS(s).value)
      {
	switch(SYMBOLS(s).value)
	{
	  case 0:
	  case 1: align=0; break;
	  case 2:
	  case 3: align=1; break;
	  case 4: 
	  case 5:
	  case 6:
	  case 7: align=3; break;
	  default: align=7;
	}
	ret->memsize+=align;
	ret->memsize&=~align;
	ret->memsize+=SYMBOLS(s).value;
	EFENCE_ADD(ret->memsize);
	ret->memsize+=PAD_DATA;
      }
    }
  }

#ifdef _M_IA64
#ifdef DLDEBUG
  fprintf(stderr, "Global Offset Table size: %d entries.\n", gp_size);
#endif /* DLDEBUG */

  /* Allocate place in the data segment. */
  ret->memsize += (gp_size + 1)*sizeof(void *)-1;
  ret->memsize &= ~(sizeof(void *)-1);
  EFENCE_ADD(ret->memsize);
  ret->memsize += PAD_DATA;
#endif /* _M_IA64 */


#ifdef DLDEBUG
  fprintf(stderr,"DL: allocating %d bytes\n",ret->memsize);
#endif

  /* Allocate data memory */
  ret->memory=VirtualAlloc(0,
			   ret->memsize,
#ifdef EFENCE
			   MEM_RESERVE,
#else
			   MEM_COMMIT,
#endif
			   data_protection_mode);

  if(!ret->memory && ret->memsize)
  {
    static char buf[300];
    sprintf(buf, "Failed to allocate %d bytes RW(X)-memory.", ret->memsize);
#ifdef DLDEBUG
    fprintf(stderr, "%s\n", buf);
#endif
    dlerr=buf;
    return -1;
  }

  ret->code=VirtualAlloc(0,
			 ret->codesize,
			 MEM_COMMIT,
			 PAGE_READWRITE); /* changed later */

  if(!ret->code && ret->codesize)
  {
    static char buf[300];
    sprintf(buf, "Failed to allocate %d bytes executable memory.", ret->memsize);
#ifdef DLDEBUG
    fprintf(stderr, "%s\n", buf);
#endif
    dlerr=buf;
    return -1;
  }


#ifdef DLDEBUG
  fprintf(stderr,"DL: Got %d bytes RWX memory at %p\n",ret->memsize,ret->memory);
#endif

  /* Create a hash table for exported symbols */
  ret->htable=alloc_htable(num_exports);

  if(data->flags & RTLD_GLOBAL)
    global_dlhandle.htable = htable_add_space(global_dlhandle.htable, num_exports);

#ifdef DLDEBUG
  fprintf(stderr,"DL: moving code\n");
  FLUSH();
#endif

  /* Copy code into executable memory */
  memptr=0;
  codeptr=0;
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
      if(align)
	align=(1<<(align-1))-1;

      if(data->sections[s].characteristics & COFF_SECT_MEM_WRITE)
      {
	memptr+=align;
	memptr&=~align;

	data->section_addresses[s]=
	  EFENCE_ALIGN((char *)ret->memory + memptr,
		       MAXIMUM(data->sections[s].raw_data_size,
			       data->sections[s].virtual_size),
		       align);
	memptr+=MAXIMUM(data->sections[s].raw_data_size,
			data->sections[s].virtual_size);
	EFENCE_ADD(memptr);
	memptr+=PAD_DATA;
      }else{
	codeptr+=align;
	codeptr&=~align;

	data->section_addresses[s]=(char *)ret->code + codeptr;
	codeptr+=MAXIMUM(data->sections[s].raw_data_size,
			data->sections[s].virtual_size);
      }
#ifdef DLDEBUG
      fprintf(stderr,"DL: section[%d]=%p { %d, %d }\n",s,
	      data->section_addresses[s],
	      data->sections[s].ptr2_raw_data,
	      data->sections[s].raw_data_size);
      FLUSH();
#endif

      if(data->sections[s].ptr2_raw_data)
      {
	memcpy((char *)data->section_addresses[s],
	       data->buffer + data->sections[s].ptr2_raw_data,
	       data->sections[s].raw_data_size);
	if(data->sections[s].raw_data_size < data->sections[s].virtual_size)
	{
	  memset((char *)data->section_addresses[s] +
		 data->sections[s].raw_data_size,
		 0,
		 data->sections[s].virtual_size - 
		 data->sections[s].raw_data_size);
	}
      }else{
	memset((char *)data->section_addresses[s],
	       0,
	       MAXIMUM(data->sections[s].raw_data_size,
		       data->sections[s].virtual_size));
      }
    }

    if(data->coff->num_symbols)
    {
      codeptr += sizeof(void *)-1;
      codeptr &= ~(sizeof(void *)-1);
      data->symbol_addresses=(char **)( ((char *)ret->code) + codeptr);
      MEMSET(data->symbol_addresses,
	     0,
	     data->coff->num_symbols * sizeof(void *));
#ifdef DLDEBUG
      fprintf(stderr,"DL: data->symbol_addresses=%p\n",
	      data->symbol_addresses);
#endif
      codeptr += data->coff->num_symbols * sizeof(void *);
    }


    /* Make addresses for common symbols */
    for(s=0;s<data->coff->num_symbols;s++)
    {
      size_t align;
      /* 
       * Setup poiners to common symbols
       */
      if((SYMBOLS(s).class == COFF_SYMBOL_EXTERN ||
	  SYMBOLS(s).class == COFF_SYMBOL_WEAK_EXTERN) &&
	 !SYMBOLS(s).secnum &&
	 SYMBOLS(s).value)
      {
	switch(SYMBOLS(s).value)
	{
	  case 0:
#ifdef DL_VERBOSE
	    fprintf(stderr,"DLOPEN: common symbol with size 0!\n");
#endif
	  case 1: align=0; break;
	  case 2:
	  case 3: align=1; break;
	  case 4: 
	  case 5:
	  case 6:
	  case 7: align=3; break;
	  default: align=7;
	}
	memptr += align;
	memptr &= ~align;

	data->symbol_addresses[s]=
	  EFENCE_ALIGN((char *)ret->memory + memptr,
		       SYMBOLS(s).value,
		       align);
	  
	memset((char *)data->symbol_addresses[s],
	       0,
	       SYMBOLS(s).value);
	memptr+=SYMBOLS(s).value;
	EFENCE_ADD(memptr);
	memptr+=PAD_DATA;
      }
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

#ifdef DL_SYMBOL_LOG
      {
	char *value=0;
	static char buf[10];
	size_t len;
	char *name;

	if(SYMBOLS(s).secnum>0)
	  value=data->section_addresses[SYMBOLS(s).secnum-1]+
	    SYMBOLS(s).value;
	else if(!SYMBOLS(s).secnum && SYMBOLS(s).value)
	  value=data->symbol_addresses[s];

	if(value)
	{
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
	    if(len == 8)
	    {
	      MEMCPY(buf,name,8);
	      buf[8]=0;
	      name=buf;
	    }
	  }
	  if(name[0] != '$')
	    fprintf(dl_symlog,"%p = %s\n", value, name);
	}
      }
#endif

      if(SYMBOLS(s).class == COFF_SYMBOL_EXTERN ||
	 SYMBOLS(s).class == COFF_SYMBOL_WEAK_EXTERN)
      {
	char *value;
	char *name;
	size_t len;

	if(SYMBOLS(s).secnum>0)
	{
	  value=data->section_addresses[SYMBOLS(s).secnum-1]+
	    SYMBOLS(s).value;
	}
	else if(!SYMBOLS(s).secnum && SYMBOLS(s).value)
	{
	  value=data->symbol_addresses[s];
	  if(!value)
	  {
	    fprintf(stderr,"Something is fishy here!!!\n");
	    exit(99);
	  }
	}
	else
	  continue;


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
	
	htable_put(ret->htable, name, len, value,
		   SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN);
	if(data->flags & RTLD_GLOBAL)
	  htable_put(global_dlhandle.htable, name, len, value,
		     SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN);
      }
    }
  }

#ifdef _M_IA64
  /* Set up the global pointer. */

  memptr += sizeof(void *)-1;
  memptr &= ~(sizeof(void *)-1);

  gp = (void **)EFENCE_ALIGN((char *)ret->memory + memptr,
			     gp_size * sizeof(void *),
			     sizeof(void *) - 1);
  memptr += gp_size * sizeof(void *);
  EFENCE_ADD(memptr);
  memptr += PAD_DATA;
#endif /* _M_IA64 */


#ifdef DLDEBUG
  fprintf(stderr,"DL: resolving\n");
  FLUSH();
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

/* This should work fine on x86 and other processors which handle
 * unaligned memory access
 */
#define RELOCS(X) (*(struct COFFReloc *)( 10*(X) + (char *)relocs ))

	char *loc=data->section_addresses[s] + RELOCS(r).location;
	char *ptr;
	ptrdiff_t sym=RELOCS(r).symbol;
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
	fprintf(stderr,"DL: Reloc[%d] sym=%d loc=%d type=%d\n",
		r,
		sym,
		RELOCS(r).location,
		RELOCS(r).type);

	if(!name[len])
	{
	  fprintf(stderr,"DL: symbol name=%s\n",name);
	}else{
	  fprintf(stderr,"DL: symbol name=%c%c%c%c%c%c%c%c\n",
		  name[0],
		  name[1],
		  name[2],
		  name[3],
		  name[4],
		  name[5],
		  name[6],
		  name[7]);
	}
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
	  else if(!SYMBOLS(sym).secnum /* && !SYMBOLS(sym).value */)
	  {
	       
#ifdef DLDEBUG
	    fprintf(stderr,"DL: resolving symbol[%d], type=%d class=%d secnum=%d aux=%d value=%d\n",
		    sym,
		    SYMBOLS(sym).type,
		    SYMBOLS(sym).class,
		    SYMBOLS(sym).secnum,
		    SYMBOLS(sym).aux,
		    SYMBOLS(sym).value);
#endif

#ifdef _M_IA64
	    if ((len == 3) && !memcmp(name, "_gp", 3)) {
	      /* Magic reference to the Global Offset Table. */
	      ptr = (void *)gp;
	    } else {
#endif /* _M_IA64 */

	      ptr=low_dlsym(ret, name, len, 1);
	      if(!ptr) 
		ptr=low_dlsym(&global_dlhandle, name, len, 0);
	      if(ptr)
		data->symbol_addresses[sym]=ptr;
	      else if(len > 6 && !memcmp(name,"__imp_",6)) {
		ptr=low_dlsym(ret, name+6, len-6, 1);
		if(!ptr) 
		  ptr=low_dlsym(&global_dlhandle, name+6, len-6, 0);
		/* This is cheating a bit.  We should really put this
		   value somewhere else, and have symbol_addresses[sym] point
		   to this somewhere else, but this saves allocating the extra
		   memory. */
		if(ptr) {
		  data->symbol_addresses[sym]=ptr;
		  ptr = (char *)&data->symbol_addresses[sym];
		}
	      }
	      if(!ptr)
	      {
		static char err[256];
		MEMCPY(err,"Symbol '",8);
		MEMCPY(err+8,name,MINIMUM(len, 128));
		MEMCPY(err+8+MINIMUM(len, 128),"' not found.\0",13);
		dlerr=err;
#ifndef DL_VERBOSE
		return -1;
#else
		fprintf(stderr,"DL: %s\n",err);
#endif
	      }
#ifdef _M_IA64
	    }
#endif /* _M_IA64 */
	  }else{
	    static char err[200];
#ifdef DLDEBUG
	    fprintf(stderr,"Gnapp??\n");
#endif
	    sprintf(err,"Unknown symbol[%d] type=%d class=%d section=%d value=%d",
		    sym,
		    SYMBOLS(sym).type,
		    SYMBOLS(sym).class,
		    SYMBOLS(sym).secnum,
		    SYMBOLS(sym).value);
	    dlerr=err;
	    return -1;
	  }

	}

#ifndef DL_VERBOSE
	if(!ptr || !data->symbol_addresses)
	{
	  fprintf(stderr,"How on earth did this happen??\n");
	  dlerr="Zero symbol?";
	  return -1;
	}
#endif

	switch(RELOCS(r).type)
	{
	  static char err[100];

#ifdef DL_VERBOSE
#define UNIMPLEMENTED_REL(X)						\
	case X:								\
	  sprintf(err, "Unimplemented relocation type: " #X " (%d)", X);\
	  dlerr = err;							\
	  fprintf(stderr, "DLERR: \"%s\"\n", dlerr);			\
	  return -1
#else /* !DL_VERBOSE */
#define UNIMPLEMENTED_REL(X)						\
	case X:								\
	  sprintf(err, "Unimplemented relocation type: " #X " (%d)", X);\
	  dlerr = err;							\
	  return -1
#endif /* DL_VERBOSE */

#ifdef _M_IA64
	  /* Data relocation. */
	case COFFReloc_IA64_dir64:
#ifdef DLDEBUG
	  fprintf(stderr,"DL: reloc absolute: loc %p = %p\n", loc,ptr);
#endif
	  ((INT64 *)loc)[0]+=(INT64)ptr;
	  break;

	  UNIMPLEMENTED_REL(COFFReloc_IA64_dir32nb);

	default:
	  {
	    /* Instruction relocation. */

	    /* IA64 bundle format:
	     *
	     * 128bit little-endian.
	     *
	     * 22222222 22222222 22222222 22222222
	     * 22222222 21111111 11111111 11111111
	     * 11111111 11111111 11000000 00000000
	     * 00000000 00000000 00000000 000ttttt
	     *
	     * Bundles are 16 byte aligned.
	     */

	    /* Instruction relocation pointers are
	     * stored as a pointer to the bundle in
	     * the upper bits, and sub-instruction
	     * number in the least significant four
	     * bits.
	     */

	    int flag = ((size_t)loc) & 0xf;
	    unsigned INT64 instr = 0;
	    unsigned INT64 S;
	    unsigned INT64 lost_bits;
	    loc = (void *)(((size_t)loc) & ~((size_t)0xf));

	    /* Read sub-instruction.
	     *
	     * The instruction is msb aligned to avoid needing
	     * to shift in case 2.
	     *
	     * Note that the low order bits are kept as is to
	     * simplify the write-back.
	     *
	     * lost_bits is used to retain the high order bits
	     * that are lost due to the shift.
	     */
	    switch (flag) {
	    case 0:
	      lost_bits = ((unsigned INT64 *)loc)[0];
	      instr = lost_bits << 18;
	      break;
	    case 1:
	      lost_bits = ((unsigned INT32 *)loc)[2];
	      instr = (lost_bits << 41) |
		(((unsigned INT64)((unsigned INT32 *)loc)[1]) << 9);
	      break;
	    case 2:
	      instr = ((unsigned INT64 *)loc)[1];
	      break;
	    default:
	      sprintf(err, "Unsupported relocation: %d, 0x%p, %d",
		      RELOCS(r).type, loc, flag);
	      dlerr = err;
#ifdef DL_VERBOSE
	      fprintf(stderr, "DLERR: \"%s\"\n", dlerr);
#endif /* DL_VERBOSE */
	      return -1;
	    }
#ifdef DLDEBUG
	    fprintf(stderr, "DL: instr 0x%p\n", (void *)(instr >> 23));
#endif
	    switch (RELOCS(r).type) {
	      /* We will need to support more types here */

	      UNIMPLEMENTED_REL(COFFReloc_IA64_imm14);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_imm22);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_imm64);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_dir32);

	    case COFFReloc_IA64_pcrel21b:
	      /* Instruction format M22, M23, B1, B2, B3, B6
	       *
	       * 43333333 33322222 22222111 11111110 00000000 0
	       * 09876543 21098765 43210987 65432109 87654321 0
	       * XXXX.XXX ........ ........ ....XXXX XXXXXXXX X
	       *
	       * S += ptr - loc
	       */
	      S = (instr & 0x00fffff000000000) + ((ptr - loc)<<(23 - 4));
	      if (S & 0x0100000000000000) {
		/* Got carry, adjust sign. */
		instr ^= 0x0800000000000000;
	      }
	      instr = (instr & ~0x00fffff000000000) | (S & 0x00fffff000000000);
	      break;

	      UNIMPLEMENTED_REL(COFFReloc_IA64_pcrel21m);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_pcrel21f);

	    case COFFReloc_IA64_gprel22:
	      /* Instruction format A5
	       *
	       * 43333333 33322222 22222111 11111110 00000000 0
	       * 09876543 21098765 43210987 65432109 87654321 0
	       * XXXX.... ........ ...XX... ....XXXX XXXXXXXX X
	       *
	       * S += ptr - gp;
	       */
	      S = (ptr - (char *)gp) +
		(((instr & 0x0fffe00000000000) >> 38)|
		 ((instr & 0x000007f000000000) >> 36));
	      instr = (instr & ~0x0fffe7f000000000) |
		((S & 0x3fff80)<<38)|((S & 0x7f)<<36);
#ifdef DLDEBUG
	      if ((S & ~0x3fffff) + ((S & 0x200000)<<1)) {
		fprintf(stderr, "DL: gp relative offset > 22 bits: 0x%p\n",
			(void *)S);
	      }
#endif /* DLDEBUG */
	      break;

	    case COFFReloc_IA64_ltoff22:
	      /* Instruction format A5
	       *
	       * 43333333 33322222 22222111 11111110 00000000 0
	       * 09876543 21098765 43210987 65432109 87654321 0
	       * XXXX.... ........ ...XX... ....XXXX XXXXXXXX X
	       *
	       * gp[gp_pos] = S + ptr;
	       * S = gp_pos++;
	       */

	      gp[gp_pos] = ((unsigned INT64) ptr) +
		(((instr & 0x0fffe00000000000) >> 38)|
		 ((instr & 0x000007f000000000) >> 36));

	      /* Note: multiplies gp_pos with 8 in the same operation. */
	      instr = (instr & ~0x0fffe7f000000000) |
		((gp_pos & 0x07fff0)<<41) | ((gp_pos & 0x00000f)<<39);
	      gp_pos++;
	      break;

	      UNIMPLEMENTED_REL(COFFReloc_IA64_sect);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_secrel22);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_secrel64i);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_secrel32);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_ltoff64);
	      UNIMPLEMENTED_REL(COFFReloc_IA64_addend);

	    default:
	      sprintf(err,"Unknown relocation type: %d",RELOCS(r).type);
	      dlerr=err;
#ifdef DL_VERBOSE
	      fprintf(stderr, "DLERR: \"%s\"\n", dlerr);
#endif /* DL_VERBOSE */
	      return -1;
	    }
#ifdef DLDEBUG
	    fprintf(stderr, "DL: relocated instr 0x%p\n",
		    (void *)(instr >> 23));
#endif
	    /* Store back the modified instruction. */
	    switch(flag) {
	    case 0:
	      ((unsigned INT64 *)loc)[0] = (lost_bits & 0xffffc00000000000) |
		(instr >> 18);
	      break;
	    case 1:
	      ((unsigned INT32 *)loc)[1] = (instr >> 9);
	      ((unsigned INT32 *)loc)[2] = (lost_bits & ~0x7fffff) |
		(instr >> 41);
	      break;
	    case 2:
	      ((unsigned INT64 *)loc)[1] = instr;
	      break;
	    }
	  }
	  break;
#elif defined(_M_IX86)

	    /* We may need to support more types here */
	case COFFReloc_I386_dir32:
#ifdef DLDEBUG
	  fprintf(stderr,"DL: reloc absolute: loc %p = %p\n", loc,ptr);
#endif
	  ((INT32 *)loc)[0]+=(INT32)ptr;
	  break;

#if 0
	case COFFReloc_I386_dir32nb:
#ifdef DLDEBUG
	  fprintf(stderr,"DL: reloc absolute nb: loc %p = %p\n", loc,ptr);
#endif
	  ((int32 *)loc)[0]+=((INT32)ptr) - global_imagebase;
	  break;	   
#endif

	case COFFReloc_I386_rel32:
#ifdef DLDEBUG
	  fprintf(stderr,"DL: reloc relative: loc %p = %p = %d\n",
		  loc,
		  ptr,
		  ptr - (loc+sizeof(INT32)));
#endif
	  ((INT32*)loc)[0]+=ptr - (loc+sizeof(INT32));
	  break;


	default:
	  sprintf(err,"Unknown relocation type: %d",RELOCS(r).type);
	  dlerr=err;
#ifdef DL_VERBOSE
	  fprintf(stderr, "DLERR: \"%s\"\n", dlerr);
#endif /* DL_VERBOSE */
	  return -1;
#endif /* _M_IA64 || _M_IX86 */
	}
      }

#ifdef DL_VERBOSE
      if(dlerr)
	return -1;
#endif

    }
  }

  for(e=0;e<num;e++)
    free((char *) data->section_addresses);

  if(ret->code)
  {
    DWORD oldprotect;
#if defined(_M_IA64) && 0
    extern void (*ia64_flush_instruction_cache)(void *addr, size_t len);
    /* Flush the instruction cache. */
    ia64_flush_instruction_cache(ret->code, ret->codesize);
#endif /* _M_IA64 && 0 */
    if(!VirtualProtect(ret->code,
		       ret->codesize,
		       PAGE_EXECUTE_READ,
		       &oldprotect))
    {
      dlerr="Failed to set memory executable";
      return -1;
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
  FLUSH();
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
  if(!tmp)
  {
    dlerr="Failed to allocate temporary storage";
    return -1;
  }
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
  FLUSH();
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
  FLUSH();
#endif

  if(data->buflen > 8 && ! memcmp(data->buffer,"!<arch>\n",8))
    return dl_loadarchive(ret,data);

  tmp=i4(0x3c);
  if(tmp >=0 && tmp<data->buflen-4 &&!memcmp(data->buffer+tmp,"PE\0\0",4))
    return dl_loadpe(ret, data);

  return dl_loadcoff(ret, data);
}

static void init_dlopen(void);

struct DLHandle *dlopen(const char *name, int flags)
{
  struct DLHandle *ret;
  struct DLTempData tmpdata;
  int retcode;
  tmpdata.flags=flags;

  if(!global_dlhandle.htable) init_dlopen();

  if(!name)
  {
    global_dlhandle.refs++;
    return &global_dlhandle;
  }

#ifdef DLDEBUG
  fprintf(stderr,"dlopen(%s,%d)\n",name,flags);
#endif

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
  ACCEPT_MEMORY_LEAK(ret);
  ret->refs=1;
  ret->filename=strdup(name);
  ACCEPT_MEMORY_LEAK(ret->filename);
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
  ret->next=first;
  first=ret;


#ifdef DL_SYMBOL_LOG
  fflush(dl_symlog);
#endif

  return ret;
}

int dlclose(struct DLHandle *h)
{
#ifdef DLDEBUG
  fprintf(stderr,"Dlclose(%s)\n",h->filename);
#endif
  if(! --h->refs)
  {
    struct DLHandle **ptr;
    for(ptr=&first;*ptr;ptr=&(ptr[0]->next))
    {
      if(*ptr == h)
      {
	*ptr=h->next;
	break;
      }
    }
    
    if(h->filename) free(h->filename);
    if(h->htable) htable_free(h->htable,0);
    if(h->dlls) dlllist_free(h->dlls);
    if(h->memory) VirtualFree(h->memory,h->memsize,MEM_RELEASE);
    if(h->code) VirtualFree(h->code,h->codesize,MEM_RELEASE);
    free((char *)h);
  }
  return 0;
}

#ifdef DEBUG_MALLOC
#ifndef __NT__
#define __cdecl
#endif

void * __cdecl dlopen_malloc_wrap(size_t size)
{
  return malloc(size);
}

void * __cdecl dlopen_realloc_wrap(void *m, size_t size)
{
  return realloc(m, size);
}

void * __cdecl dlopen_calloc_wrap(size_t size,size_t num)
{
  return calloc(size,num);
}

void __cdecl dlopen_free_wrap(void * size)
{
  free(size);
}

void * __cdecl dlopen_strdup_wrap(const char *s)
{
  return strdup(s);
}
#endif

#ifdef DL_WRAP_STDIO
int __cdecl dlopen_fseek_wrapper(FILE *f, long pos, int mode)
{
  fprintf(stderr,"fseek(%p,%ld,%d)\n",f,pos,mode);
  return fseek(f,pos,mode);
}

FILE *__cdecl dlopen_fopen_wrapper(const char *fname, const char *mode)
{
  FILE *ret=fopen(fname,mode);
  fprintf(stderr,"fopen(\"%s\",\"%s\") => %p\n",fname,mode,ret);
  return ret;
}
#endif

#ifdef USE_PDB_SYMBOLS

/* IA64 PDB (Program Data Base) support. */

/*
 * PDB file format:
 *
 * All values are stored in little-endian.
 *
 * PDB Format Identifier
 *   The file begins with a 0x1a (aka EOF) terminated string
 *   describing the program that generated the file.
 *   This is followed by two byte tag specifying the fileformat.
 *   Only format 0x44 0x55 is currently supported.
 *   The file is then padded to an even multiple of 4 bytes.
 *
 * PDB Header
 *   Then follows the PDB header.
 */

struct pdb_header {
  INT32 blocksize;	/* Blocksize */
  INT32 freelist;
  INT32 total_alloc;	/* Total number of blocks in the file */
  INT32 toc_size;	/* Size in bytes of the Table Of Contents. */
  INT32 unknown;
  INT32 toc_loc;	/* Block number of the TOC block table. */
};

/* Read a sequence of blocks from the PDB file buf. */
static void *read_pdb_blocks(unsigned char *buf, size_t len,
			     INT32 *blocklist, size_t nbytes,
			     struct pdb_header *header)
{
  unsigned char *ret, *p;

#ifdef DLDEBUG
  fprintf(stderr, "read_pdb_blocks(0x%p, %ld, 0x%p, %ld, X)...\n",
	  buf, len, blocklist, nbytes);
#endif /* DLDEBUG */

  if(len<1 || !(ret = p = malloc(nbytes))) return NULL;
  while(nbytes>0) {
    size_t chunk = (nbytes>header->blocksize? header->blocksize : nbytes);
    size_t offs = header->blocksize**(blocklist++);
    if(offs + chunk > len) { free(ret); return NULL; }
    memcpy(p, buf+offs, chunk);
    p += chunk;
    nbytes -= chunk;
  }
#ifdef DLDEBUG
  fprintf(stderr, "OK, data at 0x%p\n", ret);
#endif /* DLDEBUG */
  return ret;
}

/* Table Of Contents
 *
 * The table of contents consists of 3 sections.
 *
 *   INT32 number_of_files;
 *
 *   INT32 byte_lengths[number_of_files];
 *
 *   INT32 concatenated_block_tables[];
 */

/* Read file #n specified in the toc from the PDB file buf. */
static void *read_pdb_file(unsigned char *buf, size_t *plen,
			   INT32 *toc, int n, struct pdb_header *header)
{
  INT32 *fsz = toc+1;
  INT32 *blocklist = fsz+*toc;
  size_t len = *plen;
  int i;
  if(n<0 || n>=*toc) return NULL;
  for(i=0; i<n; i++) {
    int f_blocks = (*fsz+++header->blocksize-1)/header->blocksize;
    blocklist += f_blocks;
  }
  if(4+4*((blocklist-toc)+(*fsz+header->blocksize-1)/header->blocksize) >
     header->toc_size) 
    return NULL;
#ifdef DLDEBUG
  fprintf(stderr, "Reading PDB file %d at block %d len %d\n",
	  n, *blocklist, *fsz);
#endif
  return read_pdb_blocks(buf, len, blocklist, *plen = *fsz, header);
}

/* PDB file #1 contains the root header. */

struct pdb_root_header {
  INT32 version;	/* Root header version number (20000404). */
  INT32 timestamp;	/* Creation date (seconds since 1970). */
  INT32 unknown;
  INT32 names;
};

/* PDB file #3 contains the symbols header. */

struct pdb_symbols_header {
  INT32 signature;	/* Marker to indicate versioned header (-1). */
  INT32 version;	/* Symboltable version number (19990903). */
  INT32 unknown;	/* Symbols header file number? (3). */
  INT32 hash1_file;	/* File number */
  INT32 hash2_file;	/* File number */
  INT32 gsym_file;	/* File number of the global symbol table. */
  INT32 module_size, offset_size, hash_size;
  INT32 srcmodule_size, pdbimport_size;      
};

/* Global Symbol Table
 *
 * This table consists of entries with the format
 *
 *   INT16 len;
 *   INT8 data[len];
 *
 * Each data block starts with an INT16 identifing the type.
 *
 * Currently only the following data blocks are understood:
 */

struct cv_global_symbol {
  unsigned INT16 len;		/* Entry length */
  unsigned INT16 id;		/* 0x110e */
  unsigned INT32 type;		/* Symbol type */
  unsigned INT32 value;		/* Symbol value */
  unsigned INT16 secnum;	/* Section number (>= 1). */
  char name[1];			/* Symbol name */
};

static unsigned char *find_pdb_symtab(unsigned char *buf, size_t *plen)
{
  struct pdb_header *header;
  struct pdb_symbols_header *st;
  size_t stlen, len = *plen;
  int gsym_file, idlen=0;
  INT32 *toc;
  unsigned char *ret;
  while(idlen<256 && idlen<len && buf[idlen]!=0x1a) idlen++;
  if(idlen>=256 || idlen>=len) return NULL;
#ifdef DLDEBUG
  buf[idlen]='\0';
  fprintf(stderr, "Found PDB header, ident=\"%s\", sig=<%x,%x,%x,%x>\n",
	  buf,  buf[idlen+1],  buf[idlen+2], buf[idlen+3],  buf[idlen+4]);
#endif
  if(len<idlen+3 || buf[idlen+1]!=0x44 || buf[idlen+2]!=0x53) return NULL;
  idlen += 5;
  if(idlen&3) idlen+=4-(idlen&3);
  header = (void*)(buf+idlen);
#ifdef DLDEBUG
  fprintf(stderr, "header at:0x%p (offset:%d)\n", header, idlen);
  fprintf(stderr,
	  "blocksize:  %d\nfreelist:   %d\ntotalalloc: %d\n"
	  "toc_size:   %d\nunknown:    %d\ntoc_loc:    %d\n",
	  header->blocksize, header->freelist, header->total_alloc,
	  header->toc_size, header->unknown, header->toc_loc);
#endif /* DLDEBUG */
  if(idlen+sizeof(*header)>len || header->blocksize==0 ||
     header->toc_size < 20) return NULL;
  toc = read_pdb_blocks(buf, len,
			(INT32*)(buf+header->toc_loc*header->blocksize),
			header->toc_size, header);
  if(!toc) return NULL;
  if(*toc < 4 || header->toc_size < 4+4**toc) { free(toc); return NULL; }

  stlen = len;
  if(!(st = read_pdb_file(buf, &stlen, toc, 3, header))) {
    free(toc); return NULL;
  }
  if(stlen < sizeof(*st) || st->signature != -1) {
    free(st); free(toc); return NULL;
  }
  gsym_file = st->gsym_file;
  free(st);
  
#ifdef DLDEBUG
  fprintf(stderr, "Located PDB global symtab as file %d\n", gsym_file);
#endif
  ret = read_pdb_file(buf, plen, toc, gsym_file, header);
  free(toc);
  return ret;
}
#endif

static void init_dlopen(void)
{
  static char pike_path[MAX_PATH];
  INT32 offset;
  struct DLObjectTempData objtmp;
  HINSTANCE h;
  DWORD c;

  c=GetModuleFileName(NULL, pike_path, MAX_PATH);
  assert(c > 0);

#ifdef DLDEBUG
  fprintf(stderr,"dlopen_init(%s)\n",pike_path);
#endif

#ifdef DL_SYMBOL_LOG
  dl_symlog=fopen("dlopen_symlog","w");
#endif

  global_dlhandle.refs=1;
  global_dlhandle.filename=pike_path;
  global_dlhandle.next=0;
  first=&global_dlhandle;
  
  h=GetModuleHandle(NULL);

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
    
    
    buf= (unsigned char *)read_file(pike_path, &len);
    
    data->symbols=(struct COFFSymbol *)(buf +
					data->coff->symboltable);
    
    data->stringtable=(unsigned char *)( ((char *)data->symbols) + 
					 18 * data->coff->num_symbols);
    

    if(data->coff->sizeof_optheader) {

      data->peaout=(union PEAOUT *)(data->coff + 1);

      global_imagebase = PEAOUT_GET(*data->peaout, image_base);

    } else

      data->peaout = NULL;

#if defined(PIKE_DEBUG) && !defined(USE_PDB_SYMBOLS)
    if(!data->coff->num_symbols)
      Pike_fatal("No COFF symbols found in pike binary.\n");
#endif      
    global_dlhandle.htable=alloc_htable(data->coff->num_symbols);
    
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
      if(SYMBOLS(s).class != COFF_SYMBOL_EXTERN &&
	 SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN) continue;
      if(SYMBOLS(s).secnum <= 0) continue;
      
      if(!SYMBOLS(s).name.ptr[0])
      {
	name=data->stringtable + SYMBOLS(s).name.ptr[1];
	if(name[0]=='?') continue; /* C++ garbled */
	len = DO_NOT_WARN((int)strlen(name));
#ifdef DLDEBUG
	fprintf(stderr,"Sym[%04d] %s : ",s,name);
#endif
      }else{
	name=SYMBOLS(s).name.text;
	if(name[0]=='?') continue; /* C++ garbled */
	len = DO_NOT_WARN((int)STRNLEN(name,8));
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
      
      htable_put(global_dlhandle.htable, name, len,
		 data->buffer +
		 data->sections[SYMBOLS(s).secnum-1].virtual_addr -
		 data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
		 SYMBOLS(s).value,
		 SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN);		   
      
    }
    free(buf);
#ifdef USE_PDB_SYMBOLS
    if(strlen(pike_path)>4) {
      char *pdb_name = alloca(strlen(pike_path)+1);
      strcpy(pdb_name, pike_path);
      strcpy(pdb_name+strlen(pdb_name)-4, ".PDB");
      buf= (unsigned char *)read_file(pdb_name, &len);
    } else buf = NULL;
    if(buf) {
      unsigned char *symtab = find_pdb_symtab(buf, &len);
      if(symtab) {
	size_t i;
#ifdef DLDEBUG
	fprintf(stderr,"Loading PDB symbols (%d bytes)\n", len);
#endif
	for(i=0; i<len; i+=2+*(unsigned INT16 *)&symtab[i])
	  if(*(unsigned INT16 *)&symtab[i+2] == 0x110e) {
	    struct cv_global_symbol *sym = (void *)&symtab[i];
	    size_t namelen=STRNLEN(sym->name,sym->len-12);
	  
#ifdef DLDEBUG
	    if(!sym->name[namelen])
	      fprintf(stderr,"Sym[%04d] %s : ",s,sym->name);
	    else
	      fprintf(stderr,"Sym[%04d] %c%c%c%c%c%c%c%c = ",s,
		      sym->name[0],
		      sym->name[1],
		      sym->name[2],
		      sym->name[3],
		      sym->name[4],
		      sym->name[5],
		      sym->name[6],
		      sym->name[7]);
	    fprintf(stderr,"sect=%d value=%d type=%d",
		    sym->secnum,
		    sym->value,
		    sym->type);
#endif
	    htable_put(global_dlhandle.htable, sym->name, namelen,
		       data->buffer +
		       data->sections[sym->secnum-1].virtual_addr -
		       data->sections[sym->secnum-1].ptr2_raw_data +
		       sym->value,
		       1 /* Fixme? Weak symbols... */);
	  }
	free(symtab);
      }
      free(buf);
    }
#endif
  }else{
#ifdef DLDEBUG
    fprintf(stderr,"Couldn't find PE header.\n");
#endif
    append_dlllist(&global_dlhandle.dlls, pike_path);
    global_dlhandle.htable=alloc_htable(997);

#define EXPORT(X) \
  DO_IF_DLDEBUG( fprintf(stderr,"EXP: %s\n",#X); ) \
  htable_put(global_dlhandle.htable, SYMBOL_PREFIX #X, \
	     sizeof(SYMBOL_PREFIX #X)-sizeof(""), &X, 1)
    
    
    fprintf(stderr,"Fnord, rand()=%d\n",rand());
    EXPORT(fprintf);
    EXPORT(_iob);
    EXPORT(abort);
    EXPORT(rand);
    EXPORT(srand);
    EXPORT(getc);
    EXPORT(ungetc);
    EXPORT(printf);
    EXPORT(perror);
    EXPORT(sscanf);
    EXPORT(abs);
    EXPORT(putchar);
    EXPORT(putenv);
    EXPORT(strncat);
    EXPORT(fdopen);
    EXPORT(fstat);
    EXPORT(_assert);
    EXPORT(qsort);
    EXPORT(exp);
    EXPORT(log);
    EXPORT(sqrt);
    EXPORT(fopen);
    EXPORT(fseek);
    EXPORT(fread);
    EXPORT(strtol);
    EXPORT(rewind);
    EXPORT(fputs);
    EXPORT(freopen);
    EXPORT(memcmp);
  }
#if 0
#ifdef _M_IA64
  {
    extern void *_gp[];
    EXPORT(_gp);
  }
#endif
#endif /* 0 */

#define EXPORT_AS(X,Y) \
  DO_IF_DLDEBUG( fprintf(stderr,"EXP: %s = %s\n",#X,#Y); ) \
  htable_put(global_dlhandle.htable, SYMBOL_PREFIX #X, \
	     sizeof(SYMBOL_PREFIX #X)-sizeof(""), &Y, 1)

#if 0
  /* This doesn't work, don't know why - Hubbe */
#ifdef DEBUG_MALLOC
  EXPORT_AS(malloc,dlopen_malloc_wrap);
  EXPORT_AS(calloc,dlopen_calloc_wrap);
  EXPORT_AS(free,dlopen_free_wrap);
  EXPORT_AS(realloc,dlopen_free_wrap);
  EXPORT_AS(strdup,dlopen_strdup_wrap);
#endif
#endif

#ifdef DL_WRAP_STDIO
  EXPORT_AS(fseek, dlopen_fseek_wrapper);
  EXPORT_AS(fopen, dlopen_fopen_wrapper);
#endif

#ifdef EFENCE
  {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    dlopen_page_size=sysinfo.dwPageSize;
    fprintf(stderr,"Activating EFENCE, page_size=%d\n",dlopen_page_size);
    fflush(stderr);
  }
#endif

#ifdef PIKE_DEBUG
  if(global_imagebase != (INT32)data->buffer)
    Pike_fatal("LoadLibrary(ARGV[0]) didn't return instantiated program.\n");
#endif 

#ifdef DLDEBUG
  fprintf(stderr,"DL: init done\n");
#endif
}

/* Strap to load symbols that might be needed from modules. */
#ifdef HAVE__ALLDIV
void _alldiv(void);
#endif
#ifdef HAVE__AULLSHR
void _aullshr(void);
#endif
void dl_dummy(void)
{
#ifdef HAVE__ALLDIV
  _alldiv();
#endif

#ifdef HAVE__AULLSHR
  _aullshr();
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
  l=low_LoadLibrary(argv[0]);
  fprintf(stderr,"LoadLibrary(%s) => %p\n",argv[0],(long)l);
  addr = GetProcAddress(l, "func1");
  fprintf(stderr,"GetProcAddress(\"func1\") => %p (&func1 = %p)\n",addr, func1);
#else

  {
    HINSTANCE h=low_LoadLibrary(argv[0]);
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
      

      global_dlhandle.htable=alloc_htable(data->coff->num_symbols);

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
	if(SYMBOLS(s).class != COFF_SYMBOL_EXTERN &&
	   SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN) continue;
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

	  htable_put(global_dlhandle.htable, name, len,
		     data->buffer +
		     data->sections[SYMBOLS(s).secnum-1].virtual_addr -
		     data->sections[SYMBOLS(s).secnum-1].ptr2_raw_data +
		     SYMBOLS(s).value,
		     SYMBOLS(s).class != COFF_SYMBOL_WEAK_EXTERN);	   

      }
      free(buf);

#ifdef USE_PDB_SYMBOLS
      if(strlen(argv[0]>4)) {
	char *pdbname = alloca(strlen(argv[0])+1);
	strcpy(pdbname, argv[0]);
	strcpy(pdbname+strlen(pdbname)-4, ".PDB");
	buf= (unsigned char *)read_file(pdb_name, &len);
      } else buf = NULL;
      if(buf) {
	unsigned char *symtab = find_pdb_symtab(buf, &len);
	if(symtab) {
	  size_t i;
#ifdef DLDEBUG
	  fprintf(stderr,"Loading PDB symbols (%d bytes)\n", len);
#endif
	  for(i=0; i<len; i+=2+*(unsigned INT16 *)&symtab[i])
	    if(*(unsigned INT16 *)&symtab[i+2] == 0x110e) {
	      struct {
		unsigned INT16 len, id;
		unsigned INT32 type, value;
		unsigned INT16 secnum;
		char name[1];
	      } *sym = (void *)&symtab[i];
	      int namelen=STRNLEN(sym->name,sym->len-12);
	      
#ifdef DLDEBUG
	      if(!sym->name[namelen])
		fprintf(stderr,"Sym[%04d] %s : ",s,sym->name);
	      else
		fprintf(stderr,"Sym[%04d] %c%c%c%c%c%c%c%c = ",s,
			sym->name[0],
			sym->name[1],
			sym->name[2],
			sym->name[3],
			sym->name[4],
			sym->name[5],
			sym->name[6],
			sym->name[7]);
	      fprintf(stderr,"sect=%d value=%d type=%d",
		      sym->secnum,
		      sym->value,
		      sym->type);
#endif
	      htable_put(global_dlhandle.htable, sym->name, namelen,
			 data->buffer +
			 data->sections[sym->secnum-1].virtual_addr -
			 data->sections[sym->secnum-1].ptr2_raw_data +
			 sym->value,
			 1 /* Fixme? Weak symbols... */);
	    }
	  free(symtab);
	}
	free(buf);
      }
#endif
    }else{
#ifdef DLDEBUG
      fprintf(stderr,"Couldn't find PE header.\n");
#endif
      append_dlllist(&global_dlhandle.dlls, argv[0]);
      global_dlhandle.htable=alloc_htable(997);
#define EXPORT(X) \
  DO_IF_DLDEBUG( fprintf(stderr,"EXP: %s\n",#X); ) \
  htable_put(global_dlhandle.htable, SYMBOL_PREFIX #X, \
	     sizeof(SYMBOL_PREFIX #X)-sizeof(""), &X, 1)
      
      
      fprintf(stderr,"Fnord, rand()=%d\n",rand());
      EXPORT(fprintf);
      EXPORT(_iob);
      EXPORT(abort);
      EXPORT(rand);
      EXPORT(srand);
    }
#if 0
#ifdef _M_IA64
    {
      extern void *_gp[];
      EXPORT(_gp);
    }
#endif
#endif /* 0 */
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
