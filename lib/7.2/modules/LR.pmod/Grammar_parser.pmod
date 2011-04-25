#!/usr/local/bin/pike

#pike 7.2

/*
 * $Id$
 *
 * Generates a parser from a textual specification.
 *
 * Henrik Grubbström 1996-12-06
 */

//! This module generates an LR parser from a grammar specified according
//! to the following grammar:
//!
//! @pre{
//!        directives : directive ;
//!	   directives : directives directive ;
//!	   directive : declaration ;
//!	   directive : rule ;
//!	   declaration : "%token" terminals ";" ;
//!	   rule : nonterminal ":" symbols ";" ;
//!	   rule : nonterminal ":" symbols action ";" ;
//!	   symbols : symbol ;
//!	   symbols : symbols symbol ;
//!	   terminals : terminal ;
//!	   terminals : terminals terminal ;
//!	   symbol : nonterminal ;
//!	   symbol : "string" ;
//!	   action : "{" "identifier" "}" ;
//!	   nonterminal : "identifier" ;
//!	   terminal : "string";
//! @}

/*
 * Includes
 */

#include <stdio.h>

/*
 * Defines
 */

/* #define DEBUG */

/* Errors during parsing */
/* Action for rule is missing from master object */
#define ERROR_MISSING_ACTION	256
/* Action for rule is not a function */
#define ERROR_BAD_ACTION	512

import LR;

static private object(parser) _parser = parser();

/*
 * Scanner
 */

static private class scan {
  string str = "";
  int pos;

  array|string scan()
  {
    while (1) {
      if (pos >= sizeof(str)) {
	/* EOF */
	return("");
      } else {
	int start=pos++;
	switch (str[start]) {
	case '%':
	  /* Token */
	  while ((pos < sizeof(str)) &&
		 ((('A' <= str[pos]) && ('Z' >= str[pos])) ||
		  (('a' <= str[pos]) && ('z' >= str[pos])))) {
	    pos++;
	  }
	  return (lower_case(str[start..pos-1]));
	case '\"':
	  /* String */
	  while ((pos < sizeof(str)) &&
		 (str[pos] != '\"')) {
	    if (str[pos] == '\\') {
	      pos++;
	    }
	    pos++;
	  }
	  if (pos < sizeof(str)) {
	    pos++;
	  } else {
	    pos = sizeof(str);
	  }
	  if (start != pos-2) {
	    return ({ "string", str[start+1..pos-2] });
	  }
	  /* Throw away empty strings (EOF) */
	  break;
	case '/':
	  /* Comment */
	  if (str[pos] != '*') {
	    werror(sprintf("Bad token \"/%c\" in input\n", str[pos]));
	    break;
	  }
	  pos++;
	  while (1) {
	    if ((++pos >= sizeof(str)) ||
		(str[pos-1 .. pos] == "*/")) {
	      pos++;
	      break;
	    }
	  }
	  break;
	case '(':
	  return("(");
	case ')':
	  return(")");
	case '{':
	  return("{");
	case '}':
	  return("}");
	case ':':
	  return(":");
	case ';':
	  return(";");
	case '\n': 
	case '\r':
	case ' ':
	case '\t':
	  /* Whitespace */
	  break;
	case '0'..'9':
	  /* int */
	  while ((pos < sizeof(str)) &&
		 ('0' <= str[pos]) && ('9' >= str[pos])) {
	    pos++;
	  }
	  return (({ "int", (int)str[start .. pos-1] }));
	default:
	  /* Identifier */
	  while ((pos < sizeof(str)) &&
		 ((('A' <= str[pos]) && ('Z' >= str[pos])) ||
		  (('a' <= str[pos]) && ('z' >= str[pos])) ||
		  (('0' <= str[pos]) && ('9' >= str[pos])) ||
		  ('_' == str[pos]) || (str[pos] >= 128))) {
	    pos++;
	  }
	  return (({ "identifier", str[start .. pos-1] }));
	}
      }
    }
  }
}

static private object(scan) scanner = scan();

static private array(string) nonterminals = ({
  "translation_unit",
  "directives",
  "directive",
  "declaration",
  "rule",
  "symbols",
  "terminals",
  "symbol",
  "action",
  "nonterminal",
  "terminal",
  "priority",
});

static private object(ADT.Stack) id_stack = ADT.Stack();

static private mapping(string:int) nonterminal_lookup = ([]);

static private object(parser) g;

static private object master;

//! Error code from the parsing.
int error;

static private int add_nonterminal(string id)
{
  int nt = nonterminal_lookup[id];

  if (!nt) {
    nt = nonterminal_lookup[id] = id_stack->ptr;
    id_stack->push(id);
  }
  return(nt);
}

static private void add_tokens(array(string) tokens)
{
  /* NOOP */
#if 0
  if (sizeof(tokens)) {
    map(tokens, add_token);
  }
#endif /* 0 */
}

static private void set_left_tokens(string ignore, int pri_val, array(string) tokens)
{
  foreach (tokens, string token) {
    g->set_associativity(token, -1);	/* Left associative */
    g->set_priority(token, pri_val);
  }
}

static private string internal_symbol_to_string(int|string symbol)
{
  if (intp(symbol)) {
    return (nonterminals[symbol]);
  } else {
    return ("\"" + symbol + "\"");
  }
}

static private string symbol_to_string(int|string symbol)
{
  if (intp(symbol)) {
    if (symbol < id_stack->ptr) {
      return (id_stack->arr[symbol]);
    } else {
      /* Only happens with the initial(automatic) rule */
      return ("nonterminal"+symbol);
    }
  } else {
    return ("\""+symbol+"\"");
  }
}

