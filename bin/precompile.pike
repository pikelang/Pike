#!/usr/local/bin/pike

/*
 * This script is used to process *.cmod files into *.c files, it 
 * reads Pike style prototypes and converts them into C code.
 *
 * The input can look something like this:
 *
 * PIKECLASS fnord 
 *  attributes;
 * {
 *
 *   PIKEFUNC int function_name (int x, CTYPE char * foo)
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
 * Currently, the following attributes are understood:
 *   efun;     makes this function a global constant (no value)
 *   flags;    ID_STATIC | ID_NOMASK etc.
 *   optflags; OPT_TRY_OPTIMIZE | OPT_SIDE_EFFECT etc.
 *   type;     tInt, tMix etc. use this type instead of automatically
 *             generating type from the prototype
 *   errname;  The name used when throwing errors.
 *   name;     The name used when doing add_function.
 *
 *
 * BUGS/LIMITATIONS
 *  o Parenthesis must match, even within #if 0
 *  o Not all Pike types are supported yet.
 *  o No support for functions that takes variable number of arguments yet.
 *  o No support for class variables yet.
 *  o No support for set_init/exit/gc_mark/gc_check_callback.
 *  o No support for 'char *', 'double'  etc.
 *  o RETURN; (void) doesn't work yet
 *  o need a RETURN_NULL; or something.. RETURN 0; might work but may
 *    be confusing as RETURN x; will not work if x is zero.
 *    
 */

#define PC Parser.C

#ifdef OLD
import ".";
#define PC .C
#endif

int parse_type(array x, int pos)
{
  while(1)
  {
    while(x[pos]=="!") pos++;
    pos++;
    if(arrayp(x[pos])) pos++;
    switch(x[pos])
    {
      default:
	return pos;

      case "&":
      case "|":
	pos++;
    }
  }
}

string merge(array x)
{
  return PC.simple_reconstitute(x);
}

string trim(string s)
{
  sscanf(s,"%*[ \t]%s",s);
  s=reverse(s);
  sscanf(s,"%*[ \t]%s",s);
  s=reverse(s);
  return s;
}

string cname(mixed type)
{
  mixed btype;
  if(arrayp(type))
    btype=type[0];
  else
    btype=type;

  switch(objectp(btype) ? btype->text : btype)
  {
    case "int": return "INT_TYPE";
    case "float": return "FLOAT_NUMBER";
    case "string": return "struct pike_string *";

    case "array":
    case "multiset":
    case "mapping":

    case "object":
    case "program": return "struct "+btype+" *";

    case "function":

    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9":
    case "mixed":  return "struct svalue *";

    default:
      werror("Unknown type %s\n",objectp(btype) ? btype->text : btype);
      exit(1);
  }
}

string uname(mixed type)
{
  switch(objectp(type) ? type->text : type)
  {
    case "int": return "integer";
    case "float":
    case "string":

    case "array":
    case "multiset":
    case "mapping":

    case "object":
    case "program": return "struct "+type+" *";

    case "function":
    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9":
    case "mixed":  return "struct svalue *";

    default:
      werror("Unknown type %s\n",type);
      exit(1);
  }
}

array convert_ctype(array tokens)
{
  switch((string)tokens[0])
  {
    case "char": /* char* */
      if(sizeof(tokens) >1 && "*" == (string)tokens[1])
	return ({ PC.Token("string") });

    case "short":
    case "int":
    case "long":
    case "size_t":
    case "ptrdiff_t":
    case "INT32":
      return ({ PC.Token("int") });

    case "double":
    case "float":
      return ({ PC.Token("float") });

    default:
      werror("Unknown C type.\n");
      exit(0);
  }
}

mapping(string:string) parse_arg(array x)
{
  mapping ret=(["name":x[-1]]);
  ret->type=x[..sizeof(x)-2];
  ret->basetype=x[0];
  if(ret->basetype == PC.Token("CTYPE"))
  {
    ret->cname = merge(ret->type[1..]);
    ret->type = convert_ctype(ret->type[1..]);
    ret->is_c_type=1;
  }else{
    array ored_types=ret->type/({"|"});
    if(sizeof(ored_types)>1)
      ret->basetype="mixed";
    
    if(search(ored_types,PC.Token("void"))!=-1)
      ret->optional=1;
    
    ret->ctype=cname(ret->type);
  }
  ret->typename=trim(merge(recursive(strip_type_assignments,ret->type)));
  ret->line=ret->name->line;
  return ret;
}

