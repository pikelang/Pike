/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: parser.h,v 1.5 2004/10/05 21:17:15 nilsson Exp $
*/

void init_parser_html(void);
void exit_parser_html(void);
void init_parser_rcs(void);
void exit_parser_rcs(void);
void init_parser_c(void);
void exit_parser_c(void);
void init_parser_pike(void);
void exit_parser_pike(void);

/* c.c */

void push_token0( struct array **a, p_wchar0 *x, int l);
void push_token1( struct array **a, p_wchar1 *x, int l);
void push_token2( struct array **a, p_wchar2 *x, int l);
