
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
#include "error.h"
#include "pike_macros.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "module_support.h"

RCSID("$Id: gettext.c,v 1.2 2000/02/29 20:37:39 neotron Exp $");

/*
**! module Locale.Gettext
**!
**!	This module enables access to localization functions from within Pike.
**!
**! note
**!	$Id: gettext.c,v 1.2 2000/02/29 20:37:39 neotron Exp $
*/

/******************** PUBLIC FUNCTIONS BELOW THIS LINE */

/*
**! method string gettext(string msg)
**!
**!	Return a translated version of msg within the context
**!	of the current domain and locale.
**!
**!	If there is not translation available, msg is returned.
**! arg string msg
**!	The message to translate.
**! see also: bindtextdomain, textdomain, dgettext, dcgettext, setlocale, localeconv
*/
void f_gettext(INT32 args)
{
  char *translated;
  if (args != 1)
    error( "Wrong number of arguments to Gettext.gettext()\n" );
  if (sp[-1].type != T_STRING)
    error( "Bad argument 1 to Gettext.gettext(), expected string\n" );
  translated = gettext(sp[-1].u.string->str);

  pop_n_elems(args);
  push_string(make_shared_string(translated));
}

/*
**! method string dgettext(string domain, string msg)
**!
**!	Return a translated version of msg within the context
**!	of the specified domain and current locale.
**!
**!	If there is not translation available, msg is returned.
**!
**! arg string domain
**!	The domain to lookup the message in.
**! arg string msg
**!	The message to translate.
**! see also: bindtextdomain, textdomain, gettext, dcgettext, setlocale, localeconv
*/
void f_dgettext(INT32 args)
{
  char *translated;
  struct pike_string *domain, *msg;
  get_all_args("Gettext.dgettext", args, "%S%S", &domain, &msg);

  translated = dgettext(domain->str, msg->str);

  pop_n_elems(args);
  push_string(make_shared_string(translated));
}

/*
**! method string dcgettext(string domain, string msg, int category)
**!
**!	Return a translated version of msg within the context of the
**!	specified domain and current locale for the specified
**!	category. Calling dcgettext with category
**!	Locale.Gettext.LC_MESSAGES gives the same result as dgettext.
**!
**!	If there is not translation available, msg is returned.
**!
**! arg string domain
**!	The domain to lookup the message in.
**! arg string msg
**!	The message to translate.
**! arg int category
**!	The category which locale should be used.
**! see also: bindtextdomain, textdomain, gettext, dgettext, setlocale, localeconv
*/
void f_dcgettext(INT32 args)
{
  char *translated;
  struct pike_string *domain, *msg;
  int category;
  get_all_args("Gettext.dcgettext", args, "%S%S%d", &domain, &msg, &category);

  translated = dcgettext(domain->str, msg->str, category);

  pop_n_elems(args);
  push_string(make_shared_string(translated));
}
/*
**! method string textdomain(string domain|void)
**! method string textdomain(void)
**!
**!	The textdomain() function sets or queries the name of the
**!	current domain of the active LC_MESSAGES locale category. The
**!	domainname argument is a string that can contain only the
**!	characters allowed in legal filenames.
**!	
**!	The domainname argument is the unique name of a domain on the
**!	system. If there are multiple versions of the same domain on
**!	one system, namespace collisions can be avoided by using
**!	bindtextdomain(). If textdomain() is not called, a default
**!	domain is selected. The setting of domain made by the last
**!	valid call to textdomain() remains valid across subsequent
**!	calls to setlocale, and gettext().
**!
**!	The normal return value from textdomain() is a string
**!	containing the current setting of the domain. If domainname is
**!	void, textdomain() returns a string containing the current
**!	domain. If textdomain() was not previously called and
**!	domainname is void, the name of the default domain is
**!	returned.
**¡
**! arg string domainname
**!	The name of the domain to be made the current domain.
**! see also: bindtextdomain, gettext, dgettext, dcgettext, setlocale, localeconv
*/

