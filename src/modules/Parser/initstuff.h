/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: initstuff.h,v 1.5 2004/02/23 17:40:40 per Exp $
*/

#ifndef PARSER_INITER
#define PARSER_CLASS(a,b,c,what,id) extern struct program *what;
#define PARSER_SUBMODULE(a,b,c) 
#define PARSER_SUBMODMAG(a,b,c) 
#define PARSER_FUNCTION(a,name,c,d) void name(INT32 args);
#endif

PARSER_CLASS("HTML", init_parser_html, exit_parser_html, 
	    parser_html_program, PROG_PARSER_HTML_ID )

PARSER_SUBMODULE("_RCS", init_parser_rcs, exit_parser_rcs )

   /*
for documentation purpose:

PARSER_SUBMODULE("ANY",   init_parser_any,  exit_parser_any  ) 
PARSER_SUBMODMAG("PNG",   init_parser_png,  exit_parser_png  )

PARSER_FUNCTION("lay",parser_lay,
	       tOr(tFunc(tArr(tOr(tObj,tLayerMap)),tObj),
		   tFunc(tArr(tOr(tObj,tLayerMap))
			 tInt tInt tInt tInt,tObj)),0)
   */