/* FIXME: this is not finished yet */

array(string|int) convert_basic_type(array s, int pos)
{
  string res;

  switch(objectp(s[pos])?s[pos]->text:s[pos])
  {
    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9":
      if(sizeof(s)>(pos+1) && s[pos+1]=="=")
      {
	res = sprintf("tSetvar(%s,%s)",s[pos],convert_type(s[pos+2..]));
	pos += 3;
      }else{
	res = sprintf("tVar(%s)",s[pos]);
	pos++;
      }
      break;

    case "int":
      if ((sizeof(s) < pos + 2) || !arrayp(s[pos+1])) {
	res = "tInt";
	pos++;
      } else {
	array tmp = s[pos + 1];
	pos += 2;

	int pos2 = 1;
	int low = -0x20000000;
	if (tmp[pos2] != "..") {
	  low = (int)(tmp[pos2]->text);
	  pos2++;
	  // The second is a workaround for a bug in Parser.Pike.
	  if ((tmp[pos2] != "..") && (tmp[pos2] != ".. ")) {
	    error(sprintf("Syntax error in int range got %O, "
			  "expected \"..\".\n",
			  tmp));
	  }
	}
	pos2++;
	int high = 0x1fffffff;
	if (tmp[pos2] != ")") {
	  high = (int)(tmp[pos2]->text);
	  pos2++;
	  if (tmp[pos2] != ")") {
	    error(sprintf("Syntax error in int range got %O, "
			  "expected \")\".\n",
			  tmp));
	  }
	}
	// NOTE! This piece of code KNOWS that PIKE_T_INT is 8!
	res = sprintf("%O", sprintf("\8%4c%4c", low, high));
      }
      break;

    case "float":
      res = "tFlt";
      pos++;
      break;

    case "string":
      res = "tStr";
      pos++;
      break;

    case "array": 
      if((sizeof(s) < pos + 2) || !arrayp(s[pos + 1])) {
	res = "tArray";
	pos++;
      } else {
	res = "tArr("+convert_type(s[pos+1][1..sizeof(s[pos+1])-2])+")";
	pos += 2;
      }
      break;

    case "multiset": 
      if((sizeof(s) < pos + 2) || !arrayp(s[pos + 1])) {
	res = "tMultiset";
	pos++;
      } else {
	res = "tSet("+convert_type(s[pos+1][1..sizeof(s[pos+1])-2])+")";
	pos += 2;
      }
      break;

    case "mapping": 
      if((sizeof(s) < pos + 2) || !arrayp(s[pos + 1])) {
	res = "tMapping";
	pos ++;
      } else {
	mixed tmp=s[pos + 1][1..sizeof(s[pos + 1])-2];
	res = "tMap("+
	  convert_type((tmp/({":"}))[0])+","+
	  convert_type((tmp/({":"}))[1])+")";
	pos += 2;
      }
      break;
    
    case "object": /* FIXME: support object is/implements */
      res =  "tObj";
      pos++;
      break;

    case "program": 
      res = "tProgram";
      pos++;
      break;

    case "function": 
      if((sizeof(s) < pos + 2) || !arrayp(s[pos + 1])) {
	res = "tFunction";
	pos++;
      } else {
	array args=s[pos + 1][1..sizeof(s[pos + 1])-2];
	[ args, array ret ] = args / PC.Token(":");
	if(args[-1]=="...")
	{
	  res = sprintf("tFunc(%s,%s)",
			map(convert_type,args)*" ",
			convert_type(ret));
	}else{
	  res = sprintf("tFuncV(%s,,%s)",
			map(convert_type,args[..sizeof(args)-2])*" ",
			convert_type(args[-1]),
			convert_type(ret));
	}
	pos += 2;
      }
      break;

    case "mixed": 
      res = "tMix";
      pos++;
      break;

    default:
      res = sprintf("ERR%O",s);
      pos++;
      break;
  }  
  return ({ res, pos });
}

array(string|int) low_convert_type(array s, int pos)
{
  [string res, pos] = convert_basic_type(s, pos);
  while (pos < sizeof(s)) {
    if (s[pos] == "|") {
      [string res2, pos] = low_convert_type(s, ++pos);
      res = "tOr(" + res + "," + res2 + ")";
    } else if (s[pos] == "&") {
      [string res2, pos] = convert_basic_type(s, ++pos);
      res = "tAnd(" + res + "," + res2 + ")";
    }
  }
  return ({ res, pos });
}