void f_textdomain(INT32 args)
{
  char *domain=NULL, *returnstring;
  if (args != 0 && args != 1)
    error( "Wrong number of arguments to Gettext.textdomain()\n" );

  if(sp[-args].type == T_STRING)
    domain = sp[-args].u.string->str;
  else if(!(sp[-args].type == T_INT && sp[-args].u.integer == 0))
    error( "Bad argument 1 to Gettext.textdomain(), expected string|void\n" );
  returnstring = textdomain(domain);
  pop_n_elems(args);
  push_string(make_shared_string(returnstring));
}

/*
**! method string bindtextdomain(string|void domainname, string|void dirname)
**!
**!	The bindtextdomain() function binds the path predicate for a
**!	message domain domainname to the value contained in dirname. If
**!	domainname is a non-empty string and has not been bound
**!	previously, bindtextdomain() binds domainname with dirname. 
**!	
**!	If domainname is a non-empty string and has been bound previously,
**!	bindtextdomain() replaces the old binding with dirname. The dirname
**!	argument can be an absolute or relative pathname being resolved when
**!	gettext(), dgettext(), or dcgettext() are called. If domainname is 
**!	null pointer or an empty string, bindtextdomain() returns 0.  User
**!	defined domain names cannot begin with the string SYS_.  Domain names
**!	beginning with this string are reserved for system use.
**!
**!	The return value from bindtextdomain() is a string containing
**!	dirname or the directory binding associated with domainname if
**!	dirname is void. If no binding is found, the default locale
**!	path is returned. If domainname is void or an empty string,
**!	bindtextdomain() takes no action and returns a 0.
**!
**! arg string domainname
**!	The domain to query or bind a path to.
**! arg string dirname
**!	The directory name to bind to the choosen domain.
**! see also: textdomain, gettext, dgettext, dcgettext, setlocale, localeconv
*/

void f_bindtextdomain(INT32 args)
{
  char *returnstring, *domain = NULL, *dirname = NULL;
  if (args < 1 || args > 2)
    error( "Wrong number of arguments to Gettext.bindtextdomain()\n" );
  switch(args)
  {
   case 2:
    if(sp[-1].type == T_STRING)
      dirname = sp[-1].u.string->str;
    else if(!(sp[-1].type == T_INT && sp[-1].u.integer == 0))
      error( "Bad argument 2 to Gettext.bindtextdomain(), expected string|void\n" );
    /* FALLTHROUGH */
    
   case 1:
    if(sp[-args].type == T_STRING)
      domain = sp[-args].u.string->str;
    else if(!(sp[-args].type == T_INT && sp[-args].u.integer == 0))
      error( "Bad argument 1 to Gettext.bindtextdomain(), expected string|void\n" );
  }
  returnstring = bindtextdomain(domain, dirname);
  pop_n_elems(args);
  if(returnstring == NULL)
    push_int(0);
  else 
    push_string(make_shared_string(returnstring));
}

/*
**! method int setlocale(int category, string locale)
**!
**!	The setlocale() function is used to set the program's
**!	current locale. If locale is "C" or "POSIX", the current
**!	locale is set to the portable locale.
**!	
**!	If locale is "", the locale is set to the default locale which
**!	is selected from the environment variable LANG.
**!	
**!	The argument category determines which functions are
**!	influenced by the new locale:
**!
**!	<b>Locale.Gettext.LC_ALL</b> for all of the locale.
**!	
**!	<b>Locale.Gettext.LC_COLLATE</b> for the functions strcoll() and
**!	strxfrm() (used by pike, but not directly accessible).
**!	
**!	<b>Locale.Gettext.LC_CTYPE</b> for the character classification and
**!	conversion routines.
**!	
**!	<b>Locale.Gettext.LC_MONETARY</b> for localeconv().
**!
**!	<b>Locale.Gettext.LC_NUMERIC</b> for the decimal character.
**!
**!	<b>Locale.Gettext.LC_TIME</b> for strftime() (currently not accessible
**!	from Pike).
**!
**! arg int category
**!	The category in which to set the locale.
**! arg string locale
**!	The locale to change to
**! returns 1 if the locale setting successed, 0 for failure
**! see also: bindtextdomain, textdomain, gettext, dgettext, dcgettext, localeconv
*/

