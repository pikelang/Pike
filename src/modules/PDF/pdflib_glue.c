/*
 * $Id: pdflib_glue.c,v 1.1 2001/01/13 18:35:19 mirar Exp $
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

#include "module_magic.h"

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

//! method int open_file(string filename);

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

//! method PDF close();

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

//! method PDF begin_page()
//! method PDF begin_page(float width,float height)
//! note:
//!	Defaults to a4, portrait

static void pdf_begin_page(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE w=a4_width,h=a4_height;

   if (args)
      get_all_args("begin_page",args,"%f%f",&w,&h);

   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_begin_page(this->pdf,(float)w,(float)h);
   THREADS_DISALLOW();

   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

//! method PDF end_page();

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

//! method float get_value(string key)
//! method float get_value(string key,float modifier)

static void pdf_get_value(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m=0.0;
   char *s;

   if (args==1) get_all_args("get_value",args,"%s",&s);
   else get_all_args("get_value",args,"%s%F",&s,&m);

   if (!this->pdf) Pike_error("PDF not initiated\n");

   push_float(PDF_get_value(this->pdf,s,(float)m));
   stack_pop_n_elems_keep_top(args);
}

//! method float set_value(string key,float value)

static void pdf_set_value(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m=0.0;
   char *s;

   get_all_args("set_value",args,"%s%F",&s);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   PDF_set_value(this->pdf,s,m);
   THREADS_DISALLOW();
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

//! method string get_parameter(string key)
//! method string get_parameter(string key,float modifier)

static void pdf_get_parameter(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE m=0.0;
   char *s;

   if (args==1) get_all_args("get_parameter",args,"%s",&s);
   else get_all_args("get_parameter",args,"%s%F",&s,&m);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   push_text(PDF_get_parameter(this->pdf,s,(float)m));
   stack_pop_n_elems_keep_top(args);
}

//! method float set_parameter(string key,string parameter)

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

//! method float set_info(string key,string info)

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

//! method int findfont(string fontname);
//! method int findfont(string fontname,void|string encoding,void|int embed);

static void pdf_findfont(INT32 args)
{
   struct pdf_storage *this=THIS;
   char *encoding="host";
   int embed=0;
   int n;
   char *fontname;

   get_all_args("findfont",args,"%s",&fontname);
   if (args>=2)
      if (sp[1-args].type==T_STRING && 
	  !sp[1-args].u.string->size_shift)
	 encoding=sp[1-args].u.string->str;
      else if (sp[1-args].type!=T_INT || sp[-args].u.integer)
	 SIMPLE_BAD_ARG_ERROR("findfont",2,"8 bit string or void");
   if (args>=3)
      if (sp[2-args].type==T_INT)
	 embed=(int)sp[2-args].u.integer;
      else
	 SIMPLE_BAD_ARG_ERROR("findfont",3,"int");
   if (!this->pdf) Pike_error("PDF not initiated\n");

   THREADS_ALLOW();
   n=PDF_findfont(this->pdf,fontname,encoding,embed);
   THREADS_DISALLOW();
   pop_n_elems(args);
   push_int(n);
}

//! method PDF setfont(int n,float size)

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

//! method PDF set_text_pos(float x,float y)

static void pdf_set_text_pos(INT32 args)
{
   struct pdf_storage *this=THIS;
   FLOAT_TYPE x,y;
   
   get_all_args("set_text_pos",args,"%F%F",&x,&y);
   if (!this->pdf) Pike_error("PDF not initiated\n");

   PDF_set_text_pos(this->pdf,(float)x,(float)y);
   
   pop_n_elems(args);
   ref_push_object(THISOBJ);
}

//! method PDF show(string s)

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

//! method PDF showxy(string s,float x,float y)

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

//! method PDF continue_text(string s)

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

#endif /* HAVE_PDFLIB_H */
/*** module init & exit & stuff *****************************************/

void pike_module_exit(void)
{
#if 0
   PDF_shutdown(); /* stop pdflib */
#endif
}

void pike_module_init(void)
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

   ADD_FUNCTION("set_text_pos",pdf_set_text_pos,
		tFunc(tIoF tIoF,tObj),0);
   ADD_FUNCTION("show",pdf_show,tFunc(tStr,tObj),0);
   ADD_FUNCTION("showxy",pdf_showxy,
		tFunc(tStr tIoF tIoF,tObj),0);
   ADD_FUNCTION("continue_text",pdf_continue_text,tFunc(tStr,tObj),0);

   set_init_callback(init_pdf);
   set_init_callback(exit_pdf);
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

