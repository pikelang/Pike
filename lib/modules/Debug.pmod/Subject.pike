
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
//! _sprintf(79, ([ ]))
//! (2) Result: Debug.Subject
//! > abs(class { inherit Debug.Subject; int `<(mixed ... args) { return 1; } }());
//! create()
//! `-()
//! _destruct()
//! (3) Result: 0
//! > pow(s,2);
//! `[]("pow")
//! Attempt to call the NULL-value
//! Unknown program: 0(2)

#define ENTER(X)                       \
  string t = sprintf("%{%O, %}", args); \
  werror("%s%s(%s)\n", id, #X,           \
	 has_suffix(t, ", ") ? t[..<2] : t)

#define PROXY(X,Y) X(mixed ... args) { ENTER(X); return Y; }

protected string id = "";

protected void create(mixed ... args)
{
  ENTER(create);
  if(sizeof(args)==1 && stringp(args[0]))
    id = "(" + args[0] + ") ";
}

protected void PROXY(_destruct, 0);

protected mixed PROXY(`->, 0);
protected mixed PROXY(`->=, 0);
protected mixed PROXY(`[], 0);
protected mixed PROXY(`[]=, 0);

protected mixed PROXY(`+, 0);
protected mixed PROXY(`-, 0);
protected mixed PROXY(`&, 0);
protected mixed PROXY(`|, 0);
protected mixed PROXY(`^, 0);
protected mixed PROXY(`<<, 0);
protected mixed PROXY(`>>, 0);
protected mixed PROXY(`*, 0);
protected mixed PROXY(`/, 0);
protected mixed PROXY(`%, 0);
protected mixed PROXY(`~, 0);

protected int(0..1) PROXY(`==, 0);
protected int(0..1) PROXY(`<, 0);
protected int(0..1) PROXY(`>, 0);
protected int PROXY(`!, 0);

protected int PROXY(__hash, 0);
protected int PROXY(_sizeof, 0);
protected mixed PROXY(cast, 0);
protected mixed PROXY(`(), 0);

protected mixed PROXY(``+, 0);
protected mixed PROXY(``-, 0);
protected mixed PROXY(``&, 0);
protected mixed PROXY(``|, 0);
protected mixed PROXY(``^, 0);
protected mixed PROXY(``<<, 0);
protected mixed PROXY(``>>, 0);
protected mixed PROXY(``*, 0);
protected mixed PROXY(``/, 0);
protected mixed PROXY(``%, 0);

protected mixed PROXY(`+=, 0);

protected int(0..1) PROXY(_is_type, 0);
protected int PROXY(_equal, 0);
protected mixed PROXY(_m_delete, 0);

//! @ignore
protected array PROXY(_indices, ::_indices());
protected array PROXY(_values, ::_values());
//! @endignore

protected object _get_iterator(mixed ... args)
{
  ENTER(_get_iterator);
  string iid = id==""?"":id[1..<2];
  return this_program("("+iid+" iterator) ");
}

protected string _sprintf(int|void t, mapping|void opt, mixed ... x)
{
  string args = "";
  if(t)
    args += sprintf("'%c'", t);
  if(opt)
  {
    string tmp = sprintf("%O", opt);
    string a,b; int c;
    if( sscanf(tmp, "%s /* 1 element */%s", a, b)==2 ||
        sscanf(tmp, "%s /* %d elements */%s", a, c, b)==3 )
      tmp = a+b;
    args += sprintf(", %s", String.normalize_space(tmp));
  }
  string tmp = sprintf("%{%O, %}", x);
  if(has_suffix(tmp, ", "))
    tmp = tmp[..<2];
  if(sizeof(tmp))
    args += ", " + tmp;
  werror("%s_sprintf(%s)\n", id, args);
  return "Debug.Subject" + id[..<1];
}

protected mixed PROXY(_random, 0);
