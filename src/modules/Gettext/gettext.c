/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: gettext.c,v 1.27 2008/04/01 08:01:28 mast Exp $
*/

#include "global.h"
#include "config.h"

#ifdef HAVE_GETTEXT

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "stralloc.h"
#include "pike_error.h"
#include "pike_macros.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "module_support.h"


#define sp Pike_sp

/*! @module Locale
 */

/*! @module Gettext
 *!
 *! This module enables access to localization functions from within Pike.
 *!
 *! @note
 *! The message conversion functions in this module do not handle
 *! Unicode strings. They only provide thin wrappers to
 *! @tt{gettext(3)@} etc, which means strings are assumed to be
 *! encoded according to the @tt{LC_*@} and @tt{LANG@} variables. See
 *! the docs for @tt{gettext(3)@} for details.
 */

/******************** PUBLIC FUNCTIONS BELOW THIS LINE */

/*! @decl string gettext(string msg)
 *! @decl string gettext(string msg, string domain)
 *! @decl string gettext(string msg, string domain, int category)
 *!
 *! @param msg
 *!   Message to be translated.
 *!
 *! @param domain
 *!   Domain from within the message should be translated.
 *!   Defaults to the current domain.
 *!
 *! @param category
 *!   Category from which the translation should be taken.
 *!   Defaults to @[Locale.Gettext.LC_MESSAGES].
 *!
 *! Return a translated version of @[msg] within the context
 *! of the specified @[domain] and current locale. If there is no
 *! translation available, @[msg] is returned.
 *!
 *! @note
 *!   Prior to Pike 7.3 this function only accepted one argument,
 *!   and the other functionality was provided by @[dgettext()]
 *!   and @[dcgettext()].
 *!
 *! @seealso
 *!   @[bindtextdomain], @[textdomain], @[setlocale], @[localeconv]
 */
void f_gettext(INT32 args)
{
  const char *domain, *msg;
  int cat;

  get_all_args("Locale.Gettext.gettext", args, "%c.%C%D", &msg, &domain, &cat);

  switch(args) {
#ifdef PIKE_DEBUG
  case 0:
    /* NOT_REACHED, but... */
    Pike_error("Too few arguments to Locale.Gettext.gettext().\n");
    break;
#endif
  case 1:
    push_text(gettext(msg));
    break;
  case 2:
    push_text(dgettext(domain, msg));
    break;
  default:
    push_text(dcgettext(domain, msg, cat));
    break;
  }

  stack_pop_n_elems_keep_top(args);
}

/*! @decl string dgettext(string domain, string msg)
 *!
 *! Return a translated version of @[msg] within the context
 *! of the specified @[domain] and current locale. If there is
 *! no translation available, @[msg] is returned.
 *!
 *! @note
 *!   Obsoleted by @[gettext()] in Pike 7.3.
 *!
 *! @seealso
 *!   @[bindtextdomain], @[textdomain], @[gettext], @[setlocale], @[localeconv]
*/
void f_dgettext(INT32 args)
{
  const char *domain, *msg;
  get_all_args("Locale.Gettext.dgettext", args, "%c%c", &domain, &msg);

  push_text(dgettext(domain, msg));

  stack_pop_n_elems_keep_top(args);
}

/*! @decl string dcgettext(string domain, string msg, int category)
 *!
 *! Return a translated version of @[msg] within the context of the
 *! specified @[domain] and current locale for the specified
 *! @[category]. Calling dcgettext with category @[Locale.Gettext.LC_MESSAGES]
 *! gives the same result as dgettext.
 *!
 *! If there is no translation available, @[msg] is returned.
 *!
 *! @note
 *!   Obsoleted by @[gettext()] in Pike 7.3.
 *!
 *! @seealso
 *!   @[bindtextdomain], @[textdomain], @[gettext], @[setlocale], @[localeconv]
 */
void f_dcgettext(INT32 args)
{
  const char *domain, *msg;
  int category;

  get_all_args("Locale.Gettext.dcgettext", args, "%c%c%d",
	       &domain, &msg, &category);

  push_text(dcgettext(domain, msg, category));

  stack_pop_n_elems_keep_top(args);
}