static private void add_rule(int nt, string colon, array(mixed) symbols, string action)
{
  if (action == ";") {
    action = 0;
  }
  if ((action) && (master)) {
    if (!master[action]) {
      werror(sprintf("Warning: Missing action %s\n", action));

      error |= ERROR_MISSING_ACTION;
    } else if (!functionp(master[action])) {
      werror(sprintf("Warning: \"%s\" is not a function in object\n",
		     action));
      
      error |= ERROR_BAD_ACTION;
    } else {
      g->add_rule(rule(nt, symbols, master[action]));
      return;
    }
  }
  g->add_rule(rule(nt, symbols, action));
}

void create()
{
  _parser->set_symbol_to_string(internal_symbol_to_string);

  _parser->verbose = 0;

  _parser->add_rule(rule(0, ({ 1, "" }), 0));		/* translation_unit */
  _parser->add_rule(rule(1, ({ 2 }), 0));		/* directives */
  _parser->add_rule(rule(1, ({ 1, 2 }), 0));		/* directives */
  _parser->add_rule(rule(2, ({ 3 }), 0));		/* directive */
  _parser->add_rule(rule(2, ({ 4 }), 0));		/* directive */
  _parser->add_rule(rule(3, ({ "%token", 6, ";" }),
			 add_tokens));			/* declaration */
  _parser->add_rule(rule(3, ({ "%left", 11, 6, ";" }),
			 set_left_tokens));		/* declaration */
  _parser->add_rule(rule(4, ({ 9, ":", 5, ";" }),
			 add_rule));			/* rule */
  _parser->add_rule(rule(4, ({ 9, ":", 5, 8, ";" }),
			 add_rule));			/* rule */
  _parser->add_rule(rule(5, ({}),
			 lambda () {
			   return ({}); } ));		/* symbols */
  _parser->add_rule(rule(5, ({ 5, 7 }),
			 lambda (array x, mixed|void y) {
			   if (y) { return (x + ({ y })); }
			   else { return (x); }} ));	/* symbols */
  _parser->add_rule(rule(6, ({ 10 }),
			 lambda (string x) {
			   return ({ x }); } ));	/* terminals */
  _parser->add_rule(rule(6, ({ 6, 10 }),
			 lambda (array(string) x, string y) {
			   return (x + ({ y })); } ));	/* terminals */
  _parser->add_rule(rule(7, ({ 9 }), 0 ));		/* symbol */
  _parser->add_rule(rule(7, ({ 10 }), 0 ));		/* symbol */
  _parser->add_rule(rule(8, ({ "{", "identifier", "}" }),
			 lambda (mixed brace_l, string id, mixed brace_r) {
			   return (id); } ));		/* action */
  _parser->add_rule(rule(8, ({ "{", "string", "}" }),
			 lambda (mixed brace_l, string str, mixed brace_r) {
			   werror(sprintf("Warning: Converting string \"%s\" "
					  "to identifier\n", str));
			   return(str); } ));		/* action */
  _parser->add_rule(rule(9, ({ "identifier" }),
			 add_nonterminal));		/* nonterminal */
  _parser->add_rule(rule(10, ({ "string" }), 0));	/* terminal */
  _parser->add_rule(rule(11, ({ "(", "int", ")" }),
			 lambda (mixed paren_l, int val, mixed paren_r) {
			   return (val);
			 } ));				/* priority */
  _parser->add_rule(rule(11, ({}), 0));			/* priority */

  _parser->compile();
}

//! Compiles the parser-specification given in the first argument.
//! Named actions are taken from the object if available, otherwise
//! left as is.
//!
//! @bugs
//! Returns error-code in both Grammar_parser.error and
//! return_value->error.
object(parser) make_parser(string str, object|void m)
{
  object(parser) res = 0;

  master = m;
  error = 0;	/* No errors yet */

  g = parser();

  scanner->str = str;
  scanner->pos = 0;

  g->set_symbol_to_string(symbol_to_string);

  id_stack = ADT.Stack();

  nonterminal_lookup = ([]);

#ifdef DEBUG
  _parser->verbose = 1;
  g->verbose = 1;
#else
  g->verbose = 0;
#endif /* DEBUG */

  /* Default rule -- Will never be reduced */
  id_stack->push("Translation Unit");	/* Nonterminal #0 -- Start symbol */
  g->add_rule(rule(0, ({ 1, "" }), 0));	/* Rule #0 -- Start rule */

  _parser->parse(scanner->scan);

  if ((!_parser->error) &&
      (!error) &&
      (!g->compile())) {
    res = g;
  }

  g = 0;	/* Don't keep any references */

  return (res);
}

//! Compiles the file specified in the first argument into an LR parser.
//!
//! @seealso
//!   @[make_parser]
int|object(parser) make_parser_from_file(string fname, object|void m)
{
  return make_parser(Stdio.read_file(fname), m);
}

/*
 * Syntax-checks and compiles the grammar files
 */
int main(int argc, array(string) argv)
{
  if (argc == 1) {
    werror(sprintf("Usage:\n\t%s <files>\n", argv[0]));
  } else {
    int i;

    for (i=1; i < argc; i++) {
      werror(sprintf("Compiling \"%s\"...\n", argv[i]));

      int|object(parser) g = make_parser_from_file(argv[i]);
      if (error || !g) {
	werror("Compilation failed\n");
      } else {
	werror("Compilation done\n");
      }
    }
  }
}
