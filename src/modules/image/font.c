/* $Id: font.c,v 1.7 1996/11/18 23:13:21 hubbe Exp $ */

#include "global.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#include <netinet/in.h>
#include <errno.h>

#include "config.h"

#include "stralloc.h"
#include "types.h"
#include "macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "array.h"

#include "image.h"
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif


static struct program *font_program;
extern struct program *image_program;

#define THIS (*(struct font **)(fp->current_storage))
#define THISOBJ (fp->current_object)

struct font 
{
   unsigned long height;      /* height of character rectangles */
   unsigned long baseline;    /* baseline of characters */
#ifdef HAVE_MMAP
   unsigned long mmaped_size; /* if 0 - not mmaped: just free() mem */
#endif
   void *mem;         /* pointer to mmaped/malloced memory */
   unsigned long chars;       /* number of characters */
   struct _char      
   {
      unsigned long width;   /* character rectangle has this width in pixels */
      unsigned long spacing; /* pixels to next character */
      unsigned char *pixels; /* character rectangle */
   } charinfo [1];
};

/***************** init & exit *********************************/

static inline void free_font_struct(struct font *font)
{
   if (font)
   {
      if (font->mem)
      {
#ifdef HAVE_MMAP
	 munmap(font->mem,font->mmaped_size);
#else
	 free(font->mem);
#endif
      }
      free(font);
   }
}

static void init_font_struct(struct object *o)
{
  THIS=NULL;
}

static void exit_font_struct(struct object *obj)
{
   free_font_struct(THIS);
   THIS=NULL;
}

/***************** internals ***********************************/


static inline int my_read(int from, char *buf, int towrite)
{
  int res;
  while((res = read(from, buf, towrite)) < 0)
  {
    switch(errno)
    {
     case EAGAIN: case EINTR:
      continue;

     default:
      res = 0;
      return 0;
    }
  }
  return res;
}

static inline long file_size(int fd)
{
  struct stat tmp;
  int res;
  if(!fstat(fd, &tmp)) return res = tmp.st_size;
  return -1;
}

static inline write_char(struct _char *ci,
			 rgb_group *pos,
			 INT32 xsize,
			 INT32 height)
{
   rgb_group *nl;
   INT32 x,y;
   unsigned char *p;

   p=ci->pixels;

   for (y=height; y>0; y--)
   {
      nl=pos+xsize;
      for (x=(INT32)ci->width; x>0; x--)
      {
	 register char c;
	 c=255-*p;
	 if (pos->r==0)
	    pos->r=pos->g=pos->b=c;
	 else if (pos->r+c>255)
	    pos->r=pos->g=pos->b=255;
	 else
	 {
	    pos->r+=c;
	    pos->g+=c;
	    pos->b+=c;
	 }
	 pos++;
	 p++;
      }
      pos=nl;
   }
}

/***************** methods *************************************/


void font_load(INT32 args)
{
   int fd;

   if (args<1 
       || sp[-args].type!=T_STRING)
      error("font->read: illegal or wrong number of arguments\n");
   
   if (THIS)
   {
      free_font_struct(THIS);
      THIS=NULL;
   }
   do {
     fd = open(sp[-args].u.string->str,O_RDONLY);
   } while(fd < 0 && errno == EINTR);

   if (fd >= 0)
   {
      long size;
      size = file_size(fd);
      if (size > 0)
      {
	 THIS=(struct font *)xalloc(sizeof(struct font));
#ifdef HAVE_MMAP
	 THIS->mem = 
	    mmap(0,size,PROT_READ,MAP_SHARED,fd,0);
	 THIS->mmaped_size=size;
#else
	 THIS->mem = malloc(size);
	 if (THIS->mem)
	    my_read(fd,THIS->mem,size);
#endif
	 if (THIS->mem)
	 {
	    int i;

	    struct file_head 
	    {
	       unsigned INT32 cookie;
	       unsigned INT32 version;
	       unsigned INT32 chars;
	       unsigned INT32 height;
	       unsigned INT32 baseline;
	       unsigned INT32 o[1];
	    } *fh;
	    struct char_head
	    {
	       unsigned INT32 width;
	       unsigned INT32 spacing;
	       unsigned char data[1];
	    } *ch;

	    fh=(struct file_head*)THIS->mem;

	    if (ntohl(fh->cookie)==0x464f4e54) /* "FONT" */
	    {
	       if (ntohl(fh->version)==1)
	       {
		  unsigned long i;
		  struct font *new;

		  THIS->chars=ntohl(fh->chars);

		  new=malloc(sizeof(struct font)+
			     sizeof(struct _char)*(THIS->chars-1));
		  new->mem=THIS->mem;
#ifdef HAVE_MMAP
		  new->mmaped_size=THIS->mmaped_size;
#endif
		  new->chars=THIS->chars;
		  free(THIS);
		  THIS=new;

		  THIS->height=ntohl(fh->height);
		  THIS->baseline=ntohl(fh->baseline);

		  for (i=0; i<THIS->chars; i++)
		  {
		     ch=(struct char_head*)
			((char *)(THIS->mem)+ntohl(fh->o[i]));
		     THIS->charinfo[i].width = ntohl(ch->width);
		     THIS->charinfo[i].spacing = ntohl(ch->spacing);
		     THIS->charinfo[i].pixels = ch->data;
		  }

		  close(fd);
		  pop_n_elems(args);
		  THISOBJ->refs++;
		  push_object(THISOBJ);   /* success */
		  if (THIS->chars>32)
		     THIS->charinfo[32].spacing=THIS->height/4;
		  return;
	       } /* wrong version */
	    } /* wrong cookie */
	 } /* mem failure */
	 free_font_struct(THIS);
	 THIS=NULL;
      } /* size failure */
      close(fd);
   } /* fd failure */

   pop_n_elems(args);
   push_int(0);
   return;
}