/*! @decl string textdomain(void|string domain)
 *!
 *! The textdomain() function sets or queries the name of the
 *! current domain of the active @[LC_MESSAGES] locale category. The
 *! @[domain] argument is a string that can contain only the
 *! characters allowed in legal filenames.
 *! 
 *! The domain argument is the unique name of a domain on the
 *! system. If there are multiple versions of the same domain on
 *! one system, namespace collisions can be avoided by using
 *! @[bindtextdomain()]. If textdomain() is not called, a default
 *! domain is selected. The setting of domain made by the last
 *! valid call to textdomain() remains valid across subsequent
 *! calls to @[setlocale()], and @[gettext()].
 *!
 *! @returns
 *!   The normal return value from textdomain() is a string
 *!   containing the current setting of the domain. If domainname is
 *!   void, textdomain() returns a string containing the current
 *!   domain. If textdomain() was not previously called and
 *!   domainname is void, the name of the default domain is
 *!   returned.
 *!
 *! @seealso
 *!   @[bindtextdomain], @[gettext], @[setlocale], @[localeconv]
 */
void f_textdomain(INT32 args)
{
  const char *domain = NULL;
  char *returnstring;
  get_all_args ("Locale.Gettext.textdomain", args, ".%C", &domain);
  returnstring = textdomain(domain);
  pop_n_elems(args);
  push_text(returnstring);
}

/*! @decl string bindtextdomain(string|void domainname, string|void dirname)
 *!
 *! Binds the path predicate for a message @[domainname] domainname to
 *! the directory name specified by @[dirname].
 *!
 *! If @[domainname] is a non-empty string and has not been bound
 *! previously, bindtextdomain() binds @[domainname] with @[dirname].
 *! 
 *! If @[domainname] is a non-empty string and has been bound previously,
 *! bindtextdomain() replaces the old binding with @[dirname].
 *!
 *! The @[dirname] argument can be an absolute or relative pathname
 *! being resolved when @[gettext()], @[dgettext()] or @[dcgettext()]
 *! are called.
 *!
 *! If @[domainname] is zero or an empty string, @[bindtextdomain()]
 *! returns 0.
 *!
 *! User defined domain names cannot begin with the string @expr{"SYS_"@}.
 *! Domain names beginning with this string are reserved for system use.
 *!
 *! @returns
 *!   The return value from @[bindtextdomain()] is a string containing
 *!   @[dirname] or the directory binding associated with @[domainname] if
 *!   @[dirname] is unspecified. If no binding is found, the default locale
 *!   path is returned. If @[domainname] is unspecified or is an empty string,
 *!   @[bindtextdomain()] takes no action and returns a 0.
 *!
 *! @seealso
 *!   @[textdomain], @[gettext], @[setlocale], @[localeconv]
 */
void f_bindtextdomain(INT32 args)
{
  char *returnstring;
  const char *domain = NULL, *dirname = NULL;
  get_all_args ("Locale.Gettext.bindtextdomain", args,
		".%C%C", &domain, &dirname);

  if (!domain || !*domain)
    returnstring = NULL;
  else {
#ifdef BINDTEXTDOMAIN_HANDLES_NULL
    returnstring = bindtextdomain (domain, dirname);
#else
    if (dirname)
      returnstring = bindtextdomain (domain, dirname);
    else
      /* Awkward, but not much we can do. Still better than a
       * coredump.. */
      Pike_error ("Pike has been compiled with a version of libintl "
		  "that doesn't support NULL as directory name.\n");
#endif
  }

  pop_n_elems(args);
  if(returnstring == NULL)
    push_int(0);
  else 
    push_text(returnstring);
}

/*! @decl int setlocale(int category, string locale)
 *!
 *! The setlocale() function is used to set the program's
 *! current locale. If @[locale] is "C" or "POSIX", the current
 *! locale is set to the portable locale.
 *! 
 *! If @[locale] is "", the locale is set to the default locale which
 *! is selected from the environment variable LANG.
 *! 
 *! The argument @[category] determines which functions are
 *! influenced by the new locale are @[LC_ALL], @[LC_COLLATE], @[LC_CTYPE],
 *! @[LC_MONETARY], @[LC_NUMERIC] and @[LC_TIME].
 *!
 *! @returns
 *!   Returns 1 if the locale setting successed, 0 for failure
 *!
 *! @seealso
 *!   @[bindtextdomain], @[textdomain], @[gettext], @[dgettext], @[dcgettext], @[localeconv]
 */
