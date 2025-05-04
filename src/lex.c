/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "las.h"
#include "lex.h"
#include "bignum.h"
#include "pike_compiler.h"
#include "interpret.h"
#include "pike_embed.h"

#include <ctype.h>


static const char *control_codes[64] = {
  /* 7-bit control codes.
   * Names taken from the Unicode standard.
   */
  "NUL", "STX", "SOT", "ETX", "EOT", "ENQ", "ACK", "BEL",
  "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
  "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",

  /* 8-bit control codes.
   * Names taken from https://vt100.net/docs/vt510-rm/chapter4.html
   * table 4-3 (preferred) and figure 4-2 (others).
   * Equivalent to <ESC> + character.
   */
  /* @   A      B        C      D      E      F      G */
  "80",  "81",  "82",    "83",  "IND", "NEL", "SSA", "ESA",
  /* H   I      J        K      L      M      N      O */
  "HTS", "HTJ", "VTS",   "PLD", "PLU", "RI",  "SS2", "SS3",
  /* P   Q      R        S      T      U      V      W */
  "DCS", "PU1", "PU2",   "STS", "CRH", "MW",  "SPA", "EPA",
  /* X   Y      Z        [      \      ]      ^      _ */
  "SOS", "99",  "DECID", "CSI", "ST",  "OSC", "PM",  "APC",
};

#define LEXDEBUG 0

/* Make lexers for shifts 0, 1 and 2. */

#define SHIFT	0
#include "lexer.h"
#undef SHIFT
#define SHIFT	1
#include "lexer.h"
#undef SHIFT
#define SHIFT	2
#include "lexer.h"
#undef SHIFT

int parse_esc_seq_pcharp (PCHARP buf, p_wchar2 *chr, ptrdiff_t *len)
{
  if(LIKELY(buf.shift == 0))
    return parse_esc_seq0((void*)buf.ptr,chr,len);
  if( buf.shift == 1 )
    return parse_esc_seq1((void*)buf.ptr,chr,len);
  return parse_esc_seq2((void*)buf.ptr,chr,len);
  /* UNREACHABLE */
}

int yylex(YYSTYPE *yylval)
{
  struct lex *lex;
  CHECK_COMPILER();
  lex = &THIS_COMPILATION->lex;
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX: Calling lexer at 0x%08lx\n",
	  (long)lex->current_lexer);
#endif /* LEXDEBUG > 8 */
  return(lex->current_lexer(lex, yylval));
}