void f_setlocale(INT32 args)
{
  char *returnstring;
  struct pike_string *locale;
  int category;
  get_all_args("Gettext.setlocale", args, "%d%S", &category, &locale);

  fprintf(stderr, "locale: %s, category: %d\n", locale->str, category);
  returnstring = setlocale(category, locale->str);
  pop_n_elems(args);
  if(returnstring == NULL)
    push_int(0);
  else
    push_int(1);
}
  
#define MAPSTR(key, value) do {\
  struct svalue val; struct pike_string *valstr; \
  val.type = T_STRING; \
  valstr = make_shared_string(locale->value);\
  val.u.string = valstr; \
  mapping_string_insert(map, make_shared_string(key), &val);\
  free_string(valstr); \
  } while(0)
#define MAPINT(key, value) do {\
  struct svalue val; struct pike_string *valstr; \
  val.type = T_INT; \
  val.u.integer = (int)locale->value; \
  mapping_string_insert(map, make_shared_string(key), &val);\
  } while(0)

**! method mapping localeconv()
**!	     The localeconv() function returns a mapping with settings for
**!	     the current locale. This mapping contains all values
**!	     associated with the locale categories LC_NUMERIC and
**!	     LC_MONETARY.
**!	
**!	     <b>string decimal_point:</b>
**!	               The decimal-point character used  to  format  
**!	               non-monetary quantities.
**!	
**!	     <b>string thousands_sep:</b>
**!	               The character used to separate groups of digits to
**!	               the left of the decimal-point character in
**!		       formatted non-monetary quantities.
**!	
**!	     <b>string int_curr_symbol:</b>
**!	               The international currency symbol applicable to
**!	               the  current locale, left-justified within a 
**!	               four-character space-padded field. The character
**!	               sequences should match with those specified in ISO
**!	               4217 Codes for the Representation of Currency and
**!	               Funds.
**!	
**!	     <b>string currency_symbol:</b>
**!	               The local currency symbol applicable to the
**!	               current locale.
**!	
**!	     <b>string mon_decimal_point:</b>
**!	               The decimal point used to format monetary quantities.
**!	
**!	     <b>string mon_thousands_sep:</b>
**!	               The separator for groups of digits to the left of
**!	               the decimal point in formatted monetary quantities.
**!	
**!	     <b>string positive_sign:</b>
**!	               The string used to indicate a non-negative-valued
**!	               formatted monetary quantity.
**!	
**!	     <b>string negative_sign:</b>
**!	               The string used to indicate a negative-valued 
**!	               formatted monetary quantity.
**!	
**!	     <b>int int_frac_digits:</b>
**!	               The number of fractional digits (those to the
**!	               right of the decimal point) to be displayed in an
**!	               internationally formatted monetary quantity.
**!	
**!	     <b>int frac_digits:</b>
**!	               The number of fractional digits (those  to  the
**!	               right of the decimal point) to be displayed in a
**!	               formatted monetary quantity.
**!	
**!	     <b>int p_cs_precedes:</b>
**!	               Set to 1 or 0 if the currency_symbol respectively
**!	               precedes or succeeds the value for a non-negative
**!	               formatted monetary quantity.
**!	
**!	     <b>int p_sep_by_space:</b>
**!	               Set to 1 or 0 if the currency_symbol respectively
**!	               is or is not separated by a space from the value
**!	               for a non-negative formatted monetary quantity.
**!	
**!	     <b>int n_cs_precedes:</b>
**!	               Set to 1 or 0 if the currency_symbol  respectively
**!	               precedes or succeeds the value for a negative 
**!	               formatted monetary quantity.
**!	
**!	     <b>int n_sep_by_space:</b>
**!	               Set to 1 or 0 if the currency_symbol  respectively
**!	               is or is not separated by a space from the value
**!	               for a negative formatted monetary quantity.
**!	
**!	     <b>int p_sign_posn:</b>
**!	               Set to a value indicating the positioning  of  the
**!	               positive_sign  for  a non-negative formatted 
**!	               monetary quantity. The value of p_sign_posn is  
**!	               interpreted according to the following:
**!	
**!	               0 - Parentheses surround the quantity and
**!			   currency_symbol.
**!	
**!	               1 - The sign string precedes the quantity and 
**!			   currency_symbol.
**!	
**!	               2 - The sign string  succeeds  the  quantity
**!	                   and currency_symbol.
**!	
**!	               3 - The sign string immediately precedes the 
**!			   currency_symbol.
**!	
**!	               4 - The sign string immediately succeeds the
**!	                   currency_symbol.
**!	
**!	     <b>int n_sign_posn:</b>
**!	               Set to a value indicating the positioning of the
**!	               negative_sign  for a negative formatted monetary
**!	               quantity. The value of n_sign_posn is interpreted
**!	               according to the rules described under p_sign_posn.
**! see also: bindtextdomain, textdomain, gettext, dgettext, dcgettext, setlocale
void f_localeconv(INT32 args)
{
  struct lconv *locale; /* Information about the current locale */
  struct mapping *map;
  map = allocate_mapping(18);
  locale = localeconv();
  
  MAPSTR("decimal_point", decimal_point);  
  MAPSTR("thousands_sep", thousands_sep);
  MAPSTR("int_curr_symbol", int_curr_symbol);
  MAPSTR("currency_symbol", currency_symbol);
  MAPSTR("mon_decimal_point", mon_decimal_point);
  MAPSTR("mon_thousands_sep", mon_thousands_sep);
  MAPSTR("positive_sign", positive_sign);
  MAPSTR("negative_sign", negative_sign);

  /*
    MAPCHAR("grouping", grouping);
    MAPCHAR("mon_grouping", mon_grouping);
  */

  MAPINT("int_frac_digits", int_frac_digits);
  MAPINT("frac_digits", frac_digits);
  MAPINT("p_cs_precedes", p_cs_precedes);
  MAPINT("p_sep_by_space", p_sep_by_space);
  MAPINT("n_cs_precedes", n_cs_precedes);
  MAPINT("n_sep_by_space", n_sep_by_space);
  MAPINT("p_sign_posn", p_sign_posn);
  MAPINT("n_sign_posn", n_sign_posn);
  push_mapping(map);
}

