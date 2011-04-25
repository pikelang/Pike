/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* DiscDate.C .. converts boring normal dates to fun Discordian Date -><-
   written  the 65th day of The Aftermath in the Year of Our Lady of 
   Discord 3157 by Druel the Chaotic aka Jeremy Johnson aka
   mpython@gnu.ai.mit.edu  
      Worcester MA 01609

   and I'm not responsible if this program messes anything up (except your 
   mind, I'm responsible for that)
*/

#include "global.h"
#include "stralloc.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "array.h"
#include "pike_error.h"
#include "builtin_functions.h"
#include <time.h>
#include <string.h>
#include <stdio.h>

struct disc_time
{
  int season; /* 0-4 */
  int day; /* 0-72 */
  int yday; /* 0-365 */
  int year; /* 3066- */
};

static char *ending(int);
static void print(struct disc_time);
static struct disc_time convert(int,int);

/*! @module spider
 */

/*! @decl string discdate(int time)
 */
void f_discdate(INT32 args)
{
  time_t t;
  int bob,raw;
  struct disc_time hastur;
  if (args != 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("discdate",1);
  else {
    struct tm *eris;
    t=Pike_sp[-args].u.integer;
    eris=localtime(&t);
    if (!eris) Pike_error ("localtime() failed to convert %ld\n", (long) t);
    bob=eris->tm_yday;		/* days since Jan 1. */
    raw=eris->tm_year;		/* years since 1980 */
    hastur=convert(bob,raw);
  }
  pop_n_elems(args);
  print(hastur);
}

/*! @endmodule
 */

static char *ending(int num)
{  
  switch (num % 10)
  {
   case 1:
    return "st";
   case 2:
    return "nd";
   case 3:
    return "rd";
  }
  return "th";
}

static struct disc_time convert(int nday, int nyear)
{ 
  struct disc_time this;
   
  this.year = nyear+3066;
  this.day=nday;
  this.season=0;
  if (((this.year%4)==2) &&
      (((this.year % 100) != 66) || ((this.year % 400) >= 300)))
  {
    /* St. Tib's day adjustment. */
    if (this.day==59)
      this.day=-1;
    else if (this.day >59)
      this.day-=1;
  }
  this.yday=this.day;
  while (this.day>=73)
  { 
    this.season++;
    this.day-=73;
  }
  this.day++;
  return this;
}

static const char *days[5] =
{ 
  "Sweetmorn",
  "Boomtime",
  "Pungenday",
  "Prickle-Prickle",
  "Setting Orange"
};

static const char *seasons[5] =
{ 
  "Chaos",
  "Discord",
  "Confusion",
  "Bureaucracy",
  "The Aftermath"
};

static const char *holidays[5][2] =
{ 
  { "Mungday", "Chaoflux" },
  { "Mojoday", "Discoflux" },
  { "Syaday",  "Confuflux" },
  { "Zaraday", "Bureflux" } ,
  { "Maladay", "Afflux" }
};

static void print(struct disc_time tick)
{ 
  if (!tick.day) 
  {
    push_text("St. Tib's Day!");
  } else { 
    struct string_builder s;
    init_string_builder_alloc(&s, 30, 0);
    string_builder_sprintf(&s,
			   "%s, the %d%s day of %s",
			   days[tick.yday%5], tick.day,
			   ending(tick.day),seasons[tick.season]);
    push_string(finish_string_builder(&s));
  }
  push_int(tick.year);
  if ((tick.day==5)||(tick.day==50))
  { 
    if (tick.day==5)
      push_text(holidays[tick.season][0]);
    else
      push_text(holidays[tick.season][1]);
  } else {
    push_int(0);
  }
  f_aggregate(3);
}