void f_setlocale(INT32 args)
{
  char *returnstring;
  const char *locale;
  int category;
  get_all_args("Locale.Gettext.setlocale", args, "%d%c", &category, &locale);

  returnstring = setlocale(category, locale);
  pop_n_elems(args);
  if(returnstring == NULL)
    push_int(0);
  else
    push_int(1);
}
  
/*! @decl mapping localeconv()
 *!
 *! The localeconv() function returns a mapping with settings for
 *! the current locale. This mapping contains all values
 *! associated with the locale categories @[LC_NUMERIC] and
 *! @[LC_MONETARY].
 *!
 *! @mapping
 *!   @member string "decimal_point"
 *!     The decimal-point character used to format
 *!     non-monetary quantities.
 *!
 *!   @member string "thousands_sep"
 *!     The character used to separate groups of digits to
 *!     the left of the decimal-point character in
 *! 	formatted non-monetary quantities.
 *!
 *!   @member string "int_curr_symbol"
 *!     The international currency symbol applicable to
 *!     the current locale, left-justified within a
 *!     four-character space-padded field. The character
 *!     sequences should match with those specified in ISO
 *!     4217 Codes for the Representation of Currency and
 *!     Funds.
 *!
 *!   @member string "currency_symbol"
 *!     The local currency symbol applicable to the
 *!     current locale.
 *!
 *!   @member string "mon_decimal_point"
 *!     The decimal point used to format monetary quantities.
 *!
 *!   @member string "mon_thousands_sep"
 *!     The separator for groups of digits to the left of
 *!     the decimal point in formatted monetary quantities.
 *!
 *!   @member string "positive_sign"
 *!     The string used to indicate a non-negative-valued
 *!     formatted monetary quantity.
 *!
 *!   @member string "negative_sign"
 *!     The string used to indicate a negative-valued
 *!     formatted monetary quantity.
 *!
 *!   @member int "int_frac_digits"
 *!     The number of fractional digits (those to the
 *!     right of the decimal point) to be displayed in an
 *!     internationally formatted monetary quantity.
 *!
 *!   @member int "frac_digits"
 *!     The number of fractional digits (those  to  the
 *!     right of the decimal point) to be displayed in a
 *!     formatted monetary quantity.
 *!
 *!   @member int(0..1) "p_cs_precedes"
 *!     Set to 1 or 0 if the currency_symbol respectively
 *!     precedes or succeeds the value for a non-negative
 *!     formatted monetary quantity.
 *!
 *!   @member int(0..1) "p_sep_by_space"
 *!     Set to 1 or 0 if the currency_symbol respectively
 *!     is or is not separated by a space from the value
 *!     for a non-negative formatted monetary quantity.
 *!
 *!   @member int(0..1) "n_cs_precedes"
 *!     Set to 1 or 0 if the currency_symbol respectively
 *!     precedes or succeeds the value for a negative
 *!     formatted monetary quantity.
 *!
 *!   @member int(0..1) "n_sep_by_space"
 *!     Set to 1 or 0 if the currency_symbol respectively
 *!     is or is not separated by a space from the value
 *!     for a negative formatted monetary quantity.
 *! 
 *!   @member int(0..4) "p_sign_posn"
 *!     Set to a value indicating the positioning of the
 *!     positive_sign for a non-negative formatted
 *!     monetary quantity. The value of p_sign_posn is
 *!     interpreted according to the following:
 *!
 *!     @int
 *!       @value 0
 *!         Parentheses surround the quantity and currency_symbol.
 *!       @value 1
 *!         The sign string precedes the quantity and currency_symbol.
 *!       @value 2
 *!         The sign string succeeds the quantity and currency_symbol.
 *!       @value 3
 *!         The sign string immediately precedes the currency_symbol.
 *!       @value 4
 *!         The sign string immediately succeeds the currency_symbol.
 *!     @endint
 *!
 *!   @member int "n_sign_posn"
 *!     Set to a value indicating the positioning of the
 *!     negative_sign for a negative formatted monetary
 *!     quantity. The value of n_sign_posn is interpreted
 *!     according to the rules described under p_sign_posn.
 *! @endmapping
 *!
 *! @seealso
 *!   @[bindtextdomain], @[textdomain], @[gettext], @[dgettext], @[dcgettext], @[setlocale]
 */
