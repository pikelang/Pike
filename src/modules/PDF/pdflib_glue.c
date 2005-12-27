/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: pdflib_glue.c,v 1.14 2005/12/27 21:42:17 nilsson Exp $
*/

#include "global.h"
#include "pdf_machine.h"

#if !defined(HAVE_LIBPDF)
#ifdef HAVE_PDFLIB_H
#undef HAVE_PDFLIB_H
#endif
#endif

#ifdef HAVE_PDFLIB_H

#include <pdflib.h>

#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "threads.h"
#include "array.h"
#include "mapping.h"
#include "pike_error.h"
#include "stralloc.h"
#include "threads.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "program.h"
#include "operators.h"


#define sp Pike_sp

/*! @module PDF
 */

/*! @class PDFgen
 */

/*** PDF callabcks ******************************************************/

static void pdf_error_handler(PDF *p, int type, const char* shortmsg)
{   
   Pike_error("PDF.PDFgen: %s\n",shortmsg);   
}

/*** PDF object *********************************************************/

#define THISOBJ (Pike_fp->current_object)
#define THIS ((struct pdf_storage *)(Pike_fp->current_storage))

struct pdf_storage
{
   PDF *pdf;
};

static void init_pdf(struct object *o)
{
   THIS->pdf=NULL;
}

static void exit_pdf(struct object *o)
{
   if (THIS->pdf) 
   {
      PDF *pdf=THIS->pdf;
      THIS->pdf=NULL;
      THREADS_ALLOW();
      PDF_delete(pdf);
      THREADS_DISALLOW();
   }
}

static void pdf_create(INT32 args)
{
   PDF *pdf;

   pop_n_elems(args);

   THREADS_ALLOW();

   pdf=PDF_new2(
      pdf_error_handler,
      NULL,   /* malloc */
      NULL,   /* realloc */
      NULL,   /* free */
      NULL);  /* opaque */

   THREADS_DISALLOW();

   if (THIS->pdf) PDF_delete(THIS->pdf);
   THIS->pdf=pdf;

   push_int(0);
}

/*! @decl int open_file(string filename);
 */

static void pdf_open_file(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *s;
   int n;
   if (args<1) SIMPLE_TOO_FEW_ARGS_ERROR("open_file",1);
   if (sp[-args].type!=T_STRING || sp[-args].u.string->size_shift)
      SIMPLE_BAD_ARG_ERROR("open_file",1,"8 bit string");
   s=sp[-args].u.string->str;
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   n=PDF_open_file(this->pdf,s);
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(n!=-1);
}

/*! @decl PDF close();
 */

