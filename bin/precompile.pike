#!/usr/local/bin/pike

#define FUNC_OVERLOAD

/*
 * This script is used to process *.cmod files into *.c files, it 
 * reads Pike style prototypes and converts them into C code.
 *
 * The input can look something like this:
 *
 * PIKECLASS fnord 
 *  attributes;
 * {
 *   INHERIT bar
 *     attributes;
 *
 *   CVAR int foo;
 *   PIKEVAR mapping m;
 *
 *   DECLARE_STORAGE; // optional
 *
 *   PIKEFUN int function_name (int x, CTYPE char * foo)
 *    attribute;
 *    attribute value;
 *   {
 *     C code using 'x' and 'foo'.
 *
 *     RETURN x;
 *   }
 * }
 *
 * All the begin_class/ADD_EFUN/ADD_FUNCTION calls will be inserted
 * instead of the word INIT in your code.
 *
 * The corresponding cleanup code will be inserted instead of the word EXIT.
 *
 * Currently, the following attributes are understood:
 *   efun;          makes this function a global constant (no value)
 *   flags;         ID_STATIC | ID_NOMASK etc.
 *   optflags;      OPT_TRY_OPTIMIZE | OPT_SIDE_EFFECT etc.
 *   type;          tInt, tMix etc. use this type instead of automatically
 *                  generating a type from the prototype
 *                  FIXME: this doesn't quite work
 *   errname;       The name used when throwing errors.
 *   name;          The name used when doing add_function.
 *   prototype;     Ignore the function body, just add a prototype entry.
 *   program_flags; PROGRAM_USES_PARENT | PROGRAM_DESTRUCT_IMMEDIATE etc.
 *
 * POLYMORPHIC FUNCTION OVERLOADING
 *   You can define the same function several times with different
 *   types. This program will select the proper function whenever
 *   possible.
 *
 * BUGS/LIMITATIONS
 *  o Parenthesis must match, even within #if 0
 *  o Not all Pike types are supported yet.
 *  o No support for functions that take a variable number of arguments yet.
 *  o RETURN; (void) doesn't work yet
 *  o need a RETURN_NULL; or something.. RETURN 0; might work but may
 *    be confusing as RETURN x; will not work if x is zero.
 *  o Comments does not work inside prototypes (?)
 *  o Allow INHERIT
 *
 * C ENVIRONMENT
 *  FIXME
 */

#define PC Parser.Pike

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
	    
	  case "object":
	  case "program":
	  case "function":
	  case "mapping":
	  case "array":
	  case "multiset":
	  case "int":
	    if(arrayp(t[p])) p++;
	  case "string":
	  case "float":
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
 * This function maps C types to the approperiate
 * pike type. Input and outputs are arrays of tokens
 */
