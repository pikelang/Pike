// $Id: Subject.pike,v 1.2 2002/10/21 02:10:54 grendel Exp $

//! This is a probe subject which you can send in somewhere to
//! get probed (not to be confused with a probe object, which
//! does some active probing). All calls to lfuns will be printed
//! to stderr. It is possible to name a subject by passing one
//! string as first and only argument when creating the subject
//! object.
//!
//! @example
//! > object s = Debug.Subject();
//! create got ({ })
//! > random(s);
//! _random got ({ })
//! (1) Result: 0
//! > abs(s);
//! `< got ({ /* 1 element */
//! 	0
//! })
//!   _sprintf got ({ /* 2 elements */
//! 	79,
//! 	([ /* 1 element */
//! 	  "indent":2
//! 	])
//! })
//! (2) Result: Debug.Subject
//! > abs(class { inherit Debug.Subject; int `<(mixed ... args) { return 1; } }());
//! create got ({ })
//! `- got ({ })
//! destroy got ({ })
//! (3) Result: 0
//! > pow(s,2);
//! `[] got ({ /* 1 element */
//! 	"pow"
//! })
//! Attempt to call the NULL-value
//! Unknown program: 0(2)
//! HilfeInput:1: ___HilfeWrapper()

#define PROXY(X,Y) X(mixed ... args) { werror(id+#X+" got %O\n", args); return Y; }

static string id = "";

void create(mixed ... args) {
  werror("create got %O\n", args);
  if(sizeof(args)==1 && stringp(args[0]))
    id = "(" + args[0] + ") ";
}

void PROXY(destroy, 0);

mixed PROXY(`->, 0);
mixed PROXY(`->=, 0);
mixed PROXY(`[], 0);
mixed PROXY(`[]=, 0);

mixed PROXY(`+, 0);
mixed PROXY(`-, 0);
mixed PROXY(`&, 0);
mixed PROXY(`|, 0);
mixed PROXY(`^, 0);
mixed PROXY(`<<, 0);
mixed PROXY(`>>, 0);
mixed PROXY(`*, 0);
mixed PROXY(`/, 0);
mixed PROXY(`%, 0);
mixed PROXY(`~, 0);

int(0..1) PROXY(`==, 0);
int(0..1) PROXY(`<, 0);
int(0..1) PROXY(`>, 0);
int PROXY(`!, 0);

int PROXY(__hash, 0);
int PROXY(_sizeof, 0);
mixed PROXY(cast, 0);
mixed PROXY(`(), 0);

mixed PROXY(``+, 0);
mixed PROXY(``-, 0);
mixed PROXY(``&, 0);
mixed PROXY(``|, 0);
mixed PROXY(``^, 0);
mixed PROXY(``<<, 0);
mixed PROXY(``>>, 0);
mixed PROXY(``*, 0);
mixed PROXY(``/, 0);
mixed PROXY(``%, 0);

mixed PROXY(`+=, 0);

int(0..1) PROXY(_is_type, 0);
int PROXY(_equal, 0);
mixed PROXY(_m_delete, 0);

array _indices(mixed ... args) { 
   werror(id+"_values got %O\n", args); 
   return ::_indices(); 
}

array _values(mixed ... args) { 
   werror(id+"_values got %O\n", args); 
   return ::_values(); 
}

object _get_iterator(mixed ... args) {
  werror(id+"_get_iterator got %O\n", args);
  string iid = id==""?"":id[1..sizeof(id)-3];
  return this_program("("+iid+" iterator) ");
}

string _sprintf(mixed ... args) {
  werror(id+"_sprintf got %O\n", args);
  return sprintf("Debug.Subject"+id);
}

mixed PROXY(_random, 0);
