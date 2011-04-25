/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#ifndef PDF_INITER
#define PDF_CLASS(a,b,c,what) extern struct program *what;
#define PDF_SUBMODULE(a,b,c) 
#define PDF_SUBMODMAG(a,b,c) 
#define PDF_FUNCTION(a,name,c,d) void name(INT32 args);
#endif

PDF_CLASS("PDFlib",init_pdf_pdflib,exit_pdf_pdflib,pdflib_program )

/*

PDF_SUBMODULE("WBF",   init_pdf_wbf,  exit_pdf_wbf ) 

PDF_SUBMODMAG("PNG",   init_pdf_png,  exit_pdf_png  )

PDF_FUNCTION("lay",pdf_lay,
	       tOr(tFunc(tArr(tOr(tObj,tLayerMap)),tObj),
		   tFunc(tArr(tOr(tObj,tLayerMap))
			 tInt tInt tInt tInt,tObj)),0)

*/