void f_localeconv(INT32 args)
{
  struct lconv *locale; /* Information about the current locale */
  struct svalue *save_sp = Pike_sp;

  locale = localeconv();

#define MAPSTR(key) do {		\
    push_constant_text(TOSTR(key));	\
    push_text(locale->key);		\
  } while(0)
#define MAPINT(key) do {		\
    push_constant_text(TOSTR(key));	\
    push_int(locale->key);		\
  } while(0)

  MAPSTR(decimal_point);
  MAPSTR(thousands_sep);
  MAPSTR(int_curr_symbol);
  MAPSTR(currency_symbol);
  MAPSTR(mon_decimal_point);
  MAPSTR(mon_thousands_sep);
  MAPSTR(positive_sign);
  MAPSTR(negative_sign);

  /*
   * MAPCHAR(grouping);
   * MAPCHAR(mon_grouping);
   */

  MAPINT(int_frac_digits);
  MAPINT(frac_digits);
  MAPINT(p_cs_precedes);
  MAPINT(p_sep_by_space);
  MAPINT(n_cs_precedes);
  MAPINT(n_sep_by_space);
  MAPINT(p_sign_posn);
  MAPINT(n_sign_posn);

  f_aggregate_mapping(Pike_sp - save_sp);

  stack_pop_n_elems_keep_top(args);
}

/*! @decl constant LC_ALL
 *!
 *! Locale category for all of the locale.
 */

/*! @decl constant LC_COLLATE
 *!
 *! Locale category for the functions strcoll() and
 *! strxfrm() (used by pike, but not directly accessible).
 */

/*! @decl constant LC_CTYPE
 *!
 *! Locale category for the character classification and
 *! conversion routines.
 */

/*! @decl constant LC_MESSAGES
 *!
 *! @fixme
 *!   Document this constant.
 *! @note
 *!   This category isn't available on all platforms.
 */

/*! @decl constant LC_MONETARY
 *!
 *! Locale category for localeconv().
 */

/*! @decl constant LC_NUMERIC
 *!
 *! Locale category for the decimal character.
 */

/*! @decl constant LC_TIME
 *!
 *! Locale category for strftime() (currently not accessible
 *! from Pike).
 */

/*! @endmodule
 */

/*! @endmodule
 */

PIKE_MODULE_INIT
{

/* function(void:string) */

  ADD_FUNCTION("gettext", f_gettext,
	       tFunc(tStr tOr(tStr,tVoid) tOr(tInt,tVoid),tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("setlocale", f_setlocale, tFunc(tInt tStr,tInt),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("dgettext", f_dgettext, tFunc(tStr tStr,tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("dcgettext", f_dcgettext, tFunc(tStr tStr tInt,tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("bindtextdomain", f_bindtextdomain,
	       tFunc(tOr(tStr,tVoid) tOr(tStr,tVoid), tOr(tStr,tVoid)),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("textdomain", f_textdomain, tFunc(tOr(tStr, tVoid),tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("localeconv", f_localeconv, tFunc(tVoid, tMapping),
	       OPT_EXTERNAL_DEPEND);

  add_integer_constant("LC_ALL", LC_ALL, 0);
  add_integer_constant("LC_COLLATE", LC_COLLATE, 0);
  add_integer_constant("LC_CTYPE", LC_CTYPE, 0);
#ifdef LC_MESSAGES
  add_integer_constant("LC_MESSAGES", LC_MESSAGES, 0);
#endif
  add_integer_constant("LC_MONETARY", LC_MONETARY, 0);
  add_integer_constant("LC_NUMERIC", LC_NUMERIC, 0);
  add_integer_constant("LC_TIME", LC_TIME, 0);
}

PIKE_MODULE_EXIT
{

}
#else

#include "module.h"
#include "module_support.h"
#include "program.h"

PIKE_MODULE_INIT {
  if(!TEST_COMPAT(7,6))
    HIDE_MODULE();
}

PIKE_MODULE_EXIT {}

#endif