void pike_module_init(void)
{
  
/* function(void:string) */

  ADD_FUNCTION("gettext", f_gettext, tFunc(tStr,tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("setlocale", f_setlocale, tFunc(tInt tStr,tInt),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("dgettext", f_dgettext, tFunc(tStr tStr,tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("dcgettext", f_dcgettext, tFunc(tStr tStr tInt,tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("bindtextdomain", f_bindtextdomain, tFunc(tOr(tStr,tVoid) tOr(tStr,tVoid), tOr(tStr,tVoid)),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("textdomain", f_textdomain, tFunc(tOr(tStr, tVoid),tStr),
	       OPT_EXTERNAL_DEPEND);
  ADD_FUNCTION("localeconv", f_localeconv, tFunc(tVoid, tMapping), OPT_EXTERNAL_DEPEND);
  
  add_integer_constant("LC_ALL", LC_ALL, 0);
  add_integer_constant("LC_COLLATE", LC_COLLATE, 0);
  add_integer_constant("LC_CTYPE", LC_CTYPE, 0);
  add_integer_constant("LC_MESSAGES", LC_MESSAGES, 0);
  add_integer_constant("LC_MONETARY", LC_MONETARY, 0);
  add_integer_constant("LC_NUMERIC", LC_NUMERIC, 0);
  add_integer_constant("LC_TIME", LC_TIME, 0);
}

void pike_module_exit(void)
{
  
}
#else

void pike_module_init(void) {}
void pike_module_exit(void) {}

#endif