string convert_type(array s)
{
  // werror("%O\n",s);

  [string ret, int pos] = low_convert_type(s, 0);
  if (pos != sizeof(s)) {
    error(sprintf("Failed to convert type: %{%O, %}, pos: %O, partial:%O\n",
		  s, pos, ret));
  }
  return ret;
}


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
array fix_return(array body, string rettype, string ctype, mixed args)
{
  int pos=0;
  
  while( (pos=search(body,PC.Token("RETURN",0),pos)) != -1)
  {
    int pos2=search(body,PC.Token(";",0),pos+1);
    body[pos]=sprintf("do { %s ret_=(",ctype);
    body[pos2]=sprintf("); %s push_%s(ret_); return; }while(0);",make_pop(args),rettype);
    pos=pos2+1;
  }

  pos=0;
  while( (pos=search(body,PC.Token("REF_RETURN",0),pos)) != -1)
  {
    int pos2=search(body,PC.Token(";",0),pos+1);
    body[pos]=sprintf("do { %s ret_=(",ctype);
    body[pos2]=sprintf("); add_ref(ret_); %s push_%s(ret_); return; }while(0);",make_pop(args),rettype);
    pos=pos2+1;
  }
  return body;
}

array strip_type_assignments(array data)
{
  int pos;

  while( (pos=search(data,PC.Token("=",0))) != -1)
    data=data[..pos-2]+data[pos+1..];
  return data;
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

mapping parse_attributes(array attr)
{
  mapping attributes=([]);
  foreach(attr/ ({";"}), attr)
    {
      switch(sizeof(attr))
      {
	case 0: break;
	case 1:
	  attributes[attr[0]->text]=1;
	  break;
	default:
	  array tmp=attr[1..];
	  if(sizeof(tmp) == 1 && arrayp(tmp[0]) && tmp[0][0]=="(")
	    tmp=tmp[0][1..sizeof(tmp[0])-2];
	  
	  attributes[attr[0]->text]=merge(tmp);
      }
    }
  return attributes;
}

string file;

array convert(array x, string base)
{
  array addfuncs=({});
  array exitfuncs=({});
  array declarations=({});
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
    string name=proto[p]->text;
    mapping attributes=parse_attributes(proto[p+2..]);

    [ array classcode, array classaddfuncs, array classexitfuncs,
      array classdeclarations ]=
      convert(body[1..sizeof(body)-2],name+"_");
    ret+=({ sprintf("\n#define class_%s_defined\n",name), });
    ret+=classcode;

    addfuncs+=({
      sprintf("\n#ifdef class_%s_defined\n",name),
      "  start_new_program();\n",
      sprintf("\n#ifdef THIS_%s\n",upper_case(base)),
      sprintf("\n  %s_storage_offset=ADD_STORAGE(struct %s_struct);\n",base,base),
      sprintf("\n#endif\n"),
    })+
      classaddfuncs+
	({
	  sprintf("  end_class(%O,%d);\n",name, attributes->flags || "0"),
	  sprintf("\n#endif\n"),
	});

    exitfuncs+=({
      sprintf("\n#ifdef class_%s_defined\n",name),
    })+
      classdeclarations+
      classexitfuncs+
	({
	  sprintf("\n#endif\n"),
	});
  }


#if 0
  array thestruct=({});
  x=ret/({"PIKEVAR"});
  ret=x[0];
  for(int f=1;f<sizeof(x);f++)
  {
    array var=x[f];
    int pos=search(var,PC.Token(";",0),);
    mixed name=var[pos-1];
    array type=var[..pos-2];
    array rest=var[pos+1..];
    
    thestruct+=({
      sprintf("\n#ifdef var_%s_%s_defined\n",name,base),
      sprint("  %s %s;\n",ctype(type),name),
      "\n#endif\n",
    });
    addfuncs+=({
      sprintf("\n#ifdef var_%s_%s_defined\n",name,base),
      sprintf("  map_variable(%O,%O,%s_storage_offset + OFFSETOF(%s_struct, %s), T_%s)",
	      name,
	      merge(type),
	      base,
	      base,
	      name,
	      upper_case(basetype(type))),
      "\n#endif\n",
    });
    ret+=
      ({
	sprintf("\n#define var_%s_%s_defined\n",name,base),
      })+
      rest;
  }

  x=ret/({"CVAR"});
  ret=x[0];
  for(int f=1;f<sizeof(x);f++)
  {
    array var=x[f];
    int pos=search(var,PC.Token(";",0),);
    int npos=pos-1;
    while(arrayp(var[pos])) pos--;
    mixed name=var[npos];
    
    thestruct+=({
      sprintf("\n#ifdef var_%s_%s_defined\n",name,base),
    })+var[..pos-1]+({
      "\n#endif\n",
    });
    ret+=
      ({
	sprintf("\n#define var_%s_%s_defined\n",name,base),
      })+
      var[pos+1..];
  }


  x=ret/({"PIKEFUN"});
  ret=x[0];

  if(sizeof(thestruct))
  {
    x[0]+=
      ({
	/* FIXME:
	 * Add runtime debug to this define...
	 */
	sprintf("#define THIS_%s ((struct %s_struct *)(Pike_interpreter.frame_pointer->current_storage)\n",upper_case(base),base);
	sprintf("static int %s_storage_offset;\n",base),
	sprintf("struct %s_struct {\n",base),
      })+thestruct+({
	"};\n",
      });
  }
#else
  x=ret/({"PIKEFUN"});
  ret=x[0];
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

    p=parse_type(proto,0);
    array rettype=proto[..p-1];
    string name=proto[p]->text;
    array args=proto[p+1];

    mapping attributes=parse_attributes(proto[p+2..]);

    args=args[1..sizeof(args)-2]/({","});

//    werror("FIX RETURN: %O\n",body);
    
//    werror("name=%s\n",name);
//    werror("  rettype=%O\n",rettype);
//    werror("  args=%O\n",args);

    ret+=({
      sprintf("#define f_%s_defined\n",name),
      sprintf("PMOD_EXPORT void f_%s(INT32 args) ",name),
      "{","\n",
    });

    args=map(args,parse_arg);
    int min_args=sizeof(args);
    int max_args=sizeof(args); /* FIXME: check for ... */

    while(min_args>0 && args[min_args-1]->optional)
      min_args--;

    foreach(args, mapping arg)
      ret+=({
	PC.Token(sprintf("%s %s;\n",arg->ctype, arg->name),arg->line),
      });


    if (attributes->efun) {
      addfuncs+=({
	sprintf("\n#ifdef f_%s_defined\n",name),
	PC.Token(sprintf("  ADD_EFUN(%O, f_%s, tFunc(%s, %s), %s);\n",
			 attributes->name || name,
			 name,
			 attributes->type ? attributes->type :
			 Array.map(args->type,convert_type)*" ",
			 convert_type(rettype),
			 (attributes->efun ? attributes->optflags : 
			  attributes->flags )|| "0" ,
			 ),proto[0]->line),
	sprintf("#endif\n",name),
      });
    } else {
      addfuncs+=({
	sprintf("\n#ifdef f_%s_defined\n",name),
	PC.Token(sprintf("  ADD_FUNCTION2(%O, f_%s, tFunc(%s, %s), %s, %s);\n",
			 attributes->name || name,
			 name,
			 attributes->type ? attributes->type :
			 Array.map(args->type,convert_type)*" ",
			 convert_type(rettype),
			 attributes->flags || "0" ,
			 attributes->optflags ||
			 "OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT"
			 ),proto[0]->line),
	sprintf("#endif\n",name),
      });
    }
    int argnum;

    argnum=0;
    string argbase;
    string num_arguments;
    if(min_args == max_args)
    {
      ret+=({
	PC.Token(sprintf("if(args != %d) wrong_number_of_args_error(%O,args,%d);\n",
			 sizeof(args),
			 name,
			 sizeof(args)), proto[0]->line)
	  });
      argbase=(string) (-sizeof(args));
      num_arguments=(string)sizeof(args);
    }else{
      argbase="-args";
      num_arguments="args";
      if(min_args > 0) {
	ret+=({
	  PC.Token(sprintf("if(args < %d) wrong_number_of_args_error(%O,args,%d);\n",
			   min_args,
			   name,
			   min_args), proto[0]->line)
	    });
      }

      if(max_args != -1) {
	ret+=({
	  PC.Token(sprintf("if(args > %d) wrong_number_of_args_error(%O,args,%d);\n",
			   max_args,
			   name,
			   max_args), proto[0]->line)
	    });
      }
    }

    foreach(args, mapping arg)
      {
	if(arg->optional && "mixed" != (string)arg->basetype)
	{
	  ret+=({
	    PC.Token(sprintf("if(args >= %s) ",argnum)),
	      });
	}
	if(arg->is_c_type && arg->basetype == "string")
	{
	  /* Special case for 'char *' */
	  /* This will have to be amended when we want to support
	   * wide strings
	   */
	  ret+=({
	      PC.Token(sprintf("if(sp[%d%s].type != PIKE_T_STRING || sp[%d%s].ustring -> width)",
			       argnum,argbase,
			       argnum,argbase,
			       upper_case(arg->basetype->text)),arg->line)
	    });
	}else

	switch((string)arg->basetype)
	{
	  default:
	    ret+=({
	      PC.Token(sprintf("if(Pike_sp[%d%s].type != PIKE_T_%s)",
			       argnum,argbase,
			       upper_case(arg->basetype->text)),arg->line)
	    });
	    break;

	  case "program":
	    ret+=({
	      PC.Token(sprintf("if(!( %s=program_from_svalue(sp%+d%s)))",
			       arg->name,argnum,argbase),arg->line)
	    });
	    break;

	  case "mixed":
	}
	switch((string)arg->basetype)
	{
	  default:
	    ret+=({
	      PC.Token(sprintf(" SIMPLE_BAD_ARG_ERROR(%O,%d,%O);\n",
			       attributes->errname || attributes->name || name,
			       argnum+1,
			       arg->typename),arg->line),
	    });

	  case "mixed":
	}

	if(arg->optional)
	{
	  ret+=({
	    PC.Token(sprintf("if(args >= %s) { ",argnum)),
	      });
	}

	switch(objectp(arg->basetype) ? arg->basetype->text : arg->basetype )
	{
	  case "int":
	    ret+=({
	      sprintf("%s=Pike_sp[%d%s].u.integer;\n",arg->name,argnum,argbase)
	    });
	    break;

	  case "float":
	    ret+=({
	      sprintf("%s=Pike_sp[%d%s].u.float_number;\n",
		      arg->name,
		      argnum,argbase)
	    });
	    break;


	  case "mixed":
	    ret+=({
	      PC.Token(sprintf("%s=sp%+d%s; dmalloc_touch_svalue(sp%+d%s);\n",
			       arg->name,
			       argnum,argbase,argnum,argbase),arg->line),
	    });
	    break;

	  default:
	    if(arg->is_c_type && arg->basetype == "string")
	    {
	      /* some sort of 'char *' */
	      /* This will have to be amended when we want to support
	       * wide strings
	       */
	      ret+=({
		PC.Token(sprintf("%s=Pike_sp[%d%s].u.string->str; debug_malloc_touch(Pike_sp[%d%s].u.string)\n",
				 arg->name,
				 argnum,argbase,
				 argnum,argbase),arg->line)
		  });
	      
	    }else{
	      ret+=({
		PC.Token(sprintf("debug_malloc_pass(%s=sp[%d%s].u.%s);\n",
				 arg->name,
				 argnum,argbase,
				 arg->basetype),arg->line)
		  });
	    }

	  case "program":
	}

	if(arg->optional)
	{
	  ret+=({
	    PC.Token(sprintf("}",argnum)),
	      });
	}

        argnum++;
      }
    
    body=recursive(fix_return,body, rettype[0], cname(rettype), num_arguments); 
    if(sizeof(body))
      ret+=({body});
    ret+=({ "}\n" });

    ret+=rest;
  }


  return ({ ret, addfuncs, exitfuncs, declarations });
}

int main(int argc, array(string) argv)
{
  mixed x;
  file=argv[1];
  x=Stdio.read_file(file);
  x=PC.split(x);
  x=PC.tokenize(x,file);
  x=PC.hide_whitespaces(x);
  x=PC.group(x);

  array tmp=convert(x,"");
  x=tmp[0];
  x=recursive(replace,x,PC.Token("INIT",0),tmp[1]);
  x=recursive(replace,x,PC.Token("EXIT",0),tmp[2]);
  x=recursive(replace,x,PC.Token("DECLARATIONS",0),tmp[3]);

  if(equal(x,tmp[0]))
  {
    // No INIT / EXIT, add our own stuff..
    // NOTA BENE: DECLARATIONS are not handled automatically
    //            on the file level

    x+=({
      "void pike_module_init(void) {\n",
      tmp[1],
      "}\n",
      "void pike_module_exit(void) {\n",
      tmp[2],
      "}\n",
    });
  }
  write(PC.reconstitute_with_line_numbers(x));
}