PikeType convert_ctype(array tokens)
{
  switch((string)tokens[0])
  {
    case "char": /* char* */
      if(sizeof(tokens) >1 && "*" == (string)tokens[1])
	return PikeType("string");

    case "short":
    case "int":
    case "long":
    case "size_t":
    case "ptrdiff_t":
    case "INT32":
      return PikeType("int");

    case "double":
    case "float":
      return PikeType("float");

    default:
      werror("Unknown C type.\n");
      exit(0);
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

  static string fiddle(string name, array(string) tmp)
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

	case "|":
	  array(string) tmp=args->basetype();
	  if(`==(@tmp)) return tmp[0];
	  return "mixed";

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
	case "9": return "mixed";

	default:  return ret;
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

	default:  return ({ ret });
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
   * Return 1 if this type is or matches 'void'
   */
  int may_be_void()
    {
      switch((string)t)
      {
	case "void": return 1;
	case "|":
	  for(int e=0;e<sizeof(args);e++)
	    if(args[e]->may_be_void())
	      return 1;
	  return 0;

	case "=":
	case "&":
	  return args[-1]->may_be_void();
      }
    }

  string c_storage_type(int|void is_struct_entry)
    {
      switch(string btype=basetype())
      {
	case "void": return "void";
	case "int": return "INT_TYPE";
	case "float": return "FLOAT_TYPE";
	case "string": return "struct pike_string *";
	  
	case "array":
	case "multiset":
	case "mapping":
	  
	case "object":
	case "program": return "struct "+btype+" *";
	  
	case "function":
	case "mixed":
	  if (is_struct_entry) {
	    return "struct svalue";
	  } else {
	    return "struct svalue *";
	  }
	default:
	  werror("Unknown type %s\n",btype);
	  exit(1);
      }
    }

  /*
   * Make a C representation, like 'tInt'
   */
  string output_c_type()
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
	  return fiddle("tOr",args->output_c_type());

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
			 args[1]->output_c_type());

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
	case "string":  return "tString";
	case "object":  return "tObj";
	case "program": return "tPrg(tObj)";
	case "any":     return "tAny";
	case "mixed":   return "tMix";
	case "int":
	  // NOTE! This piece of code KNOWS that PIKE_T_INT is 8!
	  return stringify(sprintf("\010%4c%4c",
				   (int)(string)(args[0]->t),
				   (int)(string)(args[1]->t)));

	default:
	  error("Don't know how to convert %O to C type.\n",t);
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
	  ret=sprintf("int(%s..%s)",
		      args[0]->t == "-2147483648" ? "" : args[0]->t,
		      args[1]->t ==  "2147483647" ? "" : args[1]->t);
	  if(ret=="int(..)") return "int";
	  return ret;

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
   * Copy and remove type assignments
   */
  PikeType copy_and_strip_type_assignments()
    {
      if("=" == (string)t)
	return args[1]->copy_and_strip_type_assignments();
      return PikeType(t, args && args->copy_and_strip_type_assignments());
    }

  string _sprintf(int how)
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
  void create(string|array(PC.Token)|PC.Token|array(array) tok,
	      void|array(PikeType) a)
    {
      switch(sprintf("%t",tok))
      {
	case "object":
	  t=tok;
	  args=a;
	  break;

	case "string":
	  tok=convert_comments(PC.tokenize(PC.split(tok),"piketype"));
	  tok=PC.group(PC.hide_whitespaces(tok));
	  
	case "array":
	  /* strip parenthesis */
	  while(sizeof(tok) == 1 && arrayp(tok[0]))
	    tok=tok[0][1..sizeof(tok[0])-2];

	  array(array(PC.Token|array(PC.Token|array))) tmp;
	  tmp=tok/({"|"});
	  if(sizeof(tmp) >1)
	  {
	    t=PC.Token("|");
	    args=map(tmp,PikeType);
	    return;
	  }

	  tmp=tok/({"&"});
	  if(sizeof(tmp) >1)
	  {
	    t=PC.Token("&");
	    args=map(tmp,PikeType);
	    return;
	  }

	  tmp=tok/({"="});
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
		string low = (string)(int)-0x80000000;
		string high = (string)0x7fffffff;
		args=({PikeType(PC.Token(low)),PikeType(PC.Token(high))});
		break;

	      case "function":
		args=({ PikeType("mixed"), PikeType("any") });
		break;
	    }
	  }else{
	    array q;
	    switch((string)t)
	    {
	      case "!":
		args=({ PikeType(tok[1..]) });
		break;

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
		  args=({PikeType("mixed"),PikeType("any")});
		}
		return;
		
	      case "int":
		string low = (string)(int)-0x80000000;
		string high = (string)0x7fffffff;

		if(arrayp(q=tok[1]))
		{
		  tmp=q[1..sizeof(q)-2]/({".."});
		  /* Workaround for buggy Parser.Pike */
		  if(sizeof(tmp)==1) tmp=q[1..sizeof(q)-2]/({".. "});
		  if(sizeof(tmp[0])) low=(string)tmp[0][0];
		  if(sizeof(tmp[1])) high=(string)tmp[1][0];
		}
		args=({PikeType(PC.Token(low)),PikeType(PC.Token(high))});
		break;
	    }
	  }
      }
    }
};

/*
 * This class is used to represe one function argument
 */
class Argument
{
  /* internal */
  string _name;
  PikeType _type;
  string _c_type;
  string _basetype;
  int _is_c_type;
  int _line;
  string _file;
  string _typename;

  int is_c_type() { return _is_c_type; }
  int may_be_void() { return type()->may_be_void(); }
  int line() { return _line; }
  string name() { return _name; }
  PikeType type() { return _type; }

  string basetype()
    {
      if(_basetype) return _basetype;
      return _basetype = type()->basetype();
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
	type()->copy_and_strip_type_assignments()->output_pike_type(0);
    }

  void create(array x)
    {
      _name=(string)x[-1];
      _file=x[-1]->file;
      _line=x[-1]->line;
      _type=PikeType(x[..sizeof(x)-2]);
    }

  string _sprintf(int how)
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


// Workaround for bug in F_RECUR in some sub-versions of Pike7.1.34.
function(mixed,array,mixed ...:array) low_recursive = recursive;

array recursive(mixed func, array data, mixed ... args)
{
  array ret=({});

  foreach(data, mixed foo)
    {
      if(arrayp(foo))
      {
	ret+=({ low_recursive(func, foo, @args) });
      }else{
	ret+=({ foo });
      }
    }

  return func(ret, @args);
}

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
mapping parse_attributes(array attr, void|string location)
{
  mapping attributes=([]);
  foreach(attr/ ({";"}), attr)
    {
      switch(sizeof(attr))
      {
	case 0: break;
	case 1:
	  if(arrayp(attr[0]))
	  {
	    werror("%s:%d: Expected attribute name!\n",
		   attr[0][0]->file,
		   attr[0][0]->line);
	    if(location)
	      werror("%s: This is where the attributes belong\n", location);
	    werror("This is what I got: %O\n", attr[0]);
	    exit(1);
	  }
	  attributes[(string)attr[0]]=1;
	  break;
	default:
	  array tmp=attr[1..];
	  if(sizeof(tmp) == 1 && arrayp(tmp[0]) && tmp[0][0]=="(")
	    tmp=tmp[0][1..sizeof(tmp[0])-2];
	  
	  attributes[(string)attr[0]]=merge(tmp);
      }
    }
  return attributes;
}

string file;

/*
 * Generate an #ifdef/#else/#endif
 */
array IFDEF(string|array(string) define,
	    array yes,
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
      ret+=no;
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


#ifdef FUNC_OVERLOAD
class FuncData
{
  string name;
  string define;
  PikeType type;
  mapping attributes;
  int max_args;
  int min_args;
  array(Argument) args;

  string _sprintf()
    {
      return sprintf("FuncData(%s)",define);
    }
  
  int `==(mixed q)
    {
      return objectp(q) && q->name == name;
//      return 0;
    }
};


int evaluate_method(mixed q)
{
  int val=0;
  q=values(q) - ({ ({}) });
//  werror("evaluate: %O\n",q);
  for(int e=0;e<sizeof(q);e++)
  {
    if(q[e] && sizeof(q[e]))
    {
      val++;
      for(int d=e+1;d<sizeof(q);d++)
      {
	if(!sizeof(q[e] ^ q[d])) /* equal is broken in some Pikes */
	{
//	  werror("EQ, %d %d\n",e,d);
	  q[d]=0;
	}
      }
    }
  }
  return val;
}

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

  array out=({});

  /* This part should be recursive */
  array(array(FuncData)) x=allocate(256,({}));
  int min_args=0x7fffffff;
  int max_args=0;
  foreach(d, FuncData q)
    {
      for(int a=q->min_args;a<min(q->max_args,256);a++)
	x[a]+=({q});
      min_args=min(min_args, q->min_args);
      max_args=max(max_args, q->max_args);
    }

  min_args=max(min_args, min_possible_arg);
  max_args=min(max_args, max_possible_arg);

//  werror("MIN: %O\n",min_args);
//  werror("MAX: %O\n",max_args);

  string argbase="-args";

  int best_method;
  int best_method_value;

  array(mapping(string:array(FuncData))) y;
  if(min_args)
  {
    y=allocate(min(min_args,16), ([]));
    for(int a=0;a<sizeof(y);a++)
    {
      foreach(d, FuncData q)
	{
//	  werror("BT: %s\n",q->args[a]->type()->basetypes()*"|");
	  foreach(q->args[a]->type()->basetypes(), string t)
	    {
	      if(!y[a][t]) y[a][t]=({});
	      y[a][t]+=({q});
	    }
	}
    }

    best_method=-1;
    best_method_value=evaluate_method(x);
//    werror("Value X: %d\n",best_method_value);
    
    for(int a=0;a<sizeof(y);a++)
    {
      int v=evaluate_method(y[a]);
//      werror("Value %d: %d\n",a,v);
      if(v>best_method_value)
      {
	best_method=a;
	best_method_value=v;
      }
    }
  }

//  werror("Best method=%d\n",best_method);

  if(best_method == -1)
  {
    /* Switch on number of arguments */
    out+=({PC.Token(sprintf("%*nswitch(args) {\n",indent))});
    for(int a=min_args;a<max_args;a++)
    {
      array tmp=x[a];
      if(tmp && sizeof(tmp))
      {
	int d;
	for(int d=a;d<sizeof(x);d++)
	{
	  if(equal(tmp, x[d]) && !sizeof(tmp ^ x[d]))
	  {
	    out+=({sprintf("%*n case %d:\n",indent,d) });
	    x[d]=0;
	  }else{
	    break;
	  }
	}
	out+=generate_overload_func_for(tmp,
					indent+2,
					a,
					d,
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
    
    out+=({PC.Token(sprintf("%*nswitch(Pike_sp[%d%s].type) {\n",
			    indent,
			    best_method,argbase)) });

    mapping m=y[best_method];
    mapping m2=m+([]);
    foreach(indices(m), string type)
      {
	array tmp=m[type];
	if(tmp && sizeof(tmp))
	{
	  foreach(indices(m), string t2)
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
	PC.Token(sprintf("%*n  SIMPLE_BAD_ARG_ERROR(%O,%d,%O);\n",
			 indent,
			 attributes->errname || name,
			 best_method+1,
			 indices(m2)*"|")),
	PC.Token(sprintf("%*n}\n",indent)),
	});
  }
  return out;
}



#endif

/*
 * Parse a block of code
 */
class ParseBlock
{
  array code=({});
  array addfuncs=({});
  array exitfuncs=({});
  array declarations=({});

  void create(array x, string base)
    {
      array ret=x;

      x=ret/({"PIKECLASS"});
      ret=x[0];

      for(int f=1;f<sizeof(x);f++)
      {
	array func=x[f];
	int p;
	for(p=0;p<sizeof(func);p++)
	  if(arrayp(func[p]) && func[p][0]=="{")
	    break;
	array proto=func[..p-1];
	array body=func[p];
	array rest=func[p+1..];
	string name=(string)proto[0];
	mapping attributes=parse_attributes(proto[1..]);

	ParseBlock subclass = ParseBlock(body[1..sizeof(body)-2],name);
	string program_var =mkname(name,"program");

	string define=make_unique_name("class",name,"defined");

	ret+=DEFINE(define);
	ret+=({sprintf("struct program *%s=0;\n",program_var)});
	ret+=subclass->declarations;
	ret+=subclass->code;

	addfuncs+=
	  IFDEF(define,
		({
		  IFDEF("PROG_"+upper_case(name)+"_ID",
			({
			  sprintf("  START_NEW_PROGRAM_ID(%s);\n",
				  upper_case(name)),
			  "#else\n",
			  "  start_new_program();\n"
			}))
		})+
		IFDEF("THIS_"+upper_case(name),
		      ({ sprintf("\n  %s_storage_offset=ADD_STORAGE(struct %s_struct);\n",name,name) }) )+
		subclass->addfuncs+
		({
		  attributes->program_flags?
		  sprintf("  Pike_compiler->new_program->flags |= %s;\n",
			  attributes->program_flags):"",
		  sprintf("  %s=end_program();\n",program_var),
		  sprintf("  add_program_constant(%O,%s,%s);\n",
			  name,
			  program_var,
			  attributes->flags || "0"),
		    })
	    );
	exitfuncs+=
	  IFDEF(define,
		subclass->exitfuncs+
	    ({
	      sprintf("  if(%s) {\n    free_program(%s);\n    %s=0;\n  }\n",
		      program_var,program_var,program_var),
	      }));

	ret+=rest;
      }

      x=ret/({"INHERIT"});
      ret = x[0];
      for (int f = 1; f < sizeof(x); f++)
      {
	array inh=x[f];
	int pos=search(inh,PC.Token(";",0),);
	mixed name=inh[0];
	array rest=inh[pos+1..];
	string define=make_unique_name("inherit",name,base,"defined");
	mapping attributes = parse_attributes(inh[1..pos]);
	addfuncs +=
	  IFDEF(define,
		({
		  sprintf("  low_inherit(%s, NULL, -1, 0, %s, NULL);",
			  mkname((string)name, "program"),
			  attributes->flags || "0")
		}));
	ret+=DEFINE(define);
	ret+=rest;
      }

      array thestruct=({});
      x=ret/({"PIKEVAR"});
      ret=x[0];
      for(int f=1;f<sizeof(x);f++)
      {
	array var=x[f];
	int pos=search(var,PC.Token(";",0),);
	int pos2=parse_type(var,0);
	mixed name=var[pos2];
	PikeType type=PikeType(var[..pos2-1]);
	array rest=var[pos+1..];
	string define=make_unique_name("var",name,base,"defined");
	mapping attributes = parse_attributes(var[pos2+1..pos]);
    
//    werror("type: %O\n",type);

	thestruct+=
	  IFDEF(define,
		({ sprintf("  %s %s;\n",type->c_storage_type(1),name) }));
	addfuncs+=
	  IFDEF(define,
		({
		  sprintf("  map_variable(%O,%O,%s,%s_storage_offset + OFFSETOF(%s_struct, %s), %s);",
			  (string)name,
			  type->output_pike_type(0),
			  attributes->flags || "0",
			  base,
			  base,
			  name,
			  type->type_number()),
		    }));
	ret+=DEFINE(define);
	ret+=({ PC.Token("DECLARE_STORAGE") });
	ret+=rest;
      }

      x=ret/({"CVAR"});
      ret=x[0];
      for(int f=1;f<sizeof(x);f++)
      {
	array var=x[f];
	int pos=search(var,PC.Token(";",0),);
	int npos=pos-1;
	while(arrayp(var[npos])) npos--;
	mixed name=(string)var[npos];

	string define=make_unique_name("var",name,base,"defined");
    
	thestruct+=IFDEF(define,var[..pos-1]+({";"}));
	ret+=DEFINE(define);
	ret+=({ PC.Token("DECLARE_STORAGE") });
	ret+=var[pos+1..];
      }


      
      ret=(ret/({"INIT"}))*({PC.Token("PIKE_INTERNAL"),PC.Token("init")});
      ret=(ret/({"EXIT"}))*({PC.Token("PIKE_INTERNAL"),PC.Token("exit")});
      ret=(ret/({"GC_RECURSE"}))*({PC.Token("PIKE_INTERNAL"),PC.Token("gc_recurse")});
      ret=(ret/({"GC_CHECK"}))*({PC.Token("PIKE_INTERNAL"),PC.Token("gc_check")});
      ret=(ret/({"OPTIMIZE"}))*({PC.Token("PIKE_INTERNAL"),PC.Token("optimize")});

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
	
	addfuncs+=IFDEF(ev_handler_define,
			({ PC.Token(sprintf("  pike_set_prog_event_callback(%s);\n",funcname)) }));
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
	string this=sprintf("((struct %s *)(Pike_interpreter.frame_pointer->current_storage))",structname);

	/* FIXME:
	 * Add runtime debug to these defines...
	 * Add defines for parents when possible
	 */
	declarations=
	  	  DEFINE("THIS",this)+   // FIXME: we should 'pop' this define later
	  DEFINE("THIS_"+upper_case(base),this)+
	  DEFINE("OBJ2_"+upper_case(base)+"(o)",
		 sprintf("((struct %s *)(o->storage+%s_storage_offset))",
			 structname, base))+
	  DEFINE("GET_"+upper_case(base)+"_STORAGE",
		 sprintf("((struct %s *)(o->storage+%s_storage_offset)",
			 structname, base))+
	    ({
	      sprintf("static ptrdiff_t %s_storage_offset;\n",base),
		sprintf("struct %s {\n",structname),
		})+thestruct+({
		  "};\n",
		    })
	  +declarations;
      }

      x=ret/({"DECLARE_STORAGE"});
      ret=x[..sizeof(x)-2]*({})+declarations+x[-1];
      declarations=({});

      x=ret/({"PIKEFUN"});
      ret=x[0];

#ifdef FUNC_OVERLOAD
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
	  werror("%s:%d: Missing return type?\n",
		 proto[p][0]->file||"-",
		 proto[p][0]->line);
	  exit(1);
	}
	string name=(string)proto[p];
	name_occurances[name]++;
      }
#endif

      for(int f=1;f<sizeof(x);f++)
      {
	array func=x[f];
	int p;
	for(p=0;p<sizeof(func);p++)
	  if(arrayp(func[p]) && func[p][0]=="{")
	    break;

	array proto=func[..p-1];
	array body=func[p];
	array rest=func[p+1..];

	// Not all versions of Pike allow Token objects
	// to be indexed.
	catch {
	  foreach(proto, string t)
	    if(has_prefix(t, "/*!"))
	      body = ({t})+body;
	};

	p=parse_type(proto,0);
	PikeType rettype=PikeType(proto[..p-1]);

	if(arrayp(proto[p]))
	{
	  werror("%s:%d: Missing type?\n",
		 proto[p][0]->file||"-",
		 proto[p][0]->line);
	  exit(1);
	}
	string location=proto[p]->file+":"+proto[p]->line;
	string name=(string)proto[p];
	array args_tmp=proto[p+1];

	mapping attributes=parse_attributes(proto[p+2..],location);
#ifdef FUNC_OVERLOAD
	string common_name=name;
	if(!attributes->errname)
	  attributes->errname=name;
	if(name_occurances[common_name]>1)
	  name+="_"+(++name_occurances[common_name+".cnt"]);
#endif

	args_tmp=args_tmp[1..sizeof(args_tmp)-2];
	if(sizeof(args_tmp))
	{
	  args_tmp/=({","});
	}else{
	  args_tmp=({});
	}

	string funcname=mkname("f",base,name);
	string define=make_unique_name("f",base,name,"defined");
	string func_num=mkname("f",base,name,"fun_num");

//    werror("FIX RETURN: %O\n",body);
    
//    werror("name=%s\n",name);
//    werror("  rettype=%O\n",rettype);
//    werror("  args=%O\n",args);

	ret+=({
	  sprintf("#define %s\n", define),
	});

	if (!attributes->efun) {
	  ret += ({
	    sprintf("ptrdiff_t %s = 0;\n", func_num),
	  });
	}

	// werror("%O %O\n",proto,args);
	int last_argument_repeats;
	if(sizeof(args_tmp) && 
	   !arrayp(args_tmp[-1][-2]) &&
	   "..." == (string)args_tmp[-1][-2])
	{
	  last_argument_repeats++;
	  args_tmp[-1]=args_tmp[-1][..sizeof(args_tmp[-1])-3]+({
	    args_tmp[-1][-1]});
	}
	array(Argument) args=map(args_tmp,Argument);
	// werror("%O %O\n",proto,args);
	// werror("parsed args: %O\n", args);

	// FIXME: support ... types
	PikeType type;

	if(attributes->type)
	{
	  mixed err=catch {
	    type=PikeType(attributes->type);
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
	  ret+=({
	    sprintf("void %s(INT32 args) ",funcname),
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
	  }

	  while(min_args>0 && args[min_args-1]->may_be_void())
	    min_args--;

	  foreach(args, Argument arg)
	    ret+=({
	      PC.Token(sprintf("%s %s;\n",arg->c_type(), arg->name()),
		       arg->line()),
	    });


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
	    if(arg->may_be_void() && "mixed" != (string)arg->basetype())
	    {
	      ret+=({
		PC.Token(sprintf("if(args > %s) ",argnum)),
	      });
	    }
	    if (argnum == repeat_arg) {
	      // Begin the argcnt loop.
	      ret += ({
		PC.Token(sprintf("{\n"
				 "  INT32 argcnt;\n"
				 "  for (argcnt=0; argcnt < %s-%d; argcnt++) {\n",
				 num_arguments, argnum),
			 arg->line()) });
	      check_argbase = "+argcnt"+argbase;
	    }
	    if(arg->is_c_type() && arg->basetype() == "string")
	    {
	      /* Special case for 'char *' */
	      /* This will have to be amended when we want to support
	       * wide strings
	       */
	      ret+=({
		PC.Token(sprintf("if(Pike_sp[%d%s].type != PIKE_T_STRING || Pike_sp[%d%s].ustring -> width)",
				 argnum,check_argbase,
				 argnum,check_argbase,
				 upper_case(arg->basetype())),arg->line())
	      });
	    }else

	      switch(arg->basetype())
	      {
	      default:
		ret+=({
		  PC.Token(sprintf("if(Pike_sp[%d%s].type != PIKE_T_%s)",
				   argnum,check_argbase,
				   upper_case(arg->basetype())),arg->line())
		});
		break;

	      case "program":
		ret+=({
		  PC.Token(sprintf("if(!( %s=program_from_svalue(Pike_sp%+d%s)))",
				   arg->name(),argnum,check_argbase),
			   arg->line())
		});
		break;

	      case "mixed":
	      }
	    switch(arg->basetype())
	    {
	    default:
	      ret+=({
		PC.Token(sprintf(" SIMPLE_BAD_ARG_ERROR(%O,%d%s,%O);\n",
				 attributes->errname || attributes->name || name,
				 argnum+1,
				 (argnum == repeat_arg)?"+argcnt":"",
				 arg->typename()),arg->line()),
	      });

	    case "mixed":
	    }

	    if (argnum == repeat_arg) {
	      // End the argcnt loop.
	      ret += ({ PC.Token("  }\n}\n", arg->line()) });
	    }

	    if(arg->may_be_void())
	    {
	      ret+=({
		PC.Token(sprintf("if(args > %d) { ",argnum)),
	      });
	    }

	    switch(arg->basetype())
	    {
	    case "int":
	      ret+=({
		sprintf("%s=Pike_sp[%d%s].u.integer;\n",arg->name(),
			argnum,argbase)
	      });
	      break;

	    case "float":
	      ret+=({
		sprintf("%s=Pike_sp[%d%s].u.float_number;\n",
			arg->name(),
			argnum,argbase)
	      });
	      break;

	    case "mixed":
	      ret+=({
		PC.Token(sprintf("%s=Pike_sp%+d%s; dmalloc_touch_svalue(Pike_sp%+d%s);\n",
				 arg->name(),
				 argnum,argbase,argnum,argbase),arg->line()),
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
	      }

	    case "program":
	    }

	    if(arg->may_be_void())
	    {
	      ret+=({  PC.Token(sprintf("}else{\n"
					"%s=0;\n"
					"}", arg->name())) });
	    }

	    argnum++;
	  }
    
	  body=recursive(fix_return,body,rettype, num_arguments); 
	  if(sizeof(body))
	    ret+=({body});
	  ret+=({ "}\n" });

#ifdef FUNC_OVERLOAD
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
	    ret+=IFDEF(tmp->define, ({
	      sprintf("#define %s\n",define),
	      sprintf("void %s(INT32 args) ",funcname),
	      "{\n",
	    })+out+({
	      "}\n",
	    }));
	  
	  }
#endif
	}
	ret+=rest;
	

	if (attributes->efun) {
	  addfuncs+=IFDEF(define,({
	    PC.Token(sprintf("  ADD_EFUN(%O, %s, %s, %s);\n",
			     attributes->name || name,
			     funcname,
			     type->output_c_type(),
			     (attributes->optflags)|| "0" ,
			     ),proto[0]->line),
	  }));
	} else {
	  addfuncs+=IFDEF(define, ({
	    PC.Token(sprintf("  %s =\n"
			     "    ADD_FUNCTION2(%O, %s, %s, %s, %s);\n",
			     func_num,
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

int main(int argc, array(string) argv)
{
  mixed x;

  file=argv[1];
  x=Stdio.read_file(file);
  x=PC.split(x);
  x=PC.tokenize(x,file);
  x = convert_comments(x);
  x=PC.hide_whitespaces(x);
  x=PC.group(x);

//  werror("%O\n",x);

  ParseBlock tmp=ParseBlock(x,"");
  x=tmp->code;
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
  x=recursive(replace,x,PC.Token("OPTIMIZE",0),tmp->optfuncs);
  x=recursive(replace,x,PC.Token("DECLARATIONS",0),tmp->declarations);

  if(equal(x,tmp->code))
  {
    // No OPTIMIZE / DECLARATIONS
    // FIXME: Add our own stuff...
    // NOTA BENE: DECLARATIONS are not handled automatically
    //            on the file level
  }
  if(getenv("PIKE_DEBUG_PRECOMPILER"))
    write(PC.simple_reconstitute(x));
  else
    write(PC.reconstitute_with_line_numbers(x));
}
