// $Id$

#pike __REAL_VERSION__

//! This is a probe subject which you can send in somewhere to
//! get probed (not to be confused with a probe object, which
//! does some active probing). All calls to LFUNs will be printed
//! to stderr. It is possible to name the subject by passing a
//! string as the first and only argument when creating the subject
//! object.
//!
//! @example
//! > object s = Debug.Subject();
//! create()
//! > random(s);
//! _random()
//! (1) Result: 0
//! > abs(s);
//! `<(0)
//! _sprintf(79, ([ "indent":2 ]))
//! (2) Result: Debug.Subject
//! > abs(class { inherit Debug.Subject; int `<(mixed ... args) { return 1; } }());
//! create()
//! `-()
//! destroy()
//! (3) Result: 0
//! > pow(s,2);
//! `[]("pow")
//! Attempt to call the NULL-value
//! Unknown program: 0(2)

#define ENTER(X)                       \
  string t = sprintf("%{%O, %}", args); \
  werror("%s%s(%s)\n", id, #X,           \
	 has_suffix(t, ", ") ? t[..sizeof(t)-3] : t)

#define PROXY(X,Y) X(mixed ... args) { ENTER(X); return Y; }

static string id = "";

void create(mixed ... args)
{
  ENTER(create);
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

//! @ignore
array PROXY(_indices, ::_indices());
array PROXY(_values, ::_values());
//! @endignore

object _get_iterator(mixed ... args)
{
  ENTER(_get_iterator);
  string iid = id==""?"":id[1..sizeof(id)-3];
  return this_program("("+iid+" iterator) ");
}

string _sprintf(int|void t, mapping|void opt, mixed ... x)
{
  string args = "";
  if(t)
    args += sprintf("'%c'", t);
  if(opt == ([]))
    args += ", ([])";
  else if(opt)
  {
    string tmp = sprintf("%O", opt);
    sscanf(tmp, "([ /*%*s*/\n  %s\n])", tmp);
    args += sprintf(", ([ %s ])", replace(tmp, "\n ", ""));
  }
  string tmp = sprintf("%{%O, %}", x);
  if(has_suffix(tmp, ", "))
    tmp = tmp[..sizeof(tmp)-3];
  if(sizeof(tmp))
    args += ", " + tmp;
  werror("%s_sprintf(%s)\n", id, args);
  return "Debug.Subject" + id[..sizeof(id)-2];
}

mixed PROXY(_random, 0);