static void pdf_close(INT32 args)
{
   struct pdf_storage *this=THIS;
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_close(this->pdf);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl PDF begin_page()
 *! @decl PDF begin_page(float width,float height)
 *! note:
 *!	Defaults to a4, portrait
 */

static void pdf_begin_page(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE w=a4_width,h=a4_height;

   get_all_args("begin_page",args,".%f%f",&w,&h);

   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_begin_page(this->pdf,(float)w,(float)h);
   THREADS_DISALLOW();

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl PDF end_page();
 */

static void pdf_end_page(INT32 args)
{
   struct pdf_storage *this=THIS;
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_end_page(this->pdf);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl float get_value(string key)
 *! @decl float get_value(string key,float modifier)
 */

static void pdf_get_value(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m=0.0;
   char *s;

   get_all_args("get_value",args,"%s.%F",&s,&m);

   if (!this->pdf) Pike_error("PDF not initiated\n");

   push_float(PDF_get_value(this->pdf,s,(float)m));
   stack_pop_n_elems_keep_top(args);
}

/*! @decl float set_value(string key,float value)
 */

static void pdf_set_value(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m;
   char *s;

   get_all_args("set_value",args,"%s%F",&s,&m);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_set_value(this->pdf,s,m);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl string get_parameter(string key)
 *! @decl string get_parameter(string key,float modifier)
 */

static void pdf_get_parameter(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m=0.0;
   char *s;

   get_all_args("get_parameter",args,"%s.%F",&s,&m);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   push_text(PDF_get_parameter(this->pdf,s,(float)m));
   stack_pop_n_elems_keep_top(args);
}

/*! @decl float set_parameter(string key,string parameter)
 */

static void pdf_set_parameter(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *s,*v;

   get_all_args("set_parameter",args,"%s%s",&s,&v);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_set_parameter(this->pdf,s,v);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl float set_info(string key,string info)
 */

static void pdf_set_info(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *s,*v;

   get_all_args("set_info",args,"%s%s",&s,&v);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_set_info(this->pdf,s,v);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl int findfont(string fontname);
 *! @decl int findfont(string fontname,void|string encoding,void|int embed);
 */

static void pdf_findfont(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *encoding=NULL;
   int embed=0;
   int n;
   char *fontname;

   get_all_args("findfont",args,"%s.%s%d",&fontname,&encoding,&embed);
   if(!encoding)
     encoding="host";
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   n=PDF_findfont(this->pdf,fontname,encoding,embed);
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(n);
}

/*! @decl PDF setfont(int n,float size)
 */

static void pdf_setfont(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE n;
   FLOAT_TYPE size;

   get_all_args("setfont",args,"%i%F",&n,&size);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   PDF_setfont(this->pdf,(int)n,(float)size);
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl PDF show(string s)
 */

static void pdf_show(INT32 args)
{
   struct pdf_storage *this=THIS;
   struct pike_string *ps;
   
   get_all_args("show",args,"%W",&ps);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   if (ps->size_shift)
      Pike_error("wide strings not supported yet\n");

   THREADS_ALLOW();
   PDF_show2(this->pdf,(char*)ps->str,(int)ps->len);
   THREADS_DISALLOW();
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl PDF showxy(string s,float x,float y)
 */

static void pdf_showxy(INT32 args)
{
   struct pdf_storage *this=THIS;
   struct pike_string *ps;
   FLOAT_TYPE x,y;
   
   get_all_args("showxy",args,"%W%F%F",&ps,&x,&y);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   if (ps->size_shift)
      Pike_error("wide strings not supported yet\n");

   THREADS_ALLOW();
   PDF_show_xy2(this->pdf,(char*)ps->str,(int)ps->len,(float)x,(float)y);
   THREADS_DISALLOW();
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl PDF continue_text(string s)
 */

static void pdf_continue_text(INT32 args)
{
   struct pdf_storage *this=THIS;
   struct pike_string *ps;
   
   get_all_args("continue_text",args,"%W",&ps);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   if (ps->size_shift)
      Pike_error("wide strings not supported yet\n");

   THREADS_ALLOW();
   PDF_continue_text2(this->pdf,(char*)ps->str,(int)ps->len);
   THREADS_DISALLOW();
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl int show_boxed(string text,float x,float y,float width,float height,string mode)
 *! @decl int show_boxed(string text,float x,float y,float width,float height,string mode,string feature)
 */

static void pdf_show_boxed(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *text=NULL,*mode=NULL,*feature="";
   FLOAT_TYPE x=0.0,y=0.0,width=0.0,height=0.0;
   INT_TYPE res=0;

   get_all_args("show_boxed",args,"%s%F%F%F%F%s.%s",
                &text,&x,&y,&width,&height,&mode,&feature);

   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_show_boxed(this->pdf,text,(float)x,(float)y,(float)width,(float)height,mode,feature);
   THREADS_DISALLOW();
   push_int((INT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl float stringwidth(string text,int font,float size)
 */

static void pdf_stringwidth(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE res=0.0,size=0.0;
   INT_TYPE font=0;
   struct pike_string *ps=NULL;
   get_all_args("stringwidth2",args,"%W%i%F",&ps,&font,&size);
   if (ps->size_shift)
      Pike_error("wide strings not supported yet\n");
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_stringwidth2(this->pdf,(char*)ps->str,(int)ps->len,
			(int)font,(float)size);
   THREADS_DISALLOW();
   push_float((FLOAT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl object set_text_pos(float x,float y)
 */

static void pdf_set_text_pos(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0;
   get_all_args("set_text_pos",args,"%F%F",&x,&y);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_set_text_pos(this->pdf,(float)x,(float)y);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setdash(float b,float w)
 */

static void pdf_setdash(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE b=0.0,w=0.0;
   get_all_args("setdash",args,"%F%F",&b,&w);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setdash(this->pdf,(float)b,(float)w);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setflat(float flatness)
 */

static void pdf_setflat(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE flatness=0.0;
   get_all_args("setflat",args,"%F",&flatness);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setflat(this->pdf,(float)flatness);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setlinejoin(int linejoin)
 */

static void pdf_setlinejoin(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE linejoin=0;
   get_all_args("setlinejoin",args,"%i",&linejoin);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setlinejoin(this->pdf,(int)linejoin);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setlinecap(int linecap)
 */

static void pdf_setlinecap(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE linecap=0;
   get_all_args("setlinecap",args,"%i",&linecap);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setlinecap(this->pdf,(int)linecap);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setmiterlimit(float miter)
 */

static void pdf_setmiterlimit(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE miter=0.0;
   get_all_args("setmiterlimit",args,"%F",&miter);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setmiterlimit(this->pdf,(float)miter);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setlinewidth(float width)
 */

static void pdf_setlinewidth(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE width=0.0;
   get_all_args("setlinewidth",args,"%F",&width);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setlinewidth(this->pdf,(float)width);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object translate(float tx,float ty)
 */

static void pdf_translate(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE tx=0.0,ty=0.0;
   get_all_args("translate",args,"%F%F",&tx,&ty);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_translate(this->pdf,(float)tx,(float)ty);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object scale(float sx,float sy)
 */

static void pdf_scale(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE sx=0.0,sy=0.0;
   get_all_args("scale",args,"%F%F",&sx,&sy);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_scale(this->pdf,(float)sx,(float)sy);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object rotate(float phi)
 */

static void pdf_rotate(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE phi=0.0;
   get_all_args("rotate",args,"%F",&phi);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_rotate(this->pdf,(float)phi);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object skew(float alpha,float beta)
 */

static void pdf_skew(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE alpha=0.0,beta=0.0;
   get_all_args("skew",args,"%F%F",&alpha,&beta);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_skew(this->pdf,(float)alpha,(float)beta);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object concat(float a,float b,float c,float d,float e,float f)
 */

static void pdf_concat(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE a=0.0,b=0.0,c=0.0,d=0.0,e=0.0,f=0.0;
   get_all_args("concat",args,"%F%F%F%F%F%F",&a,&b,&c,&d,&e,&f);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_concat(this->pdf,(float)a,(float)b,(float)c,(float)d,(float)e,(float)f);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object moveto(float x,float y)
 */

static void pdf_moveto(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0;
   get_all_args("moveto",args,"%F%F",&x,&y);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_moveto(this->pdf,(float)x,(float)y);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object lineto(float x,float y)
 */

static void pdf_lineto(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0;
   get_all_args("lineto",args,"%F%F",&x,&y);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_lineto(this->pdf,(float)x,(float)y);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object curveto(float x1,float y1,float x2,float y2,float x3,float y3)
 */

static void pdf_curveto(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x1=0.0,y1=0.0,x2=0.0,y2=0.0,x3=0.0,y3=0.0;
   get_all_args("curveto",args,"%F%F%F%F%F%F",&x1,&y1,&x2,&y2,&x3,&y3);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_curveto(this->pdf,(float)x1,(float)y1,(float)x2,(float)y2,(float)x3,(float)y3);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object circle(float x,float y,float r)
 */

static void pdf_circle(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0,r=0.0;
   get_all_args("circle",args,"%F%F%F",&x,&y,&r);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_circle(this->pdf,(float)x,(float)y,(float)r);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object arc(float x,float y,float r,float start,float end)
 */

static void pdf_arc(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0,r=0.0,start=0.0,end=0.0;
   get_all_args("arc",args,"%F%F%F%F%F",&x,&y,&r,&start,&end);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_arc(this->pdf,(float)x,(float)y,(float)r,(float)start,(float)end);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object rect(float x,float y,float width,float height)
 */

static void pdf_rect(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0,width=0.0,height=0.0;
   get_all_args("rect",args,"%F%F%F%F",&x,&y,&width,&height);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_rect(this->pdf,(float)x,(float)y,(float)width,(float)height);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setgray_fill(float gray)
 */

static void pdf_setgray_fill(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE gray=0.0;
   get_all_args("setgray_fill",args,"%F",&gray);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setgray_fill(this->pdf,(float)gray);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setgray_stroke(float gray)
 */

static void pdf_setgray_stroke(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE gray=0.0;
   get_all_args("setgray_stroke",args,"%F",&gray);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setgray_stroke(this->pdf,(float)gray);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setgray(float gray)
 */

static void pdf_setgray(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE gray=0.0;
   get_all_args("setgray",args,"%F",&gray);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setgray(this->pdf,(float)gray);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setrgbcolor_fill(float red,float green,float blue)
 */

static void pdf_setrgbcolor_fill(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE red=0.0,green=0.0,blue=0.0;
   get_all_args("setrgbcolor_fill",args,"%F%F%F",&red,&green,&blue);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setrgbcolor_fill(this->pdf,(float)red,(float)green,(float)blue);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setrgbcolor_stroke(float red,float green,float blue)
 */

static void pdf_setrgbcolor_stroke(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE red=0.0,green=0.0,blue=0.0;
   get_all_args("setrgbcolor_stroke",args,"%F%F%F",&red,&green,&blue);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setrgbcolor_stroke(this->pdf,(float)red,(float)green,(float)blue);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object setrgbcolor(float red,float green,float blue)
 */

static void pdf_setrgbcolor(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE red=0.0,green=0.0,blue=0.0;
   get_all_args("setrgbcolor",args,"%F%F%F",&red,&green,&blue);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_setrgbcolor(this->pdf,(float)red,(float)green,(float)blue);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl int open_image_file(string type,string filename)
 *! @decl int open_image_file(string type,string filename,void|string stringparam,void|int intparam)
 */

static void pdf_open_image_file(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE res=0,intparam=0;
   char *type=NULL,*filename=NULL,*stringparam=NULL;
   get_all_args("open_image_file",args,"%s%s.%s%d",&type,
                &filename,&stringparam,&intoaram);
   if (!stringparam)
     stringparam="";
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_open_image_file(this->pdf,type,filename,stringparam,(int)intparam);
   THREADS_DISALLOW();
   push_int((INT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl int open_CCITT(string filename,int width,int height,int BitReverse,int K,int BlackIs1)
 */

static void pdf_open_CCITT(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE res=0,width=0,height=0,BitReverse=0,K=0,BlackIs1=0;
   char *filename=NULL;
   get_all_args("open_CCITT",args,"%s%i%i%i%i%i",
		&filename,&width,&height,&BitReverse,&K,&BlackIs1);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_open_CCITT(this->pdf,filename,(int)width,(int)height,
		      (int)BitReverse,(int)K,(int)BlackIs1);
   THREADS_DISALLOW();
   push_int((INT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl int open_image(string type,string source,string data,int width,int height,int components,int bpc,string params)
 */

static void pdf_open_image(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *type=NULL,*source=NULL,*params=NULL;
   struct pike_string *ps;
   INT_TYPE res=0,length=0,width=0,height=0,components=0,bpc=0;
   get_all_args("open_image",args,"%s%s%W%i%i%i%i%s",
		&type,&source,&ps,&width,&height,
		&components,&bpc,&params);
   if (ps->size_shift)
      Pike_error("wide string image data\n");
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_open_image(this->pdf,type,source,
		      (char*)ps->str,(long)ps->len,
		      (int)width,(int)height,(int)components,(int)bpc,params);
   THREADS_DISALLOW();
   push_int((INT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl object close_image(int image)
 */

static void pdf_close_image(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE image=0;
   get_all_args("close_image",args,"%i",&image);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_close_image(this->pdf,(int)image);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object place_image(int image,float x,float y,float scale)
 */

static void pdf_place_image(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x=0.0,y=0.0,scale=0.0;
   INT_TYPE image=0;
   get_all_args("place_image",args,"%i%F%F%F",&image,&x,&y,&scale);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_place_image(this->pdf,(int)image,(float)x,(float)y,(float)scale);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl int add_bookmark(string text,int parent,int open)
 */

static void pdf_add_bookmark(INT32 args)
{
   struct pdf_storage *this=THIS;
   INT_TYPE res=0,parent=0,open=0;
   char *text=NULL;
   get_all_args("add_bookmark",args,"%s%i%i",&text,&parent,&open);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   res=PDF_add_bookmark(this->pdf,text,(int)parent,(int)open);
   THREADS_DISALLOW();
   push_int((INT_TYPE)res);
   stack_pop_n_elems_keep_top(args);
}

/*! @decl object attach_file(float llx,float lly,float urx,float ury,string filename,string description,string author,string mimetype,string icon)
 */

static void pdf_attach_file(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *filename=NULL,*description=NULL,*author=NULL,*mimetype=NULL,*icon=NULL;
   FLOAT_TYPE llx=0.0,lly=0.0,urx=0.0,ury=0.0;
   get_all_args("attach_file",args,"%F%F%F%F%s%s%s%s%s",&llx,&lly,&urx,&ury,&filename,&description,&author,&mimetype,&icon);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_attach_file(this->pdf,(float)llx,(float)lly,(float)urx,(float)ury,filename,description,author,mimetype,icon);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object add_pdflink(float llx,float lly,float urx,float ury,string filename,int page,string dest)
 */

static void pdf_add_pdflink(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *filename=NULL,*dest=NULL;
   INT_TYPE page=0;
   FLOAT_TYPE llx=0.0,lly=0.0,urx=0.0,ury=0.0;
   get_all_args("add_pdflink",args,"%F%F%F%F%s%i%s",&llx,&lly,&urx,&ury,&filename,&page,&dest);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_add_pdflink(this->pdf,(float)llx,(float)lly,(float)urx,(float)ury,filename,(int)page,dest);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object add_locallink(float llx,float lly,float urx,float ury,int page,string dest)
 */

static void pdf_add_locallink(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *dest=NULL;
   INT_TYPE page=0;
   FLOAT_TYPE llx=0.0,lly=0.0,urx=0.0,ury=0.0;
   get_all_args("add_locallink",args,"%F%F%F%F%i%s",&llx,&lly,&urx,&ury,&page,&dest);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_add_locallink(this->pdf,(float)llx,(float)lly,(float)urx,(float)ury,(int)page,dest);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object add_launchlink(float llx,float lly,float urx,float ury,string filename)
 */

static void pdf_add_launchlink(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *filename=NULL;
   FLOAT_TYPE llx=0.0,lly=0.0,urx=0.0,ury=0.0;
   get_all_args("add_launchlink",args,"%F%F%F%F%s",&llx,&lly,&urx,&ury,&filename);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_add_launchlink(this->pdf,(float)llx,(float)lly,(float)urx,(float)ury,filename);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object add_weblink(float llx,float lly,float urx,float ury,string url)
 */

static void pdf_add_weblink(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *url=NULL;
   FLOAT_TYPE llx=0.0,lly=0.0,urx=0.0,ury=0.0;
   get_all_args("add_weblink",args,"%F%F%F%F%s",&llx,&lly,&urx,&ury,&url);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_add_weblink(this->pdf,(float)llx,(float)lly,(float)urx,(float)ury,url);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object set_border_style(string style,float width)
 */

static void pdf_set_border_style(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE width=0.0;
   char *style=NULL;
   get_all_args("set_border_style",args,"%s%F",&style,&width);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_set_border_style(this->pdf,style,(float)width);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object set_border_color(float red,float green,float blue)
 */

static void pdf_set_border_color(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE red=0.0,green=0.0,blue=0.0;
   get_all_args("set_border_color",args,"%F%F%F",&red,&green,&blue);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_set_border_color(this->pdf,(float)red,(float)green,(float)blue);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @decl object set_border_dash(float b,float w)
 */

static void pdf_set_border_dash(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE b=0.0,w=0.0;
   get_all_args("set_border_dash",args,"%F%F",&b,&w);
   if (!this->pdf) Pike_error("PDF not initiated\n");
   THREADS_ALLOW();
   PDF_set_border_dash(this->pdf,(float)b,(float)w);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

/*! @endclass
 */

/*! @decl constant a0_width
 *! @decl constant a0_height
 *! @decl constant a1_width
 *! @decl constant a1_height
 *! @decl constant a2_width
 *! @decl constant a2_height
 *! @decl constant a3_width
 *! @decl constant a3_height
 *! @decl constant a4_width
 *! @decl constant a4_height
 *! @decl constant a5_width
 *! @decl constant a5_height
 *! @decl constant a6_width
 *! @decl constant a6_height
 *! @decl constant b5_width
 *! @decl constant b5_height
 *! @decl constant letter_width
 *! @decl constant letter_height
 *! @decl constant legal_width
 *! @decl constant legal_height
 *! @decl constant ledger_width
 *! @decl constant ledger_height
 *! @decl constant p11x17_width
 *! @decl constant p11x17_height
 */

#endif /* HAVE_PDFLIB_H */
/*** module init & exit & stuff *****************************************/

void exit_pdf_pdflib(void)
{
#if 0
/* this is documented, but not in library */
   PDF_shutdown(); /* stop pdflib */
#endif
}

void init_pdf_pdflib(void)
{
#ifdef HAVE_PDFLIB_H
   PDF_boot(); /* start pdflib */

/* PDF object */

   start_new_program();
   ADD_STORAGE(struct pdf_storage);

   ADD_FUNCTION("create"   , pdf_create,    tFunc(tNone,tVoid),0);
   ADD_FUNCTION("open_file", pdf_open_file, tFunc(tStr,tInt),0);
   ADD_FUNCTION("close",     pdf_close,     tFunc(tNone,tObj),0);
   ADD_FUNCTION("begin_page",pdf_begin_page,tFunc(tFloat tFloat,tObj),0);
   ADD_FUNCTION("end_page",  pdf_end_page,  tFunc(tNone,tObj),0);

#define tIoF tOr(tInt,tFloat)

   ADD_FUNCTION("get_value",pdf_get_value,
		tFunc(tStr tOr(tIoF,tVoid),tFloat),0);
   ADD_FUNCTION("set_value",pdf_set_value,tFunc(tStr tIoF,tObj),0);

   ADD_FUNCTION("get_parameter",pdf_get_parameter,
		tFunc(tStr tOr(tIoF,tVoid),tStr),0);
   ADD_FUNCTION("set_parameter",pdf_set_parameter,tFunc(tStr tStr,tObj),0);

   ADD_FUNCTION("set_info",pdf_set_info,tFunc(tStr tStr,tObj),0);

   ADD_FUNCTION("findfont",pdf_findfont,
		tFunc(tStr tOr(tVoid,tStr) tOr(tVoid,tInt),tInt),0);
   ADD_FUNCTION("setfont",pdf_setfont, tFunc(tInt tIoF,tObj),0);

   ADD_FUNCTION("show",pdf_show,tFunc(tStr,tObj),0);
   ADD_FUNCTION("showxy",pdf_showxy,
		tFunc(tStr tIoF tIoF,tObj),0);
   ADD_FUNCTION("continue_text",pdf_continue_text,tFunc(tStr,tObj),0);

   ADD_FUNCTION("show_boxed",pdf_show_boxed,
                tFunc(tStr tIoF tIoF tIoF tIoF tStr tStr,tInt),0);
   ADD_FUNCTION("stringwidth",pdf_stringwidth,
                tFunc(tStr tInt tIoF,tIoF),0);
   ADD_FUNCTION("set_text_pos",pdf_set_text_pos,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("setdash",pdf_setdash,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("setflat",pdf_setflat,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("setlinejoin",pdf_setlinejoin,tFunc(tInt,tObj),0);
   ADD_FUNCTION("setlinecap",pdf_setlinecap,tFunc(tInt,tObj),0);
   ADD_FUNCTION("setmiterlimit",pdf_setmiterlimit,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("setlinewidth",pdf_setlinewidth,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("translate",pdf_translate,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("scale",pdf_scale,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("rotate",pdf_rotate,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("skew",pdf_skew,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("concat",pdf_concat,
                tFunc(tIoF tIoF tIoF tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("moveto",pdf_moveto,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("lineto",pdf_lineto,tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("curveto",pdf_curveto,
                tFunc(tIoF tIoF tIoF tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("circle",pdf_circle,tFunc(tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("arc",pdf_arc,tFunc(tIoF tIoF tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("rect",pdf_rect,tFunc(tIoF tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("setgray_fill",pdf_setgray_fill,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("setgray_stroke",pdf_setgray_stroke,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("setgray",pdf_setgray,tFunc(tIoF,tObj),0);
   ADD_FUNCTION("setrgbcolor_fill",pdf_setrgbcolor_fill,
                tFunc(tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("setrgbcolor_stroke",pdf_setrgbcolor_stroke,
                tFunc(tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("setrgbcolor",pdf_setrgbcolor,
                tFunc(tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("open_image_file",pdf_open_image_file,
                tFunc(tStr tStr tOr(tStr,tVoid) tOr(tInt,tVoid),tInt),0);
   ADD_FUNCTION("open_CCITT",pdf_open_CCITT,
                tFunc(tStr tInt tInt tInt tInt tInt,tInt),0);
   ADD_FUNCTION("open_image",pdf_open_image,
                tFunc(tStr tStr tStr tInt tInt tInt tInt tInt tStr,tInt),0);
   ADD_FUNCTION("close_image",pdf_close_image,tFunc(tInt,tObj),0);
   ADD_FUNCTION("place_image",pdf_place_image,
                tFunc(tInt tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("add_bookmark",pdf_add_bookmark,
                tFunc(tStr tInt tInt,tInt),0);
   ADD_FUNCTION("attach_file",pdf_attach_file,
                tFunc(tIoF tIoF tIoF tIoF tStr tStr tStr tStr tStr,tObj),0);
   ADD_FUNCTION("add_pdflink",pdf_add_pdflink,
                tFunc(tIoF tIoF tIoF tIoF tStr tInt tStr,tObj),0);
   ADD_FUNCTION("add_locallink",pdf_add_locallink,
                tFunc(tIoF tIoF tIoF tIoF tInt tStr,tObj),0);
   ADD_FUNCTION("add_launchlink",pdf_add_launchlink,
                tFunc(tIoF tIoF tIoF tIoF tStr,tObj),0);
   ADD_FUNCTION("add_weblink",pdf_add_weblink,
                tFunc(tIoF tIoF tIoF tIoF tStr,tObj),0);
   ADD_FUNCTION("set_border_style",pdf_set_border_style,
                tFunc(tStr tIoF,tObj),0);
   ADD_FUNCTION("set_border_color",pdf_set_border_color,
                tFunc(tIoF tIoF tIoF,tObj),0);
   ADD_FUNCTION("set_border_dash",pdf_set_border_dash,
                tFunc(tIoF tIoF,tObj),0);

   set_init_callback(init_pdf);
   set_exit_callback(exit_pdf);
   end_class("PDFgen",0);

   add_float_constant("a0_width",a0_width, 0);
   add_float_constant("a0_height",a0_height, 0);
   add_float_constant("a1_width",a1_width, 0);
   add_float_constant("a1_height",a1_height, 0);
   add_float_constant("a2_width",a2_width, 0);
   add_float_constant("a2_height",a2_height, 0);
   add_float_constant("a3_width",a3_width, 0);
   add_float_constant("a3_height",a3_height, 0);
   add_float_constant("a4_width",a4_width, 0);
   add_float_constant("a4_height",a4_height, 0);
   add_float_constant("a5_width",a5_width, 0);
   add_float_constant("a5_height",a5_height, 0);
   add_float_constant("a6_width",a6_width, 0);
   add_float_constant("a6_height",a6_height, 0);
   add_float_constant("b5_width",b5_width, 0);
   add_float_constant("b5_height",b5_height, 0);
   add_float_constant("letter_width",letter_width, 0);
   add_float_constant("letter_height",letter_height, 0);
   add_float_constant("legal_width",legal_width, 0);
   add_float_constant("legal_height",legal_height, 0);
   add_float_constant("ledger_width",ledger_width, 0);
   add_float_constant("ledger_height",ledger_height, 0);
   add_float_constant("p11x17_width",p11x17_width, 0);
   add_float_constant("p11x17_height",p11x17_height, 0);

/*     ADD_FUNCTION("decode",image_pdf_decode,tFunc(tStr tOr(tVoid,tOptions),tObj),0); */
/*     add_integer_constant("IFAST", JDCT_IFAST, 0); */

#endif /* HAVE_PDFLIB_H */
}

/*! @endmodule
 */
