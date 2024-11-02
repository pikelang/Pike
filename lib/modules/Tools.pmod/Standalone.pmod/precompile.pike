#! /usr/bin/env pike

#if __REAL_VERSION__ < 7.8
#error Pike binary is too old to be used for precompile.
#error At least 7.8 is required.
#endif

#pike 9.0

#define TOSTR(X) #X
#define DEFINETOSTR(X) TOSTR(X)

#if constant(Pike.__HAVE_CPP_PREFIX_SUPPORT__)
constant precompile_api_version = "6";
#else
constant precompile_api_version = "3";
#endif

#if !constant(zero)
/* Compat with Pike 8.0 and earlier. */
typedef int(0..0) zero;
#endif

constant want_args = 1;

//! Convert .cmod-files to .c files.

constant description = "Converts .cmod-files to .c files";

mapping map_types = ([]), need_obj_defines = ([]);

string usage = #"[options] <from> > <to>


 This script is used to process *.cmod files into *.c files, it
 reads Pike style prototypes and converts them into C code.

 Supported options are:

   -h,--help	 Display this help text.

   -v,--version	 Display the API version.

   --api	 Set the API version.

   -b,--base	 Set the base name for the file.

   -o,--out	 Set the output filename.

   -w,--warnings Enable warnings.

 The input can look something like this:

 DECLARATIONS

 PIKECLASS fnord
  attributes;
 {
   INHERIT bar
     attributes;

   INHERIT \"__builtin.foo\"
     attributes;

   IMPLEMENTS bar;

   IMPLEMENTS \"__builtin.foo\";

   CVAR int foo;
   PIKEVAR mapping m
     attributes;

   DECLARE_STORAGE; // optional

   PIKEFUN int function_name (int x, CTYPE char * foo)
    attribute;
    attribute value;
   {
     C code using 'x' and 'foo'.

     RETURN x;
   }

   INIT
   {
     // Object initialization code.
   }

   EXIT
     gc_trivial;
   {
     // Object cleanup code.
   }

   GC_RECURSE
   {
     // Code to run under the gc recurse pass.
   }

   GC_CHECK
   {
     // Code to run under the gc check pass.
   }

   EXTRA
   {
     // Code for adding extra constants etc.
   }

   OPTIMIZE
   {
     // Code for optimizing calls that clone the class.
     // The node for the call is available in the variable n.
   }
 }

 All the begin_class/ADD_EFUN/ADD_FUNCTION calls will be inserted
 instead of the word INIT in your code.

 The corresponding cleanup code will be inserted instead of the word EXIT.

 Argument count checks are added both for too many (unless the last
 arg is varargs) and too args. The too many args check can be
 disabled by putting \", ...\" last in the argument list.

 Magic types in function argument lists:

   bignum    A native int or bignum object. The C storage type is
             struct svalue.

   longest   A native int or bignum object which is range checked and stored
             in a LONGEST.

   zero      If used like \"zero|Something\" then a zero value is allowed in
             addition to Something (which is how it works by default in pike
             code). This will not cause an svalue to be constructed on the C
             level, rather 0/NULL is assigned to the C type.

             Note that this is almost the same as how void works in
             \"void|Something\", except that the argument still is required.

             \"zero\" interacts with \"void\" in a special way: If you write
             \"void|int\" then the C storage will be an svalue to make it
             possible to tell a missing argument from a zero integer. If you
             write \"void|zero|int\" then it's assumed that you don't care
             about that, and so the C storage will be an INT_TYPE which will
             be assigned zero both if the argument is zero and when it's left
             out. This applies to the int and float runtime types, but not
             for the special case \"void|zero\".

 Note: If a function argument accepts more than one runtime type, e.g.
 \"int|string\", then no type checking at all is performed on the svalue. That
 is not a bug - the user code has to do a type check later anyway, and it's
 more efficient to add the error handling there.

 Currently, the following attributes are understood:
   efun;          makes this function a global constant (no value)
   export;        Declare the corresponding C-symbol PMOD_EXPORT.
   flags;         ID_STATIC | ID_NOMASK etc.
   optflags;      OPT_TRY_OPTIMIZE | OPT_SIDE_EFFECT etc.
   optfunc;       Optimization function.
   type;          Override the pike type in the prototype with this type.
                  FIXME: this doesn't quite work
   rawtype;       Override the pike type in the prototype with this C-code
                  type, e.g. tInt, tMix etc.
   errname;       The name used when throwing errors.
   name;          The name used when doing add_function.
   c_name;        The name to use for a PIKEVAR in a C struct.
   prototype;     Ignore the function body, just add a prototype entry.
   program_flags; PROGRAM_USES_PARENT | PROGRAM_DESTRUCT_IMMEDIATE etc.
   program_id;    Set the program_id for the PIKECLASS. Note that this
                  is only needed for program id constants that do not
                  match the automatically generated PROG_*_ID pattern.
                  Note that the symbol specified here will be prefixed
                  and suffixed by PROG_/_ID when used as it is used as
                  argument to START_NEW_PROGRAM_ID().
   num_generics;  The number of generics for this PIKECLASS (if any).
   bind_generics; The types to bind the generic types in the INHERITed
                  program to.
   gc_trivial;    Only valid for EXIT functions. This instructs the gc that
                  the EXIT function is trivial and that it's ok to destruct
                  objects of this program in any order. In general, if there
                  is an EXIT function, the gc has to take the same care as if
                  there is a _destruct function. But if the EXIT function
                  doesn't mind that arbitrary referenced pike objects gets
                  destructed before this one (a very common trait since most
                  EXIT functions only do simple cleanup), then this attribute
                  can be added to make the gc work a little easier.

 POLYMORPHIC FUNCTION OVERLOADING
   You can define the same function several times with different
   types. This program will select the proper function whenever
   possible.

 AUTOMATIC ALLOCATION AND DEALLOCATION OF CONSTANT STRINGS
   You can use the syntax MK_STRING(\"my string\") to refer to
   a struct pike_string with the content \"my string\", or the
   syntax MK_STRING_SVALUE(\"my string\") for the same, but a
   full svalue. Note that this syntax can not be used in macros.

 AUTOMATIC SYMBOLS
   args                      The number of arguments a PIKEFUN has received.
   f_symbol                  The C-level implementation of a PIKEFUN.
   f_symbol_fun_num          The reference number in a PIKECLASS for a PIKEFUN.
                             Not applicable to efuns.
   symbol_fun_num            The reference number to a nested PIKECLASS in
                             the surrounding PIKECLASS.
   symbol_inh_num            The inherit number for an INHERIT.
   symbol_inh_offset         The offset to the start of references for
                             an INHERIT in the current PIKECLASS.
   symbol_inh_storage_offset The storage offset for an INHERIT in
                             the storage for the current PIKECLASS.

 BUGS/LIMITATIONS
  o Parenthesis must match, even within #if 0
  o Not all Pike types are supported yet.
  o RETURN; (void) doesn't work yet
  o need a RETURN_NULL; or something.. RETURN 0; might work but may
    be confusing as RETURN x; will not work if x is zero.
  o Comments does not work inside prototypes (?)

 C ENVIRONMENT
  o The macro PRECOMPILE_API_VERSION is provided to be able
    to detect the version of precompile at compile time.
";

#define PC Parser.Pike
#define split PC.split

int api;	// Requested API level.

int warnings;

void warn(string s, mixed ... args)
{
  if (warnings) {
    if (sizeof(args)) s = sprintf(s, @args);
    werror(s);
    return;
  }
  error(s, @args);
}

array global_declarations=({});
multiset check_used = (<>);

/* Strings declared with MK_STRING. */
mapping(string:string) strings = ([
  // From stralloc.h:
  "":"empty_pike_string",

  // From program.h:
  "this_program":"this_program_string",
  "this":"this_string",
  // lfuns:
  "__INIT":"lfun_strings[LFUN___INIT]",
  "create":"lfun_strings[LFUN_CREATE]",
  "_destruct":"lfun_strings[LFUN__DESTRUCT]",
  "`+":"lfun_strings[LFUN_ADD]",
  "`-":"lfun_strings[LFUN_SUBTRACT]",
  "`&":"lfun_strings[LFUN_AND]",
  "`|":"lfun_strings[LFUN_OR]",
  "`^":"lfun_strings[LFUN_XOR]",
  "`<<":"lfun_strings[LFUN_LSH]",
  "`>>":"lfun_strings[LFUN_RSH]",
  "`*":"lfun_strings[LFUN_MULTIPLY]",
  "`/":"lfun_strings[LFUN_DIVIDE]",
  "`%":"lfun_strings[LFUN_MOD]",
  "`~":"lfun_strings[LFUN_COMPL]",
  "`==":"lfun_strings[LFUN_EQ]",
  "`<":"lfun_strings[LFUN_LT]",
  "`>":"lfun_strings[FLUN_GT]",
  "__hash":"lfun_strings[LFUN___HASH]",
  "cast":"lfun_strings[LFUN_CAST]",
  "`!":"lfun_strings[LFUN_NOT]",
  "`[]":"lfun_strings[LFUN_INDEX]",
  "`[]=":"lfun_strings[LFUN_ASSIGN_INDEX]",
  "`->":"lfun_strings[LFUN_ARROW]",
  "`->=":"lfun_strings[LFUN_ASSIGN_ARROW]",
  "_sizeof":"lfun_strings[LFUN__SIZEOF]",
  "_indices":"lfun_strings[LFUN__INDICES]",
  "_values":"lfun_strings[LFUN__VALUES]",
  "`()":"lfun_strings[LFUN_CALL]",
  "``+":"lfun_strings[LFUN_RADD]",
  "``-":"lfun_strings[LFUN_RSUBTRACT]",
  "``&":"lfun_strings[LFUN_RAND]",
  "``|":"lfun_strings[LFUN_ROR]",
  "``^":"lfun_strings[LFUN_RXOR]",
  "``<<":"lfun_strings[LFUN_RLSH]",
  "``>>":"lfun_strings[LFUN_RRSH]",
  "``*":"lfun_strings[LFUN_RMULTIPLY]",
  "``/":"lfun_strings[LFUN_RDIVIDE]",
  "``%":"lfun_strings[LFUN_RMOD]",
  "`+=":"lfun_strings[LFUN_ADD_EQ]",
  "_is_type":"lfun_strings[LFUN__IS_TYPE]",
  "_sprintf":"lfun_strings[LFUN__SPRINTF]",
  "_equal":"lfun_strings[LFUN__EQUAL]",
  "_m_delete":"lfun_strings[LFUN__M_DELETE]",
  "_get_iterator":"lfun_strings[LFUN__GET_ITERATOR]",
  "`[..]":"lfun_strings[LFUN_RANGE]",
  "_search":"lfun_strings[LFUN__SEARCH]",
  "_types":"lfun_strings[LFUN__TYPES]",
  "_serialize":"lfun_strings[LFUN__SERIALIZE]",
  "_deserialize":"lfun_strings[LFUN__DESERIALIZE]",
  "_size_object":"lfun_strings[LFUN__SIZE_OBJECT]",
  "_random":"lfun_strings[LFUN__RANDOM]",
  "`**":"lfun_strings[LFUN_POW]",
  "``**":"lfun_strings[LFUN_RPOW]",
  "_sqrt":"lfun_strings[LFUN__SQRT]",
]);
int last_str_id = 0;
array(string) stradd = ({});
int last_svalue_id = 0;
mapping(string:string) svalues = ([]);

string parse_string(string str)
{
  if (search(str, "\\") == -1) {
    return str[1..sizeof(str)-2];
  }
  error("String escapes not supported yet.\n");
}

string allocate_string(string orig_str)
{
  string str, str_sym;

  if (sizeof(orig_str) >= 2 && `==('"', orig_str[0], orig_str[-1]))
    str = parse_string(orig_str);
  else
    str = orig_str;
  str_sym = strings[str];
  if (str_sym) return str_sym;

  if (String.width(str)>8) {
    error("Automatic allocation of wide strings with MK_STRING() or MK_STRING_SVALUE() not supported yet.\n");
  }
  int str_id = last_str_id++;
  stradd += ({
    sprintf("module_strings[%d] = \n"
	    "  make_shared_binary_string(%s,\n"
	    "                            CONSTANT_STRLEN(%s));\n",
	    str_id,
	    orig_str, orig_str),
  });
  str_sym = strings[str] = sprintf("module_strings[%d] /* %s */",
				   str_id, orig_str);
  return str_sym;
}

string allocate_string_svalue(string orig_str)
{
  string str_sym = allocate_string(orig_str);
  string svalue_sym = svalues[str_sym];
  if (svalue_sym) return svalue_sym;
  int svalue_id = last_svalue_id++;
  stradd += ({
    sprintf("\n#ifdef module_svalues_declared\n"
	    "SET_SVAL(module_svalues[%d], PIKE_T_STRING, 0, string, %s);\n"
	    "add_ref(module_svalues[%d].u.string);\n"
	    "#endif\n",
	    svalue_id, str_sym, svalue_id),
  });
  svalue_sym = svalues[str_sym] = sprintf("(module_svalues+%d)", svalue_id);
  return svalue_sym;
}

/*
 * This function takes an array of tokens containing a type
 * written in the 'array(int)' format. It returns position of
 * the first token that is not a part of the type.
 */
int parse_type(array t, int p)
{
  while(1)
  {
    while(1)
    {
      if(arrayp(t[p]))
      {
	p++;
      }else{
	switch( (string) t[p++])
	{
	  case "CTYPE":
	    p=sizeof(t)-2;
	    break;
	  case "0": case "1": case "2": case "3": case "4":
	  case "5": case "6": case "7": case "8": case "9":
	    if("=" != t[p]) break;
	  case "!":
	    continue;

	  case "__deprecated__":
	  case "object":
	  case "program":
	  case "function":
	  case "mapping":
	  case "array":
	  case "multiset":
	  case "int":
	  case "type":
	  case "string":
	    if(arrayp(t[p])) p++;
	  case "utf8_string":
	  case "sprintf_format":
	  case "sprintf_args":
	  case "bignum":
	  case "longest":
	  case "float":
	  case "zero":
	    break;
	}
      }
      break;
    }
    switch( arrayp(t[p]) ? "" : (string) t[p])
    {
      case "|":
      case "&":
	p++;
	continue;
    }
    break;
  }
  return p;
}

/*
 * This function takes an array of tokens and
 * reconstructs the string they came from. It does
 * not insert linenumbers.
 */
string merge(array x)
{
  return PC.simple_reconstitute(x);
}

/*
 * make a C identifier representation of 'n'
 */
string cquote(string|object(PC.Token) n)
{
  string ret="";
  while(sscanf((string)n,"%[a-zA-Z0-9]%c%s",string safe, int c, n)==3)
  {
    switch(c)
    {
      default: ret+=sprintf("%s_%02X",safe,c); break;
      case '+':	ret+=sprintf("%s_add",safe); break;
      case '`': ret+=sprintf("%s_backtick",safe);  break;
      case '_': ret+=sprintf("%s_",safe); break;
      case '=': ret+=sprintf("%s_eq",safe); break;
    }
  }
  ret+=n;
  if(ret[0]=='_' || (ret[0]>='0' && ret[0]<='9'))
    ret="cq_"+ret;
  return ret;
}


/*
 * This function maps C types to the appropriate
 * pike type. It is used by the CTYPE syntax.
 *
 * @param tokens
 *   An array of C tokens representing the C type.
 *
 * @returns
 *   Returns the corresponding Pike type.
 */
PikeType convert_ctype(array(PC.Token) tokens)
{
  int signed = 1;

  while (1) {
    switch((string)tokens[0]) {
    case "unsigned":
      signed = 0;
      tokens = tokens[1..];
      if (!sizeof(tokens))
	return PikeType("int(0..)");
      break;

    case "signed":
      signed = 1;
      tokens = tokens[1..];
      if (!sizeof(tokens))
	return PikeType("int");
      break;

    case "char": /* char* */
      if(sizeof(tokens) >1 && "*" == (string)tokens[1])
	return PikeType("string(8bit)");
      else if (signed)
	return PikeType("int(-128..127)");
      else
	return PikeType("int(8bit)");

    case "short":
      if (signed)
	return PikeType("int(-32768..32767)");
      else
	return PikeType("int(16bit)");

    case "int":
    case "long":
    case "INT32":
      if (signed) return PikeType("int");
      return PikeType("int(0..)");

    case "size_t":
      return PikeType("int(0..)");

    case "ptrdiff_t":
      return PikeType("int");

    case "double":
    case "float":
      return PikeType("float");

    default:
      error("Unknown C type.\n");
    }
  }
}

string stringify(string s)
{
  return sprintf("\"%{\\%o%}\"",(array)s);
//  return sprintf("%O",s);
}

class PikeType
{
  PC.Token t;
  array(PikeType|string|int) args=({});

  protected string fiddle(string name, array(string) tmp)
    {
      while(sizeof(tmp) > 7)
      {
	int p=sizeof(tmp)-7;
	string z=name + "7(" + tmp[p..]*"," +")";
	tmp=tmp[..p];
	tmp[-1]=z;
      }
      switch(sizeof(tmp))
      {
	default:
	  return sprintf("%s%d(%s)",name,sizeof(tmp),tmp*",");
	case 2: return sprintf("%s(%s,%s)",name,tmp[0],tmp[1]);
	case 1: return tmp[0];
	case 0: return "";
      }
    }

  protected array(PikeType)|zero strip_zero_alt()
  /* Assumes this is an '|' node. Returns args without any 'zero'
   * alternatives. */
  {
    if (!args) return 0;
    array(PikeType) a = ({});
    foreach (args, PikeType arg)
      if (arg->basetype() != "zero") a += ({arg});
    if (!sizeof (a) && sizeof (args))
      // Make sure we don't strip away the entire type.
      a = ({args[0]});
    return a;
  }

  protected void replace_names(array tok, mapping(string:string) names)
  {
    int scoped = 0;
    foreach(tok; int i; object(PC.Token)|array t)
      if (arrayp(t)) {
	replace_names(t, names);
	scoped = 0;
      } else if (((string)t) == "::" || ((string)t) == ".")
	scoped = 1;
      else if (scoped)
	scoped = 0;
      else if (names[(string)t])
	t->text = names[(string)t];
  }

  /*
   * return the 'one-word' description of this type
   */
  string basetype()
    {
      string ret=(string)t;
      switch(ret)
      {
	case "CTYPE":
	  return args[1]->basetype();

	case "|": {
	  // If there's only one type except "void" and "zero" then return it,
	  // else return "mixed". Plus some special cases if we only got "void"
	  // and "zero" in there.
	  int got_zero;
	  string single_type;
	  foreach (args, PikeType arg) {
	    string type = arg->basetype();
	    if (type == "zero")
	      got_zero = 1;
	    else if (type != "void") {
	      if (single_type && single_type != type)
		return "mixed";
	      else
		single_type = type;
	    }
	  }
	  if (single_type) return single_type;
	  else if (got_zero) return "zero";
	  else return "void";
	}

        case "__deprecated__":
	case "=":
	case "&":
	  return args[-1]->basetype();

	case "!":
	case "any":
	case "0":
	case "1":
	case "2":
	case "3":
	case "4":
	case "5":
	case "6":
	case "7":
	case "8":
	case "9":
	case "bignum":
        case "sprintf_args":
	  return "mixed";

	  // FIXME: Guess the special "longest" type ought to be
	  // "CTYPE LONGEST" instead, but the CTYPE stuff seems to be
	  // broken and I don't want to dig into it.. /mast
	case "longest": return "LONGEST";

      case "int":
      case "float":
      case "string":
      case "object":
      case "function":
      case "array":
      case "mapping":
      case "multiset":
      case "type":
      case "program":
      case "mixed":
      case "void":
      case "zero":
	return ret;

      case "unknown":
      case "__unknown__":
	return "__unknown__";

      case "utf8_string":
      case "sprintf_format":
	return "string";

	default: return "object";
      }
    }

  string realtype()
  /* Similar to basetype, but returns the source description if the
   * base type. */
  {
    string ts = (string) t;
    switch (ts) {
      case "bignum":
      case "longest":
	return ts;
      default:
	return basetype();
    }
  }

  /* Return an array of all possible basetypes for this type
   */
  array(string) basetypes()
    {
      string ret=(string)t;
      switch(ret)
      {
	case "CTYPE":
	  return args[1]->basetypes();

	case "|":
	  return `|(@args->basetypes());

        case "__deprecated__":
	case "=":
	case "&":
	  return args[-1]->basetypes();

	case "!":
	case "any":
	case "0":
	case "1":
	case "2":
	case "3":
	case "4":
	case "5":
	case "6":
	case "7":
	case "8":
	case "9":

        case "sprintf_args":
	case "mixed":
	  return ({
	    "int",
	    "float",
	    "string",
	    "object",
	    "program",
	    "function",
	    "array",
	    "mapping",
	    "multiset",
	    "type",
	  });
	case "program":
	  return ({
	    "object",
	    "program",
	    "function",
	  });

      case "int":
      case "float":
      case "string":
      case "object":
      case "function":
      case "array":
      case "mapping":
      case "multiset":
      case "type":
      case "void":
      case "zero":
	return ({ ret });

      case "unknown":
      case "__unknown__":
	return ({ "__unknown__" });

      case "utf8_string":
      case "sprintf_format":
	return ({ "string" });

	case "bignum":
	case "longest":
	  return ({"int", "object"});

	default:  return ({ "object" });
      }
    }


  /*
   * PIKE_T_INT, PIKE_T_STRING etc.
   */
  string type_number()
    {
      string btype=basetype();

      if (btype == "function") {
	btype = "mixed";
      }
      return "PIKE_T_"+upper_case(btype);
    }

  /*
   * Check if this type matches 'void' and/or 'zero'.
   */
  int may_be_void_or_zero (int(0..) may_be_void, int(0..) may_be_zero)
  {
    switch((string)t)
    {
      case "zero": return may_be_zero;
      case "void": return may_be_void;
      case "|":
	return max (0, @args->may_be_void_or_zero (may_be_void, may_be_zero));

      case "=":
      case "&":
	return args[-1]->may_be_void_or_zero (may_be_void, may_be_zero);
    }
    return 0;
  }

  int may_be_void()		// Compat wrapper.
  {
    return may_be_void_or_zero (1, 0);
  }

  string c_storage_type(int|void is_struct_entry)
    {
      string btype = realtype();
      switch (btype)
      {
        case "unknown": case "__unknown__":
	case "void": return "void";
	case "zero":
	  return may_be_void()?"struct svalue *":"INT_TYPE";
	case "int":
	  return (may_be_void_or_zero (1, 2) == 1 ?
		  "struct svalue *" : "INT_TYPE");
	case "float":
	  return (may_be_void_or_zero (1, 2) == 1 ?
		  "struct svalue *" : "FLOAT_TYPE");
        case "utf8_string": case "sprintf_format":
	case "string": return "struct pike_string *";

	case "array":
	case "multiset":
	case "mapping":

	case "object":
	case "program": return "struct "+btype+" *";

        case "type": return "struct pike_type *";

	case "function":
	case "mixed":
	case "bignum":
        case "sprintf_args":
	  if (is_struct_entry) {
	    return "struct svalue";
	  } else {
	    return "struct svalue *";
	  }

	case "longest":
	  return "LONGEST";

	default:
	  error("Unknown type %s\n", btype);
      }
    }

  /*
   * Make a C representation, like 'tInt'. Can also strip off an
   * "|zero" alternative on the top level.
   */
  string output_c_type (void|int ignore_zero_alt)
    {
      string ret=(string)t;
//      werror("FOO: %O %O\n",t,args);
      switch(ret)
      {
	case "CTYPE":
	  return (string)args[0]->t;

	case "&":
	  return fiddle("tAnd",args->output_c_type());

	case "|":
	  array(PikeType) a = args; // strip_zero_alt();
	  if (sizeof (a) == 1) return a[0]->output_c_type();
	  return fiddle("tOr",a->output_c_type());

	case "!":
	  return sprintf("tNot(%s)",args[0]->output_c_type());

	case "mapping":
	  ret=sprintf("tMap(%s)",args->output_c_type()*",");
	  if(ret=="tMap(tMix,tMix)") return "tMapping";
	  return ret;

	case "multiset":
	  ret=sprintf("tSet(%s)",args[0]->output_c_type());
	  if(ret=="tSet(tMix)") return "tMultiset";
	  return ret;

	case "array":
	  ret=sprintf("tArr(%s)",args[0]->output_c_type());
	  if(ret=="tArr(tMix)") return "tArray";
	  return ret;

        case "__deprecated__":
	  return sprintf("tDeprecated(%s)", args[0]->output_c_type());

        case "type":
	  return sprintf("tType(%s)", args[0]->output_c_type());

	case "function":
	  string t=args[..sizeof(args)-3]->output_c_type()*" ";
	  if(t == "") t="tNone";
	  if(args[-2]->output_pike_type(0) == "void")
	  {
	    return sprintf("tFunc(%s,%s)",
			   t,
			   args[-1]->output_c_type());
	  }else{
	    return sprintf("tFuncV(%s,%s,%s)",
			   t,
			   args[-2]->output_c_type(),
			   args[-1]->output_c_type());
	  }

	case "=":
	  return sprintf("tSetvar(%s,%s)",
			 (string)args[0]->t,
			 args[1]->output_c_type (ignore_zero_alt));

	case "0":
	case "1":
	case "2":
	case "3":
	case "4":
	case "5":
	case "6":
	case "7":
	case "8":
	case "9":       return sprintf("tVar(%s)",ret);

	case "zero":    return "tZero";
	case "void":    return "tVoid";
	case "float":   return "tFloat";
	case "string":
	  {
            // Clamp the integer range to 32 bit signed.
            int low = limit(-0x80000000, (int)(string)(args[0]->t), 0x7fffffff);
            int high = limit(-0x80000000, (int)(string)(args[1]->t), 0x7fffffff);
            if (high < low)
              exit(1, "Error: Inversed range in integer type, string(%s..%s).\n",
                   (string)args[0]->t, (string)args[1]->t);

	    if (!low) {
	      switch(high) {
	      case 0x0000: return "tStr0";
	      case 0x007f: return "tStr7";
	      case 0x00ff: return "tStr8";
	      case 0xffff: return "tStr16";
	      }
	    } else if ((low == -0x80000000) && (high == 0x7fffffff)) {
	      return "tStr";
	    }
	    // NOTE! This piece of code KNOWS that the serialized
	    //       value of PIKE_T_INT is 8!
	    return sprintf("tNStr(%s)",
			   stringify(sprintf("\010%4c%4c", low, high)));
	  }
	case "utf8_string": return "tUtf8Str";
        case "sprintf_format": return "tAttr(\"sprintf_format\", tStr)";
        case "sprintf_args": return "tAttr(\"sprintf_args\", tMix)";
	case "program": return "tPrg(tObj)";
	case "any":     return "tAny";
	case "unknown": return "tUnknown";
	case "__unknown__": return "tUnknown";
	case "mixed":   return "tMix";
	case "int":
	  // Clamp the integer range to 32 bit signed.
	  int low = limit(-0x80000000, (int)(string)(args[0]->t), 0x7fffffff);
	  int high = limit(-0x80000000, (int)(string)(args[1]->t), 0x7fffffff);
	  if (high < low)
            exit(1, "Error: Inversed range in integer type, int(%s..%s).\n",
                 (string)args[0]->t, (string)args[1]->t);

	  // NOTE! This piece of code KNOWS that PIKE_T_INT is 8!
	  return stringify(sprintf("\010%4c%4c", low, high));

	case "bignum":
	case "longest":
	  return "tInt";

	case "object":
          return "tObj";
	default:
          string define = "tObjImpl_"+replace(upper_case(ret), ".", "_");
          if( !need_obj_defines[define] )
            need_obj_defines[define] = ret;
	  return sprintf("tName(%O, tObjImpl_%s)",
			 ret, replace(upper_case(ret), ".", "_"));
      }
    }


  /*
   * Output pike type, such as array(int)
   */
  string output_pike_type(int pri)
    {
      string ret=(string)t;
      switch(ret)
      {
	case "CTYPE":
	  return args[1]->output_pike_type(pri);

	case "&":
	case "|":
	  ret=args->output_pike_type(1)*ret;
	  if(pri) ret="("+ret+")";
	  return ret;

	case "!":
	  return "!"+args[0]->output_pike_type(1);

	case "mapping":
	  ret=sprintf("mapping(%s:%s)",
		      args[0]->output_pike_type(0),
		      args[1]->output_pike_type(0));
	  if(ret=="mapping(mixed:mixed")
	    return "mapping";
	  return ret;

	case "multiset":
	case "array":
	{
	  string tmp=args[0]->output_pike_type(0);
	  if(tmp=="mixed") return ret;
	  return sprintf("%s(%s)",ret,tmp);
	}

	case "__deprecated__":
	  return sprintf("%s(%s)", ret, args[0]->output_pike_type(0));

	case "function":
	  array(string) tmp=args->output_pike_type(0);
	  ret=sprintf("function(%s:%s)",
		      tmp[..sizeof(tmp)-3]*","+
		      (tmp[-2]=="void"?"":tmp[-2] + "..."),
		      tmp[-1]);
	  if(ret=="function(mixed...:any)")
	    return "function";
	  return ret;

	case "=":
	  return sprintf("%s=%s",
			 args[0]->output_pike_type(0),
			 args[1]->output_pike_type(0));

	case "int":
        case "string":
	  // Clamp the integer range to 32 bit signed.
	  int low = limit(-0x80000000, (int)(string)(args[0]->t), 0x7fffffff);
	  int high = limit(-0x80000000, (int)(string)(args[1]->t), 0x7fffffff);
	  if (!low && !(high & (high + 1))) {
	    if (!high) {
	      if (ret == "int") return "zero";
	      return sprintf("%s(zero)", ret);
            } else if (ret == "int" && high == 0x7fffffff) {
              return "int(0..)";
	    }

	    int i;
	    for (i = 0; high; high >>= 1, i++)
	      ;
	    return sprintf("%s(%dbit)", ret, i);
	  }
	  ret=sprintf("%s(%s..%s)",
		      ret,
		      args[0]->t == "-2147483648" ? "" : args[0]->t,
		      args[1]->t ==  "2147483647" ? "" : args[1]->t);
	  if(has_suffix(ret, "(..)")) return ret[..sizeof(ret)-5];
	  return ret;

        case "utf8_string":
	  return "utf8_string";

        case "sprintf_format":
	  return "string";

        case "sprintf_args":
	  return "mixed";

	case "bignum":
	case "longest":
	  return "int";

	default:
	  return ret;
      }
    }


  /* Make a copy of this type */
  PikeType copy()
    {
      return PikeType(t, args->copy());
    }

  /*
   * Copy and remove type assignments. Can also strip off an "|zero"
   * alternative on the top level.
   */
  PikeType copy_and_strip_type_assignments (void|int ignore_zero_alt)
    {
      if("=" == (string)t)
	return args[1]->copy_and_strip_type_assignments (ignore_zero_alt);
      array(PikeType) a;
      if (args && ignore_zero_alt && (string) t == "|")
	a = strip_zero_alt();
      else
	a = args;
      return PikeType(t, a && a->copy_and_strip_type_assignments (0));
    }

  protected string _sprintf(int how)
    {
      switch(how)
      {
	case 'O':
	  catch {
	    return sprintf("PikeType(%O)",output_pike_type(0));
	  };
	  return sprintf("PikeType(%O)",t);

	case 's':
	  return output_pike_type(0);
        case 't':
	  return "object";
      }
    }

  /*
   * Possible ways to initialize a PikeType:
   * PikeType("array(array(int))")
   * PikeType("CTYPE char *")
   * PikeType( ({ PC.Token("array"), ({ PC.Token("("),PC.Token("int"),PC.Token(")"), }) }) )
   *
   * And this way is for internal use only:
   * PikeType( PC.Token("array"), ({ PikeType("int") }) )
   */
  protected void create(string|array(PC.Token)|PC.Token|array(array) tok,
			void|array(PikeType)|mapping(string:string) a)
    {
      switch(sprintf("%t",tok))
      {
	case "object":
	  t=tok;
	  args=a;
	  break;

	case "string":
	  tok=convert_comments(PC.tokenize(split(tok),"piketype"));
	  tok=PC.group(PC.hide_whitespaces(tok));

	  // FALLTHRU
	case "array":
	  /* strip parenthesis */
	  while(sizeof(tok) == 1 && arrayp(tok[0]))
	    tok=tok[0][1..sizeof(tok[0])-2];

	  if (a)
	    replace_names(tok, a);

	  array(array(PC.Token|array(PC.Token|array))) tmp;
	  tmp = ([array]tok)/({"|"});

	  if(sizeof(tmp) >1)
	  {
	    t=PC.Token("|");
	    args=map(tmp,PikeType);
	    return;
	  }

	  tmp = ([array]tok)/({"&"});
	  if(sizeof(tmp) >1)
	  {
	    t=PC.Token("&");
	    args=map(tmp,PikeType);
	    return;
	  }

	  tmp = ([array]tok)/({"="});
	  if(sizeof(tmp) >1)
	  {
	    t=PC.Token("=");
	    args=map(tmp,PikeType);
	    return;
	  }

	  t=tok[0];

	  if(sizeof(tok) == 1)
	  {
	    switch((string)t)
	    {
	      case "CTYPE":
		args=({ PikeType(PC.Token(merge(tok[1..]))),
			  convert_ctype(tok[1..]) });
		break;

	      case "mapping":
		args=({PikeType("mixed"), PikeType("mixed")});
		break;
	      case "multiset":
	      case "array":
		args=({PikeType("mixed")});
		break;
	      case "int":
	      case "string":
		string low = (string)(int)-0x80000000;
		string high = (string)0x7fffffff;
		args=({PikeType(PC.Token(low)),PikeType(PC.Token(high))});
		break;

	      case "function":
		args=({ PikeType("unknown"), PikeType("any") });
		break;

	      case "type":
		args=({ PikeType("mixed") });
		break;
	    }
	  }else{
	    array q;
	    switch((string)t)
	    {
	      case "!":
		args=({ PikeType(tok[1..]) });
		break;

	      case "__deprecated__":
	      case "array":
	      case "multiset":
		args=({ PikeType(tok[1..]) });
		break;

	      case "mapping":
		if(arrayp(q=tok[1]))
		{
		  tmp=q[1..sizeof(q)-2]/({":"});
		  args=map(tmp,PikeType);
		}else{
		  args=({PikeType("mixed"),PikeType("mixed")});
		}
		break;

	      case "function":
		if(arrayp(q=tok[1]))
		{
		  tmp=q[1..sizeof(q)-2]/({":"});
		  if(sizeof(tmp)<2)
		  {
		    throw( ({
		      sprintf("%s:%d: Missing return type in function type.\n",
			      t->file||"-",
			      t->line),
			backtrace()
			}) );
		  }
		  array rettmp=tmp[1];
		  array argstmp=tmp[0]/({","});

		  int end=sizeof(argstmp)-1;
		  PikeType repeater;
		  if(sizeof(argstmp) &&
		     sizeof(argstmp[-1]) &&
		     argstmp[-1][-1]=="...")
		  {
		    repeater=PikeType(argstmp[-1][..sizeof(argstmp[-1])-2]);
		    end--;
		  }else{
		    repeater=PikeType("void");
		  }
		  args=map(argstmp[..end]-({({})}),PikeType)+
		    ({repeater, PikeType(rettmp) });
		}else{
		  args=({PikeType("unknown"),PikeType("any")});
		}
		return;

	      case "int":
	      case "string":
		string low = (string)(int)-0x80000000;
		string high = (string)0x7fffffff;

		if(arrayp(q=tok[1]))
		{
		  tmp=q[1..sizeof(q)-2]/({".."});
		  if(sizeof(tmp)==1)
                  {
		    int bits;
		    /* Support the string(Xbit) syntax too. */
		    if (sizeof(q) == 3 && sscanf((string)q[1], "%dbit", bits))
                    {
		      low = "0";
		      high = sprintf("%d", (1 << bits) - 1);
		    }
		    else if (sizeof(q) == 4 && q[2] == "bit")
                    {
		      // Parser.Pike from Pike 8.1.3 or earlier.
		      low = "0";
		      high = sprintf("%d", (1 << (int)(string)q[1]) - 1);
		    }
                    else
                    {
		      werror("%s:%d: Syntax error: %s is not a valid type.\n",
                             t->file, t->line, merge(tok));
                      exit(1);
                    }
		  } else {
		    if(sizeof(tmp[0])) low=(array(string))tmp[0]*"";
		    if(sizeof(tmp[1])) high=(array(string))tmp[1]*"";
		  }
		}
		args=({PikeType(PC.Token(low)),PikeType(PC.Token(high))});
		break;

	      case "object":
	      case "type":
		if (arrayp(tok[1]) && sizeof(tok[1]) > 2) {
		  if ((sizeof(tok[1]) > 3) && (api < 3)) {
		    // Make sure that the user specifies
		    // the correct minimum API level
		    // for build-script compatibility.
		    warn("%s:%d: API level 3 (or higher) is required "
			 "for type %s.\n",
			 t->file, t->line,
			 PC.simple_reconstitute(({ t, tok[1] })));
		  }
		  t = PC.Token(merge(tok[1][1..sizeof(tok[1])-2]));
		}
		break;
	    }
	  }
      }
    }
};

class CType {
  inherit PikeType;
  string ctype;

  string output_c_type() { return ctype; }
  protected void create(string _ctype) {
    ctype = _ctype;
  }
}

// Used to find the declared upper and lower range for strings
// in argument types.
array(int) clamp_int_range(PikeType complex_type, string ranged_type)
{
  switch((string)complex_type->t) {
  case "|":
    {
      array(int)|zero res = UNDEFINED;
      foreach(complex_type->args, PikeType arg) {
	array(int)|zero sub_res = clamp_int_range(arg, ranged_type);
	if (!sub_res) continue;
	if (!res) {
	  res = sub_res;
	  continue;
	}
	if (res[0] > sub_res[0]) res[0] = sub_res[0];
	if (res[1] < sub_res[1]) res[1] = sub_res[1];
      }
      return res;
    }
  case "&":
    {
      array(int) res = ({ -0x80000000, 0x7fffffff });
      foreach(complex_type->args, PikeType arg) {
	array(int)|zero sub_res = clamp_int_range(arg, ranged_type);
	if (!sub_res) return UNDEFINED;
	if (res[0] < sub_res[0]) res[0] = sub_res[0];
	if (res[1] > sub_res[1]) res[1] = sub_res[1];
      }
      if (res[0] > res[1]) return UNDEFINED;
      return res;
    }

  case "sprintf_format":
    return ({ -0x80000000, 0x7fffffff });

  case "utf8_string":
    if (ranged_type == "string") {
      return ({ 0, 255 });
    }
    // FALL_THROUGH
  default:
    if (((string)complex_type->t) == ranged_type) {
      array(PikeType) args = complex_type->args;
      if (sizeof(args) == 2) {
	return ({ limit(-0x80000000,
			(int)(string)(args[0]->t),
			0x7fffffff),
		  limit(-0x80000000,
			(int)(string)(args[1]->t),
			0x7fffffff),
	});
      }
      return ({ -0x80000000, 0x7fffffff });
    }
    return UNDEFINED;
  }
}

/*
 * This class is used to represent one function argument
 */
class Argument
{
  /* internal */
  string _name;		// Name of argument.
  PikeType _type;	// Declared Pike type.
  int _line;		// Line number in CMOD.
  string _file;		// Filename of CMOD.

  int _is_c_type;

  // The following are cached values.
  string _c_type;	// _type->c_storage_type().
  string _basetype;	// _type->basetype().
  string _typename;	// Pretty-printed Pike type (used in error messages).
  string _realtype;	// _type->realtype().

  int is_c_type() { return _is_c_type; }
  int may_be_void_or_zero (int may_be_void, int may_be_zero)
    { return type()->may_be_void_or_zero (may_be_void, may_be_zero); }
  int may_be_void() { return type()->may_be_void(); }
  int line() { return _line; }
  string name() { return _name; }
  PikeType type() { return _type; }

  string basetype()
    {
      if(_basetype) return _basetype;
      return _basetype = type()->basetype();
    }

  string realtype()
  {
    if (_realtype) return _realtype;
    return _realtype = type()->realtype();
  }

  string c_type()
    {
      if(_c_type) return _c_type;
      return _c_type = type()->c_storage_type();
    }

  string typename()
    {
      if(_typename) return _typename;
      return _typename=
	type()->copy_and_strip_type_assignments (1)->output_pike_type(0);
    }

  protected void create(array x, void|mapping(string:string) names)
    {
      PC.Token t;
      if( arrayp(x[-1]) )
      {
        // Nameless argument prototype
        t = x[0];
      }
      else
      {
        _name=(string)x[-1];
        t = x[-1];
      }
      _file=t->file;
      _line=t->line;
      _type=PikeType(x[..sizeof(x)-2], names);
    }

  protected string _sprintf(int how)
    {
      return type()->output_pike_type(0)+" "+name();
    }
};

/*
 * This function takes a bunch of strings an makes
 * a unique C identifier with underscores in between.
 */
string mkname(string ... parts)
{
  return map(parts - ({"",0}), cquote) * "_";
}

mapping(string:int) names = ([]);

/*
 * Variant of mkname that always returns a unique name.
 */
string make_unique_name(string ... parts)
{
  string id = mkname(@parts);
  if (names[id]) {
    int i = 2;
    while (names[id + "_" + i]) i++;
    id += "_" + i;
  }
  names[id] = 1;
  return id;
}


/*
 * Create C code for popping 'howmany' arguments from
 * the stack. 'howmany' may be a constant or an expression.
 */
string make_pop(mixed howmany)
{
  switch(howmany)
  {
    default:
      return "pop_n_elems("+howmany+");";

    case "0": case 0: return "";
    case "1": case 1: return "pop_stack();";
  }
}

/* Fixme:
 * This routine inserts non-tokenized strings into the data, which
 * can confuse a later stage, we might need to do something about that.
 * However, I need a *simple* way of doing it first...
 * -Hubbe
 */
array fix_return(array body, PikeType rettype, mixed args)
{
  int pos=0;

  while( (pos=search(body,PC.Token("RETURN",0),pos)) != -1)
  {
    int pos2=search(body,PC.Token(";",0),pos+1);
    body[pos]=sprintf("do { %s ret_=(",rettype->c_storage_type());
    body[pos2]=sprintf("); %s push_%s(ret_); return; }while(0);",
		       make_pop(args),
		       rettype->basetype());
    pos=pos2+1;
  }

  pos=0;
  while( (pos=search(body,PC.Token("REF_RETURN",0),pos)) != -1)
  {
    int pos2=search(body,PC.Token(";",0),pos+1);
    body[pos]=sprintf("do { %s ret_=(",rettype->c_storage_type());
    body[pos2]=sprintf("); add_ref(ret_); %s push_%s(ret_); return; }while(0);",
		       make_pop(args),
		       rettype->basetype());
    pos=pos2+1;
  }
  return body;
}


array recursive(mixed func, array data, mixed ... args)
{
  array ret=({});

  foreach(data, mixed foo)
    {
      if(arrayp(foo))
      {
	ret+=({ recursive(func, foo, @args) });
      }else{
	ret+=({ foo });
      }
    }

  return func(ret, @args);
}

// FIXME: We could expand this to declare which attributes are valid
// where.
constant valid_attributes = (<
  "efun",
  "export",
  "flags",
  "optflags",
  "optfunc",
  "type",
  "rawtype",
  "errname",
  "name",
  "c_name",		/* Override of generated C-symbol */
  "prototype",
  "program_flags",
  "program_id",
  "num_generics",	/* PIKECLASS only. */
  "bind_generics",	/* INHERIT only. */
>);

/*
 * This function takes a list of tokens, containing attributes on the form:
 *   attributename foo bar gazonk ;
 *   attributename2 foo2 bar2 gazonk2 ;
 *
 * Returns a mapping like:
 * ([
 *   "attributename":"foo bar gazonk",
 *   "attributename2":"foo2 bar2 gazonk2",
 *   "attrflagname":1,
 * ])
 */
mapping parse_attributes(array attr, void|string location,
			 void|multiset really_valid_attributes)
{
  mapping attributes=([]);
  foreach(attr/ ({";"}), attr)
    {
      while(sizeof(attr) && has_prefix((string)attr[0], "/*"))
	attr = attr[1..];
      switch(sizeof(attr))
      {
	case 0: continue;
	case 1:
	  if(arrayp(attr[0]))
	  {
	    werror("%s:%d: Expected attribute name!\n",
		   attr[0][0]->file,
		   attr[0][0]->line);
	    if(location)
	      werror("%s: This is where the attributes belong\n", location);
	    error("This is what I got: %O\n", attr[0]);
	  }
	  attributes[(string)attr[0]]=1;
	  break;
	default:
	  array tmp=attr[1..];
	  if(sizeof(tmp) == 1 && arrayp(tmp[0]) && tmp[0][0]=="(")
	    tmp=tmp[0][1..sizeof(tmp[0])-2];

	  attributes[(string)attr[0]]=merge(tmp);
      }

      if(!(really_valid_attributes || valid_attributes)[(string)attr[0]]) {
	error("%s:%d: Invalid attribute name %O.\n",
	      attr[0]->file, attr[0]->line, (string)attr[0]);
      }
    }

  if(attributes->optfunc && !attributes->efun) {
    warn("Only efuns may have an optfunc.\n");
  }

  return attributes;
}

/*
 * Generate an #ifdef/#else/#endif
 */
array IFDEF(string|array(string) define,
	    array|zero yes,
	    void|array no)
{
  array ret=({});
  if(!arrayp(define)) define=({define});
  if(!yes || !sizeof(yes))
  {
    if(!no || !sizeof(no)) return ({"\n"});
    if(sizeof(define)==1)
    {
      ret+=({ sprintf("\n#ifndef %s\n",define[0]) });
    }else{
      ret+=({ sprintf("\n#if !defined(%s)\n",define*") && !defined(") });
    }
  }else{
    if(sizeof(define)==1)
    {
      ret+=({ sprintf("\n#ifdef %s\n",define[0]) });
    }else{
      ret+=({ sprintf("\n#if defined(%s)\n",define*") || defined(") });
    }
    ret+=yes;
    if(no && sizeof(no))
    {
      ret+=({sprintf("\n#else /* %s */\n",define*", ")});
    }
  }
  ret+=no || ({});
  ret+=({sprintf("\n#endif /* %s */\n",define*", ")});
  return ret;
}

/*
 * Generate a #define
 */
array DEFINE(string define, void|string as)
{
  string tmp=define;
  sscanf(tmp,"%s(",tmp);

  return ({
    sprintf("\n#undef %s\n",tmp),
    sprintf("#define %s%s\n",define, as?" "+as:"")
      });
}


class FuncData
{
  string name;
  string define;
  PikeType type;
  mapping attributes;
  int max_args;
  int min_args;
  array(Argument) args;

  protected string _sprintf(int c)
    {
      return sprintf("FuncData(%s)",define);
    }

  protected int `==(mixed q)
    {
      return objectp(q) && q->name == name;
//      return 0;
    }
};

// Returns the number of non-empty equvivalence classes in q.
int evaluate_method(mixed q)
{
  int val=0;
  q=values(q) - ({ ({}) });
#ifdef PRECOMPILE_OVERLOAD_DEBUG
  werror("evaluate: %O\n",q);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
  for(int e=0;e<sizeof(q);e++)
  {
    if(q[e] && sizeof(q[e]))
    {
      val++;
      for(int d=e+1;d<sizeof(q);d++)
      {
	if(q[d] && !sizeof(q[e] ^ q[d])) /* equal is broken in some Pikes */
	{
#ifdef PRECOMPILE_OVERLOAD_DEBUG
	  werror("EQ, %d %d\n",e,d);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
	  q[d]=0;
	}
      }
    }
  }
  return val;
}

// Polymorphic overloader.
array generate_overload_func_for(array(FuncData) d,
				 int indent,
				 int min_possible_arg,
				 int max_possible_arg,
				 string name,
				 mapping attributes)
{
  if(sizeof(d)==1)
  {
    return IFDEF(d[0]->define, ({
      PC.Token(sprintf("%*n%s(args);\n",indent,mkname("f",d[0]->name))),
	PC.Token(sprintf("%*nreturn;\n",indent)),
	}))+
      ({ PC.Token(sprintf("%*nbreak;\n",indent)) });
  }

#ifdef PRECOMPILE_OVERLOAD_DEBUG
  werror("generate_overload_func_for(%O, %d, %d, %d, %O, %O)...\n",
	 d, indent, min_possible_arg, max_possible_arg, name, attributes);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */

  array out=({});

  /* This part should be recursive */
  array(array(FuncData)) x=allocate(256,({}));
  int min_args=0x7fffffff;
  int max_args=0;
  foreach(d, FuncData q)
    {
#ifdef PRECOMPILE_OVERLOAD_DEBUG
      werror("LOOP: q:%O, min_args:%O, max_args:%O\n",
	     q, q->min_args, q->max_args);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
      int low = max(min_possible_arg, q->min_args);
      int high = min(max_possible_arg, q->max_args);
      for(int a = low; a <= min(high, 255); a++)
	x[a]+=({q});
      min_args=min(min_args, low);
      max_args=max(max_args, high);
    }

  min_args=max(min_args, min_possible_arg);
  max_args=min(max_args, max_possible_arg);

#ifdef PRECOMPILE_OVERLOAD_DEBUG
  werror("MIN: %O\n",min_args);
  werror("MAX: %O\n",max_args);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */

  string argbase="-args";

  int best_method = -1;
  int best_method_value=evaluate_method(x);
#ifdef PRECOMPILE_OVERLOAD_DEBUG
  werror("Value X: %d\n",best_method_value);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */

  array(mapping(string:array(FuncData))) y;
  if(min_args >= 0)
  {
    y=allocate(min(min_args,16), ([]));
    for(int a=0;a<sizeof(y);a++)
    {
#ifdef PRECOMPILE_OVERLOAD_DEBUG
      werror("BT: arg: %d\n", a);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
      foreach(d, FuncData q)
	{
#ifdef PRECOMPILE_OVERLOAD_DEBUG
	  werror("BT: q: %O %s\n",q, q->args[a]->type()->basetypes()*"|");
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
	  foreach(q->args[a]->type()->basetypes(), string t)
	  {
	    if (t != "void") {
	      if(!y[a][t]) y[a][t]=({});
	      y[a][t]+=({q});
	    }
	  }
	}
    }

    for(int a=0;a<sizeof(y);a++)
    {
      int v=evaluate_method(y[a]);
#ifdef PRECOMPILE_OVERLOAD_DEBUG
      werror("Value %d: %d\n",a,v);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
      if(v>best_method_value)
      {
	best_method=a;
	best_method_value=v;
      }
    }
  }

#ifdef PRECOMPILE_OVERLOAD_DEBUG
  werror("Best method=%d\n",best_method);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */

  if(best_method == -1)
  {
    if (min_args == max_args) {
      error("Can't differentiate between %d implementations of %s().\n",
	    sizeof(d), name);
    }

    /* Switch on number of arguments */
    out+=({PC.Token(sprintf("%*nswitch(args) {\n",indent))});
    for(int a=min_args;a<=max_args;a++)
    {
      array tmp=x[a];
      if(tmp && sizeof(tmp))
      {
	int d;
	for(d=a;d<sizeof(x);d++)
	{
	  if(equal(tmp, x[d]) && !sizeof(tmp ^ x[d]))
	  {
	    out+=({sprintf("%*n case %d:\n",indent,d) });
	    x[d]=0;
	  }else{
	    break;
	  }
	}
#ifdef PRECOMPILE_OVERLOAD_DEBUG
	werror("Generating code for %d..%d arguments.\n", a, d-1);
#endif /* PRECOMPILE_OVERLOAD_DEBUG */
	out+=generate_overload_func_for(tmp,
					indent+2,
					a,
					d-1,
					name,
					attributes);
      }
    }
    out+=({
      PC.Token(sprintf("%*n default:\n",indent)),
	PC.Token(sprintf("%*n  wrong_number_of_args_error(%O,args,%d);\n",
			 indent,
			 name,
			 min_args)),
	PC.Token(sprintf("%*n}\n",indent)),
	});
  }else{
    /* Switch on an argument */
    /* First check that we have at least that many arguments (if needed) */

    if(min_possible_arg < best_method+1)
    {
      out+=({
	PC.Token(sprintf("%*nif(args < %d) wrong_number_of_args_error(%O,args,%d);\n",
			 indent,
			 best_method+1,
			 name,
			 min_args))
	  });
      min_possible_arg=best_method;
    }

    if(min_possible_arg == max_possible_arg)
      argbase=(string) (-min_possible_arg);

    out+=({PC.Token(sprintf("%*nswitch(TYPEOF(Pike_sp[%d%s])) {\n",
			    indent,
			    best_method,argbase)) });

    mapping m=y[best_method];
    mapping m2=m+([]);
    foreach(sort(indices(m)), string type)
      {
	array tmp=m[type];
	if(tmp && sizeof(tmp))
	{
	  foreach(sort(indices(m)), string t2)
	    {
	      if(equal(tmp, m[t2]) && !sizeof(tmp ^ m[t2]))
	      {
		m_delete(m,t2);
		out+=({PC.Token(sprintf("%*n case PIKE_T_%s:\n",
					indent,
					upper_case(t2)))});
	      }
	    }
	  out+=generate_overload_func_for(tmp,
					  indent+2,
					  min_possible_arg,
					  max_possible_arg,
					  name,
					  attributes);
	}
      }
    out+=({
      PC.Token(sprintf("%*n default:\n",indent)),
        PC.Token(sprintf("%*n  SIMPLE_ARG_TYPE_ERROR(%O,%d,%O);\n",
			 indent,
			 attributes->errname || name,
			 best_method+1,
			 sort(indices(m2))*"|")),
	PC.Token(sprintf("%*n}\n",indent)),
	});
  }
  return out;
}

int gid;

// Parses a block of cmod code, separating it into declarations,
// functions to add, exit functions and other code.
class ParseBlock
{
  array code=({});
  array addfuncs=({});
  array exitfuncs=({});
  array declarations=({});
  int local_id = ++gid;

  protected void create(array(array|PC.Token) x, string base, string class_name,
			mapping(string:string) names)
    {
      array(array|PC.Token) ret=({});
      array thestruct=({});

      // FIXME: Consider generating code in the order it appears in
      //        the source file.
      //	/grubba 2004-12-10

      string gc_live_obj_define = make_unique_name (base, "gc", "live", "obj");

      int e;
      for(e = 0; e < sizeof(x); e++) {
	array|PC.Token t;
	if (arrayp(t = x[e])) {
	  ret += ({ t });
	  continue;
	}
	switch((string)t) {
	default:
	  ret += ({ t });
	  break;
	case "extern":
	  ret += ({ t });

	  if ((e+2 < sizeof(x)) && (((string)x[e+1])[0] == '\"') &&
	      arrayp(x[e+2])) {
	    // C++ syntax support...
	    create(x[e+2], base, class_name, names);
	    ret += ({ x[e+1], code });
	    code = ({});
	    e += 2;
	  }
	  break;
	case "INHERIT":
	  {
	    int pos=search(x,PC.Token(";",0),e);

	    string p;
	    string numid = "-1";
	    string offset = "0";
	    string indent = "  ";
	    array pre = ({});
	    array post = ({});

	    mixed name=x[e+1];
	    string define=make_unique_name("inherit",name,base,"defined");
	    string inh_num = make_unique_name(base, name, "inh_num");
	    string inh_offset = make_unique_name(base, name, "inh_offset");
	    string inh_storage = make_unique_name(base, name, "inh_storage_offset");
            string inh_name;
	    if ((string)name == "::") {
	      e++;
	      name = x[e+1];
	      if ((name == "this_program") &&
		  has_suffix(base, "_" + class_name)) {
		define=make_unique_name("inherit",name,base,"defined");
		inh_num = make_unique_name(base, name, "inh_num");
		inh_offset = make_unique_name(base, name, "inh_offset");
		inh_storage = make_unique_name(base, name, "inh_storage_offset");
		name = UNDEFINED;
                inh_name = sprintf("%q", class_name);
		pre = ({
		  PC.Token(
sprintf("  do {\n"
	"    int i__ =\n"
	"      reference_inherited_identifier(Pike_compiler->previous, NULL,\n"
	"                                     %s);\n"
	"    if (i__ != -1) {\n"
	"      struct program *p = Pike_compiler->previous->new_program;\n"
	"      struct identifier *id__ = ID_FROM_INT(p, i__);\n"
	"      if (IDENTIFIER_IS_CONSTANT(id__->identifier_flags) &&\n"
	"          (id__->func.const_info.offset != -1)) {\n"
	"        struct svalue *s = &PROG_FROM_INT(p, i__)->\n"
	"          constants[id__->func.const_info.offset].sval;\n"
	"        if (TYPEOF(*s) == T_PROGRAM) {\n"
	"          p = s->u.program;\n",
	allocate_string(sprintf("%q", class_name))),
			   x[e]->line),
		});
		indent = "          ";
		p = "p";
		numid = "i__";
		offset = "1 + 42";
		post = ({
		  PC.Token(
sprintf("        } else {\n"
	"          yyerror(\"Previous definition of %s is not a program.\");\n"
	"        }\n"
	"      } else {\n"
	"        yyerror(\"Previous definition of %s is not constant.\");\n"
	"      }\n"
	"    } else {\n"
	"      yyerror(\"Failed to find previous definition of %s.\");\n"
	"    }\n"
	"  } while(0);\n",
	class_name, class_name, class_name),
			   x[e]->line),
		});
	      } else {
		error("Unsupported INHERIT syntax.\n");
	      }
	    }

            if (x[e+2] == ":") {
              inh_name = sprintf("%q", (string)x[e+3]);
              e += 2;
            }

	    mapping attributes = parse_attributes(x[e+2..pos]);
	    if (((string)name)[0] == '\"') {
	      pre = ({
		PC.Token("  do {\n"),
		PC.Token("    struct program *p = resolve_program(" +
			 allocate_string((string)name) +
			 ");\n",
			 name->line),
		PC.Token("    if (p) {\n"),
	      });
	      indent = "      ";
	      p = "p";
	      post = ({
		PC.Token("      free_program(p);\n"
			 "    } else {\n", name->line),
		PC.Token("      yyerror(\"Inherit failed.\");\n"
			 "    }\n"
			 "  } while(0);", x[e]->line),
	      });
	      if (api < 5) {
		warn("%s:%d: API level 5 (or higher) is required "
		     "for inherit of strings.\n",
		     name->file, name->line);
	      }
	    } else if (name) {
	      p = mkname(names[(string)name]||(string)name, "program");
              inh_name = inh_name || sprintf("%q", (string)name);
	    }
	    check_used[inh_offset] = 1;
	    check_used[inh_storage] = 1;

            if (attributes->bind_generics) {
              array(string) binding_types =
                map(attributes->bind_generics/",", String.trim_all_whites) -
                ({ "" });
              foreach(binding_types, string rawtype) {
                pre += ({
                  PC.Token(indent + "push_type_value(make_pike_type(" + rawtype + "));\n"),
                });
              }
              pre += ({
                PC.Token(indent + sprintf("f_aggregate(%d);\n",
                                          sizeof(binding_types))),
              });

              post = ({
                PC.Token(indent + "pop_stack();\n"),
              });
            }

	    addfuncs +=
	      IFDEF(define,
		    ({
		      PC.Token(
			       sprintf("  %s = Pike_compiler->new_program->"
				       "num_inherits;\n",
				       inh_num),
			       x[e]->line),
		    }) + pre + ({
                      IFDEF("module_strings_declared",
                            ({
                              PC.Token(sprintf("%slow_inherit(%s, NULL, %s, "
                                               "%s, %s, %s\n"
                                               "#ifdef tGeneric\n"
                                               ", %s\n"
                                               "#endif\n"
                                               ");\n",
                                               indent, p, numid, offset,
                                               attributes->flags || "0",
                                               inh_name?allocate_string(inh_name):"NULL",
                                               attributes->bind_generics?
                                               "Pike_sp[-1].u.array":"NULL"),
                                       x[e]->line),
                            }),
                            ({
                              PC.Token(sprintf("%slow_inherit(%s, NULL, %s, "
                                               "%s, %s, NULL\n"
                                               "#ifdef tGeneric\n"
                                               ", %s\n"
                                               "#endif\n"
                                               ");\n",
                                               indent, p, numid, offset,
                                               attributes->flags || "0",
                                               attributes->bind_generics?
                                               "Pike_sp[-1].u.array":"NULL"),
                                       x[e]->line),
                            })),
		      IFDEF(inh_offset + "_used",
			    ({
			      PC.Token(sprintf("%s%s = Pike_compiler->new_program->"
					       "inherits[%s].identifier_level;\n",
					       indent, inh_offset, inh_num),
				       x[e]->line),
			    })),
		      IFDEF(inh_storage + "_used",
			    ({
			      PC.Token(sprintf("%s%s = Pike_compiler->new_program->"
					       "inherits[%s].storage_offset;\n",
					       indent, inh_storage, inh_num),
				       x[e]->line),
			    })),
		    }) + post);
	    ret += DEFINE(define) + ({
	      PC.Token(sprintf("static int %s = -1;\n", inh_num),
		       x[e]->line),
	      IFDEF(inh_offset + "_used",
		    ({
		      PC.Token(sprintf("static int %s = -1;\n", inh_offset),
			       x[e]->line),
		    })),
	      IFDEF(inh_storage + "_used",
		    ({
		      PC.Token(sprintf("static int %s = -1;\n", inh_storage),
			       x[e]->line),
		    })),
	    });
	    e = pos;
	    break;
	  }
	case "IMPLEMENTS":
	  {
	    int pos=search(x,PC.Token(";",0),e);

	    string p;
	    string numid = "-1";
	    string offset = "0";
	    string indent = "  ";
	    array pre = ({});
	    array post = ({});

	    mixed name=x[e+1];
	    string define = make_unique_name("implements",name,base,"defined");
	    if ((string)name == "::") {
#if 0
	      e++;
	      name = x[e+1];
	      if ((name == "this_program") &&
		  has_suffix(base, "_" + class_name)) {
		define = make_unique_name("implements",name,base,"defined");
		name = UNDEFINED;
		// FIXME: The following is broken!
		pre = ({
		  PC.Token(
sprintf("  do {\n"
	"    int i__ =\n"
	"      reference_inherited_identifier(Pike_compiler->previous, NULL,\n"
	"                                     %s);\n"
	"    if (i__ != -1) {\n"
	"      struct program *p = Pike_compiler->previous->new_program;\n"
	"      struct identifier *id__ = ID_FROM_INT(p, i__);\n"
	"      if (IDENTIFIER_IS_CONSTANT(id__->identifier_flags) &&\n"
	"          (id__->func.const_info.offset != -1)) {\n"
	"        struct svalue *s = &PROG_FROM_INT(p, i__)->\n"
	"          constants[id__->func.const_info.offset].sval;\n"
	"        if (TYPEOF(*s) == T_PROGRAM) {\n"
	"          p = s->u.program;\n",
	allocate_string(sprintf("%q", class_name))),
			   x[e]->line),
		});
		indent = "          ";
		p = "p";
		numid = "i__";
		offset = "1 + 42";
		post = ({
		  PC.Token(
sprintf("        } else {\n"
	"          yyerror(\"Previous definition of %s is not a program.\");\n"
	"        }\n"
	"      } else {\n"
	"        yyerror(\"Previous definition of %s is not constant.\");\n"
	"      }\n"
	"    } else {\n"
	"      yyerror(\"Failed to find previous definition of %s.\");\n"
	"    }\n"
	"  } while(0);\n",
	class_name, class_name, class_name),
			   x[e]->line),
		});
	      } else {
		error("Unsupported IMPLEMENTS syntax.\n");
	      }
#else
	      error("Unsupported IMPLEMENTS syntax.\n");
#endif
	    }

	    mapping attributes = parse_attributes(x[e+2..pos]);
	    if (((string)name)[0] == '\"') {
	      pre = ({
		PC.Token("  do {\n"),
		PC.Token("    struct program *p = resolve_program(" +
			 allocate_string((string)name) +
			 ");\n",
			 name->line),
		PC.Token("    if (p) {\n"),
	      });
	      indent = "      ";
	      p = "p";
	      post = ({
		PC.Token("      free_program(p);\n"
			 "    } else {\n", name->line),
		PC.Token("      yyerror(\"Implements failed.\");\n"
			 "    }\n"
			 "  } while(0);", x[e]->line),
	      });
	      if (api < 5) {
		warn("%s:%d: API level 5 (or higher) is required "
		     "for implements of strings.\n",
		     name->file, name->line);
	      }
	    } else if (name) {
	      p = mkname(names[(string)name]||(string)name, "program");
	    }
	    addfuncs +=
	      IFDEF(define,
		    pre + ({
		      PC.Token(sprintf("%sref_push_program(%s);\n",
				       indent, p),
			       x[e]->line),
		      PC.Token(sprintf("%spush_object(clone_object("
				       "Implements_program, 1));\n",
				       indent),
			       x[e]->line),
		      PC.Token(sprintf("%sadd_program_annotation(0, Pike_sp-1);\n",
				       indent),
			       x[e]->line),
		      PC.Token(sprintf("%spop_stack();\n", indent), x[e]->line),
		    }) + post);
	    ret += DEFINE(define);
	    e = pos;
	    break;
	  }
	case "EXTRA":
	  {
	    string define = make_unique_name("extra",base,"defined");
	    addfuncs += IFDEF(define, x[e+1]);
	    ret += DEFINE(define);
	    e++;
	    break;
	  }
	case "PIKECLASS":
	  {
	    int p;
	    for(p=e+1;p<sizeof(x);p++)
	      if(arrayp(x[p]) && x[p][0]=="{")
		break;

	    array proto = x[e+1..p-1];
	    array body = x[p];

	    string name=(string)proto[0];
	    string lname = mkname(base, name);
	    mapping attributes=parse_attributes(proto[1..]);
            int num_generics = (int)attributes->num_generics;
	    names[name] = lname;
	    ParseBlock subclass = ParseBlock(body[1..sizeof(body)-2],
					     mkname(base, name), name,
					     copy_value(names));
	    string program_var = mkname(base, name, "program");

	    string define = make_unique_name("class", base, name, "defined");

	    ret+=DEFINE(define);
	    global_declarations +=
	      ({sprintf("DEFAULT_CMOD_STORAGE struct program *%s=NULL;\n",
			program_var)});
	    global_declarations +=
	      IFDEF(program_var+"_fun_num_used",
		    ({sprintf("DEFAULT_CMOD_STORAGE int %s_fun_num=-1;",
			      program_var)}));
	    ret+=subclass->declarations;
	    ret+=subclass->code;
	    // Restore THIS to the parent class.
	    ret+=
	      IFDEF("THIS_" + upper_case(base),
		    DEFINE("THIS", "THIS_" + upper_case(base)));

            need_obj_defines["tObjImpl_"+upper_case(lname)] = 1;
            map_types[subclass->local_id] = ({ define, "return "+program_var+"->id;" });

            check_used[program_var+"_fun_num"]=1;
	    addfuncs+=
	      IFDEF(define,
		    ({
		      IFDEF("PROG_"+upper_case(lname)+"_ID",
			    ({
			      PC.Token(sprintf("  START_NEW_PROGRAM_ID(%s);\n",
					       upper_case(lname)),
				       proto[0]->line),
			      "#else\n",
			      attributes->program_id?
			      PC.Token(sprintf("  START_NEW_PROGRAM_ID(%s);\n",
					       attributes->program_id),
				       proto[0]->line):
			      PC.Token("  start_new_program();\n",
				       proto[0]->line),
			    })),
		      PC.Token(sprintf("  %s = Pike_compiler->new_program;\n",
				       program_var),
			       proto[0]->line),
		      IFDEF("tObjImpl_"+upper_case(lname),
			    0,
			    DEFINE("tObjIs_"+upper_case(lname),
                                   sprintf("%O", sprintf("\3\1\x7f%3c",
                                                         subclass->local_id)))+
			    DEFINE("tObjImpl_"+upper_case(lname),
                                   sprintf("%O", sprintf("\3\0\x7f%3c",
                                                         subclass->local_id)))),
		    })+
                    (num_generics ? ({
                      PC.Token(sprintf("  %s->num_generics = %d;\n",
                                       program_var, num_generics),
                               proto[0]->line),
                      PC.Token(sprintf("  Pike_compiler->num_generics = %d;\n",
                                       num_generics),
                               proto[0]->line),
                    }):({})) +
		    subclass->addfuncs+
		    ({
		      attributes->program_flags?
		      PC.Token(sprintf("  Pike_compiler->new_program->flags |= %s;\n",
				     attributes->program_flags),
			       proto[1]->line):"",
		      PC.Token(sprintf("  %s=end_program();\n",program_var),
			       proto[0]->line),
		      PC.Token(sprintf("#ifdef %[0]s_fun_num_used\n%s_fun_num=\n#endif\n"
                                       +" add_program_constant(%O,%s,%s);\n",
				       program_var,
				       name,
				       program_var,
				       attributes->flags || "0"),
			       proto[0]->line),
		    })
		    );
	    exitfuncs+=
	      IFDEF(define,
		    subclass->exitfuncs+
		    ({
		      sprintf("  if(%s) {\n", program_var),
		      PC.Token(sprintf("    free_program(%s);\n", program_var),
			       proto[0]->line),
		      sprintf("    %s=0;\n"
			      "  }\n",
			      program_var),
		    }));
	    e = p;
	    break;
	  }

	case "PIKEVAR":
	  {
	    int pos = search(x, PC.Token(";",0), e);
	    int pos2 = parse_type(x, e+1);
	    mixed name = x[pos2];
	    PikeType type = PikeType(x[e+1..pos2-1], names);
	    string define = make_unique_name("var",name,base,"defined");
        while( x[pos2+1] == "." )
        {
            name = (string)name + (string)x[pos2+1]+ (string)x[pos2+2];
            pos2+=2;
        }
	    mapping attributes = parse_attributes(x[pos2+1..pos]);
//    werror("type: %O\n",type);
	    mixed csym = attributes->c_name || name;
        if( !has_value( csym, "." ) )
        {
            thestruct+=
                IFDEF(define,
                      ({ PC.Token(sprintf("  %s %s;\n",
					  type->c_storage_type(1), csym),
				  x[pos2]->line),
		      }));
        }
        else
        {
            if( name[0] == "." )
                name = name[1..];
        }

	    addfuncs+=
	      IFDEF(define,
		    ({
		      PC.Token(sprintf("  PIKE_MAP_VARIABLE(%O, %s_storage_offset + OFFSETOF(%s_struct, %s),\n"
				       "                    %s, %s, %s);",
				       ((string)name/".")[-1], base, base, csym,
				       type->output_c_type(), type->type_number(),
				       attributes->flags || "0"),
			       x[pos2]->line),
		    }));
	    ret+=DEFINE(define);
	    ret+=({ PC.Token("DECLARE_STORAGE") });
	    e = pos;
	    break;
	  }

	case "CVAR":
	  {
	    int pos = search(x,PC.Token(";",0),e);
	    int npos=pos-1;
	    while(arrayp(x[npos])) npos--;
	    mixed name=(string)x[npos];

	    string define=make_unique_name("var",name,base,"defined");

	    thestruct+=IFDEF(define,x[e+1..pos-1]+({";"}));
	    ret+=DEFINE(define);
	    ret+=({ PC.Token("DECLARE_STORAGE") });
	    e = pos;
	    break;
	  }

	case "EXIT": {
	  int p;
	  for(p=e+1;p<sizeof(x);p++)
	    if(arrayp(x[p]) && x[p][0]=="{")
	      break;

	  mapping attributes =
	    parse_attributes (x[e+1..p-1], 0, (<"gc_trivial">));

	  if (!attributes->gc_trivial)
	    ret += DEFINE (gc_live_obj_define);
	}
	  // Fall through.

	case "INIT":
	case "GC_RECURSE":
	case "GC_CHECK":
	case "OPTIMIZE":
	  {
	    ret += ({
	      PC.Token("PIKE_INTERNAL"),
	      PC.Token(lower_case((string)t)),
	    });
	    break;
	  }
	}
      }

      x=ret/({"PIKE_INTERNAL"});
      ret=x[0];

      string ev_handler_define=make_unique_name(base,"event","handler","defined");
      string opt_callback_define=make_unique_name(base,"optimize","callback","defined");
      int opt_callback;
      array ev_handler=({});
      for(int f=1;f<sizeof(x);f++)
      {
	array func=x[f];
	int p;
	for(p=0;p<sizeof(func);p++)
	  if(arrayp(func[p]) && func[p][0]=="{")
	    break;

	array body=func[p];
	array rest=func[p+1..];
//	werror("%O\n",func);
	string name=(string)func[0];
	string funcname=mkname(name,base,"struct");
	string define=make_unique_name("internal",name,base,"defined");
	ret+=DEFINE(define);
	if (name == "optimize") {
	  opt_callback = 1;
	  ret += DEFINE(opt_callback_define);
	  ret += ({ PC.Token(sprintf("static node *%s(node *n)\n",
				     funcname)) });
	} else {
	  ret+=DEFINE(ev_handler_define);
	  ret+=({ PC.Token(sprintf("static void %s(void)\n",funcname)) });
	  ev_handler+=IFDEF(define,({
	    sprintf("  case PROG_EVENT_%s: %s(); break;\n",
		    upper_case(name),
		    funcname)
	  }));
	}
	ret+=body;
	ret+=rest;
      }
      if(sizeof(ev_handler))
      {
	string funcname=mkname(base,"event","handler");
	ret+=IFDEF(ev_handler_define,
		   ({
		     sprintf("static void %s(int ev) {\n",funcname),
		       "  switch(ev) {\n"
		       })+
		   ev_handler+
		   ({ "  default: break; \n"
		      "  }\n}\n" }));

	addfuncs+=
	  IFDEF(ev_handler_define,
		({ PC.Token(sprintf("  pike_set_prog_event_callback(%s);\n",funcname)) }) +
		IFDEF (gc_live_obj_define,
		       0,
		       ({PC.Token ("  Pike_compiler->new_program->flags &= ~PROGRAM_LIVE_OBJ;\n")}))
	       );
      }
      if (opt_callback) {
	addfuncs +=
	  IFDEF(opt_callback_define,
		({ PC.Token(sprintf("  pike_set_prog_optimize_callback(%s);\n",
				    mkname("optimize",base,"struct"))) }));
      }

      if(sizeof(thestruct))
      {
	/* We insert the struct definition after the last
	 * variable definition
	 */
	string structname = base+"_struct";
	string this = sprintf("((struct %s *)(Pike_interpreter.frame_pointer->current_storage + %s_storage_offset))", structname, base);
        string program_var = mkname(base, "program");

	/* FIXME:
	 * Add runtime debug to these defines...
	 * Add defines for parents when possible
	 */
	declarations=
	  	  DEFINE("THIS",this)+   // FIXME: we should 'pop' this define later
	  DEFINE("THIS_"+upper_case(base),this)+
	  DEFINE("OBJ2_"+upper_case(base)+"(o)",
		 sprintf("((struct %s *)(o->storage+%s_storage_offset"
                         "+%s->inherits[0].storage_offset))",
			 structname, base, program_var))+
	  DEFINE("GET_"+upper_case(base)+"_STORAGE(o)",
		 sprintf("((struct %s *)(o->storage+%s_storage_offset"
                         "+%s->inherits[0].storage_offset))",
			 structname, base, program_var))+
	    ({
	      sprintf("static ptrdiff_t %s_storage_offset;\n",base),
		sprintf("struct %s {\n",structname),
		})+thestruct+({
		  "};\n",
		  sprintf (#"\
#ifdef PIKE_DEBUG
/* Ensure the struct is used in a variable declaration, or else gdb might not see it. */
static struct %s *%s_gdb_dummy_ptr;
#endif\n", structname, base),
		    })
	  +declarations;
	addfuncs = ({
	  IFDEF("THIS_"+upper_case(base), ({
		  PC.Token(sprintf("  %s_storage_offset = "
				   "ADD_STORAGE(struct %s);",
				   base, structname)),
		})),
	}) + addfuncs;
      }

      x=ret/({"DECLARE_STORAGE"});
      ret=x[..sizeof(x)-2]*({})+declarations+x[-1];
      declarations=({});

      x=ret/({"PIKEFUN"});
      ret=x[0];

      mapping(string:array(FuncData)) name_data=([]);
      mapping(string:int) name_occurances=([]);

      for(int f=1;f<sizeof(x);f++)
      {
	array func=x[f];
	int p;
	for(p=0;p<sizeof(func);p++)
	  if(arrayp(func[p]) && func[p][0]=="{")
	    break;

	array proto=func[..p-1];
	p=parse_type(proto,0);
	if(arrayp(proto[p]))
	{
	  error("%s:%d: Missing return type?\n",
		proto[p][0]->file||"-",
		proto[p][0]->line);
	}
	string name="";
	do {
	  name += (string)proto[p];
	  p++;
	} while(!arrayp(proto[p]));
	name_occurances[name]++;
      }

      for(int f=1;f<sizeof(x);f++)
      {
	array func=x[f];
	int p;
	for(p=0;p<sizeof(func);p++)
	  if(arrayp(func[p]) && func[p][0]=="{")
	    break;

	if (p >= sizeof(func)) {
	  error("%s:%d: Invalid function declaration.\n",
		func[0]->file||"-",
		func[0]->line);
	}

	array proto=func[..p-1];
	array body=func[p];
	array rest=func[p+1..];

	// Not all versions of Pike allow Token objects
	// to be indexed.
	catch {
	  foreach(Array.flatten(proto), string t)
	    if(has_prefix((string)t, "/*!"))
	      body = ({t})+body;
	};

	p=parse_type(proto,0);
	PikeType rettype=PikeType(proto[..p-1], names);

	if(arrayp(proto[p]))
	{
	  error("%s:%d: Missing type?\n",
		proto[p][0]->file||"-",
		proto[p][0]->line);
	}
	string location=proto[p]->file+":"+proto[p]->line;

	string name="";
	do {
	  name += (string)proto[p];
	  p++;
	} while(!arrayp(proto[p]));
	p--; // Readjust the pointer

	array args_tmp=proto[p+1];

	mapping attributes=parse_attributes(proto[p+2..],location);
	string common_name=name;
	if(!attributes->errname)
	  attributes->errname=name;
	if(name_occurances[common_name]>1)
	  name+="_"+(++name_occurances[common_name+".cnt"]);

	args_tmp=args_tmp[1..sizeof(args_tmp)-2];
	if(sizeof(args_tmp))
	{
	  args_tmp/=({","});
	}else{
	  args_tmp=({});
	}

	string funcname=mkname("f",base,name);
	string define=make_unique_name("f",base,name,"defined");
	string func_num=mkname("f", base,name,"fun_num");

//    werror("FIX RETURN: %O\n",body);

//    werror("name=%s\n",name);
//    werror("  rettype=%O\n",rettype);
//    werror("  args=%O\n",args);

	ret+=({
	  sprintf("#define %s\n", define),
	});

	if (!attributes->efun) {
          check_used[func_num] = 1;
	  global_declarations += ({
            IFDEF(func_num+"_used",
                  ({ sprintf("DEFAULT_CMOD_STORAGE ptrdiff_t %s = -1;\n",
                             func_num) }),
                  0)
	  });
	}

	// werror("%O %O\n",proto,args);
	int last_argument_repeats, ignore_more_args;
	if (sizeof(args_tmp)) {
	  array last_arg = args_tmp[-1];
	  if (sizeof (last_arg) > 1) {
	    if (!arrayp(last_arg[-2]) && "..." == (string)last_arg[-2]) {
	      last_argument_repeats = 1;
	      args_tmp[-1] = last_arg[..sizeof(last_arg)-3] + ({last_arg[-1]});
	    }
	  }
	  else if( sizeof(last_arg))
      {
	    if (!arrayp(last_arg[0]) && "..." == (string)last_arg[0]) {
	      ignore_more_args = 1;
	      args_tmp = args_tmp[..sizeof (args_tmp) - 2];
	    }
      }
      else
      {
          warn("%s:%s Failed to parse types\n", location,name);
      }
	}
	array(Argument) args=map(args_tmp,Argument,names);
	// werror("%O %O\n",proto,args);
	// werror("parsed args: %O\n", args);

	if((<"`<", "`>", "`==", "_equal">)[name])
	{
	  if(sizeof(args) != 1)
	  {
	    warn("%s must take one argument.\n");
	  }
	  if(sprintf("%s", [string](mixed)args[0]->type()) != "mixed")
	  {
	    warn("%s:%s must take a mixed argument (was declared as %s)\n",
		 location, name, args[0]->type());
	  }
	}

	// FIXME: support ... types
	PikeType type;

	if(attributes->rawtype)
	  type = CType(attributes->rawtype);
	else if(attributes->type)
	{
	  mixed err=catch {
	    type=PikeType(attributes->type, names);
	  };
	  if(err)
	  {
	    werror("%s: Something went wrong when parsing type: %s\n",
		   location,
		   attributes->type);
	    throw(err);
	  }
	}else{
	  if(last_argument_repeats)
	  {
	    type = PikeType(PC.Token("function"),
			    args[..sizeof(args)-2]->type() +
			    ({ args[-1]->type(), rettype }) );
	  }else{
	    type = PikeType(PC.Token("function"),
			    args->type() + ({ PikeType("void"), rettype }) );
	  }
	}

//	werror("NAME: %O\n",name);
//	werror("type: %O\n", type);
//	werror("C type: %O\n", type->output_c_type());

	if (attributes->prototype) {
	  funcname = "NULL";
	} else {
	  if (attributes->export) {
	    ret+=({
	      sprintf("PMOD_EXPORT void %s(INT32 args) ",funcname),
	    });
	  } else {
	    ret+=({
	      sprintf("DEFAULT_CMOD_STORAGE void %s(INT32 args) ",funcname),
	    });
	  }
	  ret += ({
	    "{","\n",
	  });

	  int min_args=sizeof(args);
	  int max_args=sizeof(args);
	  int repeat_arg = -1;

	  if(last_argument_repeats)
	  {
	    min_args--;
	    max_args=0x7fffffff;
	    repeat_arg = min_args;
	    args[-1]->_c_type = "struct svalue *";
	  }
	  else if (ignore_more_args)
	    max_args = 0x7fffffff;

	  while(min_args>0 && args[min_args-1]->may_be_void())
	    min_args--;

	  foreach(args, Argument arg)
	    if( arg->name() != "UNUSED" ) {
              ret+=({
                  PC.Token(sprintf("%s %s;\n",arg->c_type(), arg->name()),
                           arg->line()),
              });
	      if (arg->c_type() == "struct object *") {
		ret += ({
                  PC.Token(sprintf("int %s_inh_num;\n", arg->name()),
                           arg->line()),
		});
	      }
	    }


	  int argnum;

	  argnum=0;
	  string argbase;
	  string num_arguments;
	  if(min_args == max_args)
	  {
	    ret+=({
	      PC.Token(sprintf("if(args != %d) "
			       "wrong_number_of_args_error(%O,args,%d);\n",
			       sizeof(args),
			       attributes->errname || name,
			       sizeof(args)), proto[0]->line)
	    });
	    argbase=(string) (-sizeof(args));
	    num_arguments=(string)sizeof(args);
	  }else{
	    argbase="-args";
	    num_arguments="args";
	    if(min_args > 0) {
	      ret+=({
		PC.Token(sprintf("if(args < %d) "
				 "wrong_number_of_args_error(%O,args,%d);\n",
				 min_args,
				 name,
				 min_args), proto[0]->line)
	      });
	    } else {
	      ret+=({
                "#ifdef PIKE_DEBUG\n",
		PC.Token(sprintf("if(args < 0) Pike_fatal(%O);\n",
                                 "CMOD function argument negative.\n")),
                "#else\n",
		PC.Token("STATIC_ASSUME(args >= 0);\n"),
                "#endif\n"
	      });
            }

	    if(max_args != 0x7fffffff && max_args != -1) {
	      ret+=({
		PC.Token(sprintf("if(args > %d) "
				 "wrong_number_of_args_error(%O,args,%d);\n",
				 max_args,
				 name,
				 max_args), proto[0]->line)
	      });
	    }
	  }

	  string check_argbase = argbase;

	  foreach(args, Argument arg)
	  {
	    int got_void_or_zero_check = 0;
        if( arg->name() == "UNUSED" )
        {
            // simply skip directly for now.
            // should probably also handle ... etc?
            argnum++;
            continue;
        }
	    if (argnum == repeat_arg) {
	      // Begin the argcnt loop.
	      check_argbase = "+argcnt"+argbase;
	      ret += ({ PC.Token(sprintf("if (args > %d) {\n"
					 "  INT32 argcnt = 0;\n"
					 "  do {\n"
					 "    dmalloc_touch_svalue(Pike_sp%+d%s);\n",
					 argnum, argnum, check_argbase),
				 arg->line()) });
	    }

	    else {
	      int void_or_zero = arg->may_be_void_or_zero (2, 1);
	      if (void_or_zero == 2) {
		if (!(<"int","mixed">)[arg->basetype()]) {
		  ret+=({
		    PC.Token(sprintf("if (args > %d &&\n"
				     "    (TYPEOF(Pike_sp[%d%s]) != PIKE_T_INT ||\n"
				     "     Pike_sp[%d%s].u.integer)) {\n",
				     argnum,
				     argnum, check_argbase,
				     argnum, check_argbase), arg->line()),
		  });
		} else {
		  ret+=({
		    PC.Token(sprintf("if ((args > %d) &&\n"
				     "    ((TYPEOF(Pike_sp[%d%s]) != PIKE_T_INT) ||\n"
				     "     (SUBTYPEOF(Pike_sp[%d%s]) != NUMBER_UNDEFINED))) {\n",
				     argnum,
				     argnum, check_argbase,
				     argnum, check_argbase), arg->line()),
		  });
		}
		got_void_or_zero_check = 1;
	      }

	      else if (void_or_zero == 1) {
		if (!(<"int", "mixed">)[arg->basetype()]) {
		  ret += ({
		    PC.Token (sprintf ("if (TYPEOF(Pike_sp[%d%s]) != PIKE_T_INT ||\n"
				       "    Pike_sp[%d%s].u.integer) {\n",
				       argnum, check_argbase,
				       argnum, check_argbase), arg->line()),
		  });
		  got_void_or_zero_check = 1;
		}
	      }
	    }

	  check_arg: {
	    if(arg->is_c_type() && arg->basetype() == "string")
	    {
	      // FIXME: Probably dead code!
	      /* Special case for 'char *' */
	      /* This will have to be amended when we want to support
	       * wide strings
	       */
	      ret+=({
		PC.Token(sprintf("if(TYPEOF(Pike_sp[%d%s]) != PIKE_T_STRING ||\n"
				 "   Pike_sp[%d%s].u.string->shift_size)",
				 argnum,check_argbase,
				 argnum,check_argbase), arg->line())
	      });
	    } else {

	      switch(arg->realtype())
	      {
	      case "int":
		{
		  // Clamp the integer range to 32 bit signed.
		  [int low, int high] = clamp_int_range(arg->type(), "int");

		  ret += ({
		    PC.Token(sprintf("if((TYPEOF(Pike_sp[%d%s]) != PIKE_T_%s)",
				     argnum,check_argbase,
				     upper_case(arg->basetype())),
			     arg->line()),
		  });
		  do {
		    if (low == high) {
		      ret += ({
			PC.Token(sprintf(" || (Pike_sp[%d%s].u.integer != %d)",
					 argnum, check_argbase, high),
				 arg->line()),
			});
		      continue;
		    }
		    if (!low && !(high & (high + 1)) && (high < 0x7fffffff)) {
		      ret += ({
			PC.Token(sprintf(" || (Pike_sp[%d%s].u.integer & ~0x%x) ",
					 argnum, check_argbase, high),
				 arg->line()),
		      });
		      continue;
		    }
		    if (low > -0x80000000) {
		      ret += ({
			PC.Token(sprintf(" || (Pike_sp[%d%s].u.integer < %d)",
					 argnum, check_argbase, low),
				 arg->line()),
		      });
		    }
		    if (high < -0x7fffffff) {
		      ret += ({
			PC.Token(sprintf(" || (Pike_sp[%d%s].u.integer > %d)",
					 argnum, check_argbase, high),
				 arg->line()),
		      });
		    }
		  } while(0);
		  ret += ({
		    PC.Token(")", arg->line()),
		  });
		}
		break;
	      case "string":
		// Clamp the integer range to 32 bit signed.
		[int low, int high] = clamp_int_range(arg->type(), "string");
		if ((low >= 0) && (high <= 0xffff)) {
		  // Narrow string.
		  if (high <= 0xff) {
		    if (low > 0) {
		      ret += ({
			PC.Token(sprintf("if((TYPEOF(Pike_sp[%d%s]) != PIKE_T_%s) || "
					 "Pike_sp[%d%s].u.string->size_shift || "
					 "string_has_null(Pike_sp[%d%s].u.string))",
					 argnum, check_argbase,
					 upper_case(arg->basetype()),
					 argnum, check_argbase,
					 argnum, check_argbase),
				 arg->line()),
		      });
		    } else {
		      ret += ({
			PC.Token(sprintf("if((TYPEOF(Pike_sp[%d%s]) != PIKE_T_%s) || "
					 "Pike_sp[%d%s].u.string->size_shift)",
					 argnum, check_argbase,
					 upper_case(arg->basetype()),
					 argnum, check_argbase),
				 arg->line()),
		      });
		    }
		  } else {
		    ret += ({
		      PC.Token(sprintf("if((TYPEOF(Pike_sp[%d%s]) != PIKE_T_%s) || "
				       "(Pike_sp[%d%s].u.string->size_shift > 1))",
				       argnum, check_argbase,
				       upper_case(arg->basetype()),
				       argnum, check_argbase),
			       arg->line()),
		    });
		  }
		  break;
		}
		// FALL_THROUGH
	      default:
		ret+=({
		  PC.Token(sprintf("if(TYPEOF(Pike_sp[%d%s]) != PIKE_T_%s)",
				   argnum,check_argbase,
				   upper_case(arg->basetype())),arg->line())
		});
		break;

	      case "program":
		ret+=({
		  PC.Token(sprintf("if(!(%sprogram_from_svalue(Pike_sp%+d%s)))",
				   (arg->c_type() == "struct program *" ?
				    arg->name() + "=" : ""),
				   argnum,check_argbase),
			   arg->line())
		});
		break;

	      case "longest":
	      case "bignum":
		// Note that for "longest" we always accept a bignum
		// object, even if LONGEST isn't longer than INT_TYPE.
		// That way we get a more natural error below if the
		// bignum is too large (i.e. "Integer too large"
		// instead of "Expected int, got object").
		ret += ({
		  PC.Token (
		    sprintf ("if (TYPEOF(Pike_sp[%d%s]) != PIKE_T_INT",
			     argnum, check_argbase),
		    arg->line()),
		  PC.Token (
		    sprintf ("  && !is_bignum_object_in_svalue (Pike_sp%+d%s))",
			     argnum, check_argbase),
		    arg->line()),
		});
		break;

	      case "mixed":
		break check_arg;
	      }
	    }

	    ret+=({
	      PC.Token(sprintf(" SIMPLE_ARG_TYPE_ERROR(%O,%d%s,%O);\n",
			       attributes->errname || attributes->name || name,
			       argnum+1,
			       (argnum == repeat_arg)?"+argcnt":"",
			       arg->typename()),arg->line()),
	    });
	    }

	    if (argnum == repeat_arg) {
	      // End the argcnt loop.
	      ret += ({ PC.Token(sprintf ("  } while (++argcnt < %s-%d);\n"
					  "  %s=Pike_sp%+d%s;\n"
					  "} else %s = NULL;\n",
					  num_arguments, argnum,
					  arg->name(), argnum, argbase,
					  arg->name()),
				 arg->line()) });
	    }

	    else {
	      switch(arg->c_type())
	      {
		case "INT_TYPE":
		  ret+=({
		    PC.Token (sprintf("%s=Pike_sp[%d%s].u.integer;\n",
				      arg->name(), argnum,argbase),
			      arg->line())
		  });
		  break;

		case "FLOAT_TYPE":
		  ret+=({
		    PC.Token (sprintf("%s=Pike_sp[%d%s].u.float_number;\n",
				      arg->name(), argnum,argbase),
			      arg->line())
		  });
		  break;

		case "struct svalue *":
		  ret+=({
		    PC.Token(sprintf("%s=Pike_sp%+d%s; dmalloc_touch_svalue(Pike_sp%+d%s);\n",
				     arg->name(),
				     argnum,argbase,argnum,argbase),arg->line()),
		  });
		  break;

		case "struct program *":
		  // Program arguments are assigned directly in the check above.
		  break;

		case "LONGEST":
		  ret += ({
		    PC.Token (
		      sprintf ("if (TYPEOF(Pike_sp[%d%s]) == PIKE_T_INT)\n",
			       argnum, argbase),
		      arg->line()),
		    PC.Token (
		      sprintf (" %s = Pike_sp[%d%s].u.integer;\n",
			       arg->name(), argnum, argbase),
		      arg->line()),
		    PC.Token ("else", arg->line()),
		    "\n#if SIZEOF_LONGEST > SIZEOF_INT_TYPE\n",
		    PC.Token (
		      sprintf ("if (!int64_from_bignum (&(%s), "
			       "Pike_sp[%d%s].u.object))\n",
			       arg->name(), argnum, argbase),
		      arg->line()),
		    "\n#endif\n",
		    PC.Token (
		      sprintf (" SIMPLE_ARG_ERROR (%O, %d,"
			       " \"Integer too large.\");\n",
			       attributes->errname || attributes->name || name,
			       argnum+1),
		      arg->line()),
		  });
		  break;

		default:
		  if(arg->is_c_type() && arg->basetype() == "string")
		  {
		    /* some sort of 'char *' */
		    /* This will have to be amended when we want to support
		     * wide strings
		     */
		    ret+=({
		      PC.Token(sprintf("%s=Pike_sp[%d%s].u.string->str; debug_malloc_touch(Pike_sp[%d%s].u.string)\n",
				       arg->name(),
				       argnum,argbase,
				       argnum,argbase),arg->line())
		    });

		  }else{
		    ret+=({
		      PC.Token(sprintf("debug_malloc_pass(%s=Pike_sp[%d%s].u.%s);\n",
				       arg->name(),
				       argnum,argbase,
				       arg->basetype()),arg->line())
		    });
		    if (arg->c_type() == "struct object *") {
		      ret += ({
			PC.Token(sprintf("%s_inh_num = SUBTYPEOF(Pike_sp[%d%s]);\n",
					 arg->name(),
					 argnum,argbase),arg->line())
		      });
		    }
		  }

		case "program":
	      }

	      if (got_void_or_zero_check)
	      {
		string zero_val;
		switch (arg->c_type()) {
		  case "INT_TYPE": zero_val = "0"; break;
		  case "FLOAT_TYPE": zero_val = "0.0"; break;
		  default: zero_val = "NULL"; break;
		}
		ret+=({  PC.Token(sprintf("} else %s = %s;\n",
					  arg->name(), zero_val),
				  arg->line()) });
	      }
	    }

	    argnum++;
	  }

	  body=recursive(fix_return,body,rettype, num_arguments);
	  if(sizeof(body))
	    ret+=({body});
	  ret+=({ "}\n" });

	  if(name_occurances[common_name] > 1)
	  {
	    FuncData d=FuncData();
	    d->name=name;
	    d->define=define;
	    d->type=type;
	    d->attributes=attributes;
	    d->max_args=max_args;
	    d->min_args=min_args;
	    d->args=args;
	    name_data[common_name]=( name_data[common_name] || ({}) ) + ({d});

	    if(name_occurances[common_name]!=name_occurances[common_name+".cnt"])
	    {
	      ret+=rest;
	      continue;
	    }
	    array(FuncData) tmp=map(name_data[common_name],
				    lambda(FuncData fun) {
				      fun->name = mkname(base, fun->name);
				      return fun;
				    });
	    /* Generate real funcname here */
	    name=common_name;
	    funcname=mkname("f",base,common_name);
	    define=make_unique_name("f",base,common_name,"defined");
	    func_num=mkname(base,funcname,"fun_num");
	    array(string) defines=({});

	    type=PikeType(PC.Token("|"), tmp->type);
	    attributes=`|(@ tmp->attributes);

	    array out=generate_overload_func_for(tmp,
						 2,
						 0,
						 0x7fffffff,
						 common_name,
						 attributes);

	    /* FIXME: This definition should be added
	     * somewhere outside of all #ifdefs really!
	     * -Hubbe
	     */
	    global_declarations += ({
              IFDEF(func_num+"_used",
                    ({sprintf("DEFAULT_CMOD_STORAGE ptrdiff_t %s = -1;\n", func_num)})),
	    });
            check_used[func_num] = 1;
	    ret+=IFDEF(tmp->define, ({
	      sprintf("#define %s\n",define),
	      sprintf("DEFAULT_CMOD_STORAGE void %s(INT32 args) ",funcname),
	      "{\n",
	    })+out+({
	      "}\n",
	    }));
	  }
	}
	ret+=rest;

	if (attributes->efun) {
	  if(attributes->optfunc)
	    addfuncs+=IFDEF(define,({
	      PC.Token(sprintf("  ADD_EFUN2(%O, %s, %s, %s, %s, 0);\n",
			       attributes->name || name,
			       funcname,
			       type->output_c_type(),
			       (attributes->optflags)|| "0",
			       attributes->optfunc
			       ), proto[0]->line),
	    }));
	  else
	    addfuncs+=IFDEF(define,({
	      PC.Token(sprintf("  ADD_EFUN(%O, %s, %s, %s);\n",
			       attributes->name || name,
			       funcname,
			       type->output_c_type(),
			       (attributes->optflags)|| "0"
			       ), proto[0]->line),
	    }));
	} else {
          check_used[func_num] = 1;
	  addfuncs+=IFDEF(define, ({
               IFDEF(func_num+"_used",
                    ({PC.Token(sprintf("  %s =", func_num))})),
	    PC.Token(sprintf("    ADD_FUNCTION2(%O, %s, %s, %s, %s);\n",
			     attributes->name || name,
			     funcname,
			     type->output_c_type(),
			     attributes->flags || "0" ,
			     attributes->optflags ||
			     "OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT"
			     ),proto[0]->line),
	  }));
	}
      }

      code=ret;
    }
}

array(PC.Token) convert_comments(array(PC.Token) tokens)
{
  // Filter AutoDoc mk II, and convert other C++ comments to C-style.
  array new = ({});
  array autodoc;

  foreach(tokens, PC.Token token) {
    if(has_prefix((string)token, "//!") ||
       has_prefix((string)token, "/*!")) {
      if(!autodoc)
	autodoc = ({});
      autodoc += ({ ((string)token)[3..] });
    }
    else {
      if(autodoc) {
	new += ({ PC.Token("/*!" + autodoc*"\n *!" ) });
	autodoc = 0;
      }
      if(has_prefix((string)token, "//"))
	new += ({ PC.Token("/*" + ((string)token)[2..] + " */") });
      else
	new += ({ token });
    }
  }

  return new;
}

array(PC.Token) allocate_strings(array(PC.Token|array(PC.Token)) tokens)
{
  int i = -1;

  while ((i = search(tokens, PC.Token("MK_STRING"), i+1)) != -1) {
    // werror("MK_STRING found: %O\n", tokens[i..i+10]);
    if (arrayp(tokens[i+1]) && (sizeof(tokens[i+1]) == 3)) {
      tokens[i] = PC.Token(allocate_string((string)tokens[i+1]->text[1]));
      tokens = tokens[..i] + tokens[i+2..];
    }
  }
  while ((i = search(tokens, PC.Token("MK_STRING_SVALUE"), i+1)) != -1) {
    // werror("MK_STRING_SVALUE found: %O\n", tokens[i..i+10]);
    if (arrayp(tokens[i+1]) && (sizeof(tokens[i+1]) == 3)) {
      tokens[i] = PC.Token(allocate_string_svalue((string)tokens[i+1][1]));
      tokens = tokens[..i] + tokens[i+2..];
    }
  }
  return tokens;
}

class ArgCheckFunction(array tokens)
{
}

class Handler {
    inherit "/master";

    mixed resolv(string identifier, string|void current_file,
		 object|void current_handler) {
	return predef::master()->resolv(identifier, current_file,
					current_handler);
    }

    protected void create(mapping|void predefines) {
	::create();
	if (predefines) this_program::predefines = predefines;
    }
}

int find_identifier( array in, string ident )
{
  foreach( in, mixed q )
  {
    if( objectp(q) )
    {
      if( q == ident )
        return 1;
    }
    else if( arrayp( q ) )
    {
      if( find_identifier( q, ident ) )
        return 1;
    }
  }
}

array resolve_obj_defines()
{
  array res = ({});

  foreach( need_obj_defines; string key; int|string id )
  {
    if( intp( id ) )
    {
      m_delete( need_obj_defines, key );
      continue; /* type in this file. Not external. */
    }
  }
  if( sizeof( need_obj_defines ) )
  {
    res += ({ "{ int i=0;\n"});
    foreach( sort(indices(need_obj_defines)), string key )
    {
      string id = need_obj_defines[key];
      int local_id = ++gid;
      res += ({
        sprintf("#ifndef %s\n"
                "# define %[0]s %s\n"
                "{\n"
                "   struct program *tmp = resolve_program(%s);\n"
                "   if( tmp ) {\n"
                "      ___cmod_ext_used[i].from = %d;\n"
                "      ___cmod_ext_used[i].to = tmp->id;\n"
                "      i++;\n"
                "   }\n"
                "}\n"
                "#endif /* %[0]s */\n",
                key, sprintf("%O", sprintf("\3\0\x7f%3c", local_id)),
                allocate_string(sprintf("%O",id)), local_id)
      });
    }
    res += ({"}\n"});
  }
  return res;
}

int main(int argc, array(string) argv)
{
  mixed x;

  string base = "";

  warnings = 1;
  api = (int)precompile_api_version;

  string outpath;
  if( has_suffix( lower_case(dirname( argv[0] )), "standalone.pmod" ) )
    usage = "pike -x " + basename( argv[0] )-".pike" + " " + usage;
  foreach(Getopt.find_all_options( argv, ({
      ({ "api",        Getopt.HAS_ARG,      "--api"/"," }),
      ({ "help",       Getopt.NO_ARG,       "-h,--help"/"," }),
      ({ "output",     Getopt.HAS_ARG,      "-o,--out"/"," }),
      ({ "version",    Getopt.NO_ARG,       "-v,--version"/"," }),
      ({ "warnings",   Getopt.NO_ARG,       "-w,--warnings"/"," }),
      ({ "base",       Getopt.HAS_ARG,      "-b,--base"/"," }),
   })), array opt ) {
    switch(opt[0]) {
    case "version":
      write( "Precompile format version "+precompile_api_version+"\n");
      return 0;
    case "help":
      write( usage );
      return 0;

    case "api":

        /* If we want to be able to compile old versions of pike with
         * new precompile.pike, and new versions of pike with old
         * precompile.pike we have to ignore the api version.
         *
         * Since none of the API changes really change how previously
         * valid code is interpreted the whole API system only adds
         * incompatibility.
         *
         * As an example, backend.cmod in pike 7.8 can not be
         * precompiled if you only have a pike 7.9 installed, which
         * makes it impossible to build a pike 7.8 if you have pike
         * 7.9+ installed (specifically, the object(Thread.Thread)
         * will break, since newer precompile added actual support for
         * it, while the backend.cmod depends on the old non-support,
         * but both version works just fine if the check is removed).
         *
         * Adding the --api argument to the 7.8 makefiles is not
         * really an option, because that will not work for older
         * releases, and will make it impossible to precompile pike
         * 7.8 .cmod-files with pikes older than 7.9 (which is sort of
         * non-optimal).
         *
         * Similar issues occured when compiling pike 7.9 with an
         * installed pike 7.8.
         *
         * Simply removing the api check makes all issues just go away.
         */
#if 0
      if (lower_case(opt[1]) == "max") {
	// This is used by eg the autodoc extractor, to ensure
	// that all files are precompilable.
	api = (int)precompile_api_version;
	break;
      }
      api = (int)opt[1];
      if (api > (int)precompile_api_version) {
	werror("Unsupported API version: %d (max: %d)\n",
	       api, (int)precompile_api_version);
	return 1;
      } else if (api < 3) {
	// The --api option was not supported by the API level 2 precompiler.
	werror("API level 2 and earlier are implicit, and\n"
	       "must not be specified with the --api option.\n");
	return 1;
      }
#endif
      break;
    case "output":
      outpath = opt[1];
      break;
    case "base":
      base = opt[1];
      break;
    }
  }
  argv = Getopt.get_args(argv);
  if( sizeof( argv ) < 2 )
  {
    write((usage/"\n")[0]+"\n");
    return 1;
  }
  string file = argv[1];

  x=Stdio.read_file(file)-"\r";
#if constant(Pike.__HAVE_CPP_PREFIX_SUPPORT__)
  // Make sure that the user specifies the correct
  // minimum API level for build-script compatibility.
  if (api >= 4) {
    x=sprintf("#cmod_line %d %O\n%s", __LINE__+1, __FILE__,
	      sprintf("#cmod_define cmod_CONCAT_EVAL(x...)\tcmod_CONCAT(x)\n"
		      "#cmod_define cmod_EVAL(x...)\tcmod_CONCAT(x)\n"
		      "#cmod_define cmod_STRFY(x...)\t#x\n"
		      "#cmod_define cmod_DEFINE_EVAL(x...)\tcmod_DEFINE(x)\n"
		      "#cmod_define cmod_STRFY_EVAL(x...)\tcmod_STRFY( x )\n"
		      "#cmod_define cmod_REDEFINE(x)\tcmod_DEFINE(##x, x)\n"
		      "#cmod_define cmod_COMMA ,\n"
                      "#cmod_define cmod_DIRECTIVE_EVAL(x...)"
                      "\tcmod_DIRECTIVE(x)\n"
                      "#cmod_line 1 %q\n"
                      "%s", file, x));
    x=cpp(x, ([
	    "current_file" : file,
	    "prefix" : "cmod",
	    "keep_comments" : 1,
	    "handler" : Handler(([
				  "cmod___SLASH__" : "/",
				  "DOCSTART()" :
				  lambda() { return "cmod___SLASH__*!"; },
				  "DOCEND()" :
				  lambda() { return "*cmod___SLASH__"; },
				  "cmod_DEFINE()" :
				  lambda(string x, string y) {
				    return sprintf("#ifdef %s\n"
						   "#undef %<s\n"
						   "#endif\n"
						   "#define %<s %s", x, y);
				  },
				  "cmod_CONCAT()" :
				  lambda(string ... s) { return s*""; },
				  "cmod___CMOD__" : "1",
                                  "cmod_DIRECTIVE()" :
                                  lambda(string ... s) {
                                    return sprintf("#%{%s\t%}", s);
                                  },
				]))
	  ]));
  } else
#endif
    if (has_value(x, "cmod_include") || has_value(x, "cmod_define") ){
      werror("%[0]s:1: Warning: It looks like %[0]O might use features not present in\n"
             "%[0]s:1: Warning: the pike used to run the precompiler\n", file);
    }
  x=split(x);
  x=PC.tokenize(x,file);
  x = convert_comments(x);
  x=PC.hide_whitespaces(x);
  x=PC.group(x);

  x = ({
    sprintf("/* Generated from %O by precompile.pike running %s\n"
	    " *\n"
	    " * Do NOT edit this file.\n"
	    " */\n",
	    argv[1], version()),
  }) + DEFINE("PRECOMPILE_API_VERSION", (string)precompile_api_version) + ({
    "\n\n",
  })
#if constant(Pike.__HAVE_CPP_PREFIX_SUPPORT__)
  + DEFINE("cmod___CMOD__", "1")
#endif
  + x;

//  werror("%O\n",x);

  x = recursive(allocate_strings, x);

  ParseBlock tmp=ParseBlock(x, base, "", ([]));

  tmp->declarations += ({
    "\n\n"
    "#ifndef PIKE_UNUSED_ATTRIBUTE\n"
    "#define PIKE_UNUSED_ATTRIBUTE\n"
    "#endif\n"
    "#define CMOD_MAP_PROGRAM_IDS_DEFINED 1\n"
    "static int ___cmod_map_program_ids(int id) PIKE_UNUSED_ATTRIBUTE;\n"
    "#ifndef TYPEOF\n"
    "/* Compat with older Pikes. */\n"
    "#define TYPEOF(SVAL)\t((SVAL).type)\n"
    "#define SUBTYPEOF(SVAL)\t((SVAL).subtype)\n"
    "#define SET_SVAL_TYPE(SVAL, TYPE)\t(TYPEOF(SVAL) = TYPE)\n"
    "#define SET_SVAL_SUBTYPE(SVAL, TYPE)\t(SUBTYPEOF(SVAL) = TYPE)\n"
    "#define SET_SVAL(SVAL, TYPE, SUBTYPE, FIELD, EXPR) do {\t\\\n"
    "    /* Set the type afterwards to avoid a clobbered\t\\\n"
    "     * svalue in case EXPR throws. */\t\t\t\\\n"
    "    (SVAL).u.FIELD = (EXPR);\t\t\t\t\\\n"
    "    SET_SVAL_TYPE((SVAL), (TYPE));\t\t\t\\\n"
    "    SET_SVAL_SUBTYPE((SVAL), (SUBTYPE));\t\t\\\n"
    "  } while(0)\n"
    "#endif /* !TYPEOF */\n"
    "#ifndef PIKE_UNUSED\n"
    "/* Compat with Pike 7.8 and earlier. */\n"
    "/* NB: Not strictly correct; PIKE_UNUSED was added the\n"
    " *     day after set_program_id_to_id(), but good enough.\n"
    " */\n"
    "#ifndef UNUSED\n"
    "#define UNUSED(X)	X\n"
    "#endif\n"
    "#ifndef set_program_id_to_id\n"
    "/* NB: Recent Pike 7.8 has a #define that conflicts with\n"
    " *     the following declaration.\n"
    " */\n"
    "static void set_program_id_to_id(void*UNUSED(id)){}\n"
    "#endif\n"
    "#else /* */\n"
    "PMOD_EXPORT void set_program_id_to_id( int (*to)(int) );\n"
    "#endif /* !PIKE_UNUSED */\n"
    "#if !defined(string_has_null) && !defined(STRING_CONTENT_CHECKED)\n"
    "/* This symbol was added as a macro in Pike 7.7.20, and is an\n"
    " * inline function in Pike 7.9.4 and later, at which time the\n"
    " * flag STRING_CONTENT_CHECKED was added.\n"
    " */\n"
    "#define string_has_null(X) (strlen((X)->str)!=(size_t)(X)->len)\n"
    "#endif\n"
    "\n\n",

    "#ifndef DEFAULT_CMOD_STORAGE\n"
    "#define CMOD_COND_USED\n"
    "#define DEFAULT_CMOD_STORAGE static\n"
    "#endif\n"
  });


  tmp->addfuncs =
    IFDEF("CMOD_MAP_PROGRAM_IDS_DEFINED",
          resolve_obj_defines() +
          ({ "set_program_id_to_id( ___cmod_map_program_ids );\n" }))
    + tmp->addfuncs
    + IFDEF("CMOD_MAP_PROGRAM_IDS_DEFINED",
            ({ "set_program_id_to_id( 0 );" }));

  if (last_str_id) {
    // Add code for allocation and deallocation of the strings.
    tmp->addfuncs =
      ({ "\n#ifdef module_strings_declared\n" }) +
      stradd +
      ({ "#endif\n" }) + tmp->addfuncs;
    tmp->exitfuncs += ({
      sprintf("\n#ifdef module_strings_declared\n"
	      "{\n"
	      "  int i;\n"
	      "  for(i=0; i < %d; i++) {\n"
	      "    if (module_strings[i]) free_string(module_strings[i]);\n"
	      "    module_strings[i] = NULL;\n"
	      "  }\n"
	      "}\n"
	      "#endif\n",
	      last_str_id),
    });

    tmp->declarations += ({
      sprintf("#define module_strings_declared\n"
	      "static struct pike_string *module_strings[%d] = {\n"
	      "%s};\n",
	      last_str_id, "  NULL,\n"*last_str_id),
    });

    // Same for svalues.
    // NOTE: This code needs changing in several aspects if
    //       support for other svalues than strings is added.
    if (last_svalue_id) {
      tmp->exitfuncs += ({
	sprintf("\n#ifdef module_svalues_declared\n"
		"free_svalues(module_svalues, %d, BIT_STRING);\n"
		"#endif\n",
		last_svalue_id),
      });
      tmp->declarations += ({
	sprintf("#define module_svalues_declared\n"
		"static struct svalue module_svalues[%d];\n",
		last_svalue_id),
      });
    }
  }

  x = tmp->code;

  x += ({
    "\n"
    "#ifdef CMOD_MAP_PROGRAM_IDS_DEFINED\n"
    "static int ___cmod_map_program_ids(int id)\n"
    "{\n"
    "  if( (id&0x7f000000) != 0x7f000000 ) return id;\n"
    "  id = id&0x00ffffff;\n"
  });

  if( sizeof(map_types) )
  {
    x += ({ "  switch(id) {\n" });
    foreach( sort(indices(map_types)), int i )
    {
      string how = map_types[i];
      if( how[0] )
        x += ({ "#ifdef "+how[0]+"\n" });
      x += ({  "  case "+i+":\n    "+how[1]+"\n    break;\n" });
      if( how[0] )
        x += ({ "#endif\n" });
    }
    x += ({ "  };\n\n" });
  }

  if( sizeof( need_obj_defines ) )
  {
    tmp->declarations += ({
      "static struct { INT32 from;INT32 to; } ___cmod_ext_used[" +
      sizeof( need_obj_defines ) + "];\n"
    });

    x += ({
      "  {\n"
      "    int i = 0;\n"
      "    while(___cmod_ext_used[i].from ) {\n"
      "      if( ___cmod_ext_used[i].from == id ) {\n"
      "        return ___cmod_ext_used[i].to;\n"
      "      }\n"
      "      i++;\n"
      "    }\n"
      "  }\n"
    });
  }
  x += ({
    "  return 0;\n"
    "}\n"
    "#endif /* CMOD_MAP_PROGRAM_IDS_DEFINED */\n"
  });

  tmp->code = x;
  x=recursive(replace,x,PC.Token("INIT",0),tmp->addfuncs);
  int need_init;
  if ((need_init = equal(x, tmp->code)))
  {
    // No INIT, add our own stuff..

    x+=({
      "PIKE_MODULE_INIT {\n",
      tmp->addfuncs,
      "}\n",
    });
  }
  tmp->code = x;
  x=recursive(replace,x,PC.Token("EXIT",0),tmp->exitfuncs);
  if(equal(x, tmp->code))
  {
    // No EXIT, add our own stuff..

    x+=({
      "PIKE_MODULE_EXIT {\n",
      tmp->exitfuncs,
      "}\n",
    });

    if (!need_init) {
      werror("Warning: INIT without EXIT. Added PIKE_MODULE_EXIT.\n");
    }
  } else if (need_init) {
    werror("Warning: EXIT without INIT. Added PIKE_MODULE_INIT.\n");
  }
  tmp->code = x;

  // FIXME: This line is fishy; there is no optfuncs in ParseBlock.
  x=recursive(replace,x,PC.Token("OPTIMIZE",0),tmp->optfuncs);

  array(string) cond_used = ({});
  foreach( sort(indices(check_used)), string key)
  {
    if( find_identifier( x, key ) )
      tmp->declarations +=({ "#define "+key+"_used 1\n" });
    else
      cond_used += ({ key });
  }
  if (sizeof(cond_used)) {
    tmp->declarations += ({ "#ifndef CMOD_COND_USED\n" });
    foreach(cond_used, string key) {
      tmp->declarations += ({ "# define "+key+"_used 1\n" });
    }
    tmp->declarations += ({ "#else /* !CMOD_COND_USED */\n" });
    foreach(cond_used, string key) {
      tmp->declarations += ({ "# undef "+key+"_used\n" });
    }
    tmp->declarations += ({ "#endif /* !CMOD_COND_USED */\n" });
  }

  tmp->declarations += global_declarations;
  tmp->code = x;
  x=recursive(replace,x,PC.Token("DECLARATIONS",0),tmp->declarations);


  if(equal(x,tmp->code))
  {
    // No OPTIMIZE / DECLARATIONS
    // FIXME: Add our own stuff...
    // NOTA BENE: DECLARATIONS are not handled automatically
    //            on the file level
  }
  Stdio.File out = Stdio.stdout;
  if (outpath) {
    out = Stdio.File(outpath, "twc", 0666);
  }
  if(getenv("PIKE_DEBUG_PRECOMPILER"))
    out->write(PC.simple_reconstitute(x));
  else
    out->write(PC.reconstitute_with_line_numbers(x));
}