void font_write(INT32 args)
{
   struct object *o;
   struct image *img;
   INT32 xsize,i,maxwidth,c,maxwidth2,j;

   if (!THIS)
      error("font->write: no font loaded\n");

   maxwidth2=0;

   for (j=0; j<args; j++)
   {
     if (sp[j-args].type!=T_STRING)
       error("font->write: illegal argument(s)\n");
     
     xsize = 0;
     maxwidth = 0;
     
     for (i = 0; i < sp[j-args].u.string->len; i++)
     {
       c=EXTRACT_UCHAR(sp[j-args].u.string->str+i);
       if (c < (INT32)THIS->chars)
       {
	 if (xsize + (signed long)THIS->charinfo[c].width > maxwidth)
	   maxwidth = xsize + THIS->charinfo[c].width;
	 xsize += THIS->charinfo[c].spacing;
       }
     }

     if (xsize>maxwidth) maxwidth=xsize;
     if (maxwidth>maxwidth2) maxwidth2=maxwidth;
   }
   
   o = clone(image_program,0);
   img = ((struct image*)o->storage);
   img->xsize = maxwidth2;
   img->ysize = THIS->height * args;
   img->rgb.r=img->rgb.g=img->rgb.b=255;
   img->img=malloc(img->xsize*img->ysize*sizeof(rgb_group));

   if (!img) { free_object(o); error("Out of memory\n"); }

   MEMSET(img->img,0,img->xsize*img->ysize*sizeof(rgb_group));

   for (j=0; j<args; j++)
   {
     xsize = 0;
     for (i = 0; i < (int)sp[j-args].u.string->len; i++)
     {
       c=EXTRACT_UCHAR(sp[j-args].u.string->str+i);
       if ( c < (INT32)THIS->chars)
       {
	 write_char(THIS->charinfo+c,
		    img->img+xsize+img->xsize*j*THIS->height,
		    img->xsize,
		    THIS->height);
	 xsize += THIS->charinfo[c].spacing;
       }
     }
   }
   
   pop_n_elems(args);
   push_object(o);
}

void font_height(INT32 args)
{
   pop_n_elems(args);
   if (THIS)
      push_int(THIS->height);
   else
      push_int(0);
}

void font_baseline(INT32 args)
{
   pop_n_elems(args);
   if (THIS)
      push_int(THIS->baseline);
   else
      push_int(0);
}

/***************** global init etc *****************************/

/*

int load(string filename);  // load font file, true is success
object write(string text);  // new image object
int height();               // font heigth
int baseline();             // font baseline

*/

void init_font_programs(void)
{
   start_new_program();
   add_storage(sizeof(struct font*));

   add_function("load",font_load,
                "function(string:int)",0);

   add_function("write",font_write,
                "function(string:object)",0);

   add_function("height",font_height,
                "function(:int)",0);

   add_function("baseline",font_baseline,
                "function(:int)",0);
		
   set_init_callback(init_font_struct);
   set_exit_callback(exit_font_struct);
  
   font_program=end_c_program("/precompiled/font");
   
   font_program->refs++;
}

void exit_font(void) 
{
  if(font_program)
  {
    free_program(font_program);
    font_program=0;
  }
}


