#!/usr/local/bin/pike

/*
 * This script is used to process *.cmod files into *.c files, it 
 * reads Pike style prototypes and converts them into C code.
 *
 * The input can look something like this:
 *
 * PIKEFUNC int function_name (int x)
 *  attribute;
 *  attribute value;
 * {
 *   C code using 'x'.
 *
 *   RETURN x;
 * }
 *
 * All the ADD_EFUN/ADD_FUNCTION calls will be inserted instead of the
 * word INIT in your code.
 *
 * Currently, the following attributes are understood:
 *   efun;     makes this function a global constant (no value)
 *   flags;    ID_STATIC | ID_NOMASK etc.
 *   optflags; OPT_TRY_OPTIMIZE | OPT_SIDE_EFFECT etc.
 *   type;     tInt, tMix etc. use this type instead of automatically
 *             generating type from the prototype
 *
 *
 * BUGS/LIMITATIONS
 *  o Parenthesis must match, even within #if 0
 *  o Not all Pike types are supported yet
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

array(PC.Token) strip(array(PC.Token) t)
{
  array ret=({});
  foreach(t, mixed x)
    {
      if(objectp(x))
      {
	switch(x->text[0])
	{
	  case ' ':
	  case '\t':
	  case '\n':
	  case '\r':
	    continue;
	}
      }else{
	x=strip(x);
      }
      ret+=({x});
    }
  return ret;
}

string merge(array x)
{
  string ret="";
  foreach(x,x)
    ret+=arrayp(x)?merge(x):objectp(x)?x->text:x;
  return ret;
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
    case "int": return "INT32";
    case "float": return "FLOAT_NUMBER";
    case "string": return "struct pike_string *";

    case "array":
    case "multiset":
    case "mapping":

    case "object":
    case "program": return "struct "+btype+" *";

    case "function":
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
    case "mixed":  return "struct svalue *";

    default:
      werror("Unknown type %s\n",type);
      exit(1);
  }
}

mapping(string:string) parse_arg(array x)
{
  mapping ret=(["name":x[-1]]);
  ret->type=x[..sizeof(x)-2];
  ret->basetype=x[0];
  if(sizeof(ret->type/({"|"}))>1)
    ret->basetype="mixed";
  ret->ctype=cname(ret->basetype);
  return ret;
}

/* FIXME: this is not finished yet */
string convert_type(array s)
{
//  werror("%O\n",s);
  switch(objectp(s[0])?s[0]->text:s[0])
  {
    case "0": case "1": case "2": case "3": case "4":
    case "5": case "6": case "7": case "8": case "9":
      if(sizeof(s)>1 && s[1]=="=")
      {
	return sprintf("tSetvar(%s,%s)",s[0],convert_type(s[2..]));
      }else{
	return sprintf("tVar(%s)",s[0]);
      }

    case "int": return "tInt";
    case "float": return "tFlt";
    case "string": return "tStr";
    case "array": 
      if(sizeof(s)<2) return "tArray";
      return "tArr("+convert_type(s[1][1..sizeof(s[1])-2])+")";
    case "multiset": 
      if(sizeof(s)<2) return "tMultiset";
      return "tSet("+convert_type(s[1][1..sizeof(s[1])-2])+")";
    case "mapping": 
      if(sizeof(s)<2) return "tMapping";
      return "tMap("+
	convert_type((s[1]/({":"}))[0])+","+
	  convert_type((s[1]/({":"}))[1])+")";
    case "object": 
      return "tObj";

    case "program": 
      return "tProg";
    case "function": 
      return "tFunc";
    case "mixed": 
      return "tMix";
  }
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
    data=data[pos-2]+data[pos+1];
  return data;
}

array recursive(mixed func, array data, mixed ... args)
{
  array ret=({});

  data=func(data, @args);

  foreach(data, mixed foo)
    {
      if(arrayp(foo))
      {
	ret+=({ recursive(func, foo, @args) });
      }else{
	ret+=({ foo });
      }
    }

  return ret;
}

int main(int argc, array(string) argv)
{
  mixed x;
  string file=argv[1];
  x=Stdio.read_file(file);
  x=PC.split(x);
  x=PC.tokenize(x);
  x=PC.group(x);

  x=x/({"PIKEFUN"});
  array ret=x[0];
  array addfuncs=({});

  for(int f=1;f<sizeof(x);f++)
  {
    array func=x[f];
    int p;
    for(p=0;p<sizeof(func);p++)
      if(arrayp(func[p]) && func[p][0]=="{")
	break;

    array proto=strip(func[..p-1]);
    array body=func[p];
    array rest=func[p+1..];

    p=parse_type(proto,0);
    array rettype=proto[..p-1];
    string name=proto[p]->text;
    array args=proto[p+1];

    array attr=strip(proto[p+2..]);

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

//    werror("%O\n",attributes);


    args=args[1..sizeof(args)-2]/({","});

//    werror("FIX RETURN: %O\n",body);
    body=recursive(fix_return,body, rettype[0], cname(rettype), sizeof(args)); 
    
//    werror("name=%s\n",name);
//    werror("  rettype=%O\n",rettype);
//    werror("  args=%O\n",args);

    ret+=({
      sprintf("\n#line %d %O\n",rettype[0]->line,file),
      sprintf("void f_%s(INT32 args)\n{\n",name),
    });

    args=map(args,parse_arg);

    foreach(args, mapping arg)
      ret+=({  sprintf("%s%s;\n",arg->ctype, arg->name) });


    addfuncs+=({
      sprintf("  %s(%O,f_%s,tFunc(%s,%s),%s);\n",
	      attributes->efun ? "ADD_EFUN" : "ADD_FUNCTION",
	      name,
	      name,
	      attributes->type ? attributes->type :
	      Array.map(args->type,convert_type)*" ",
	      convert_type(rettype),
	      (attributes->efun ? attributes->optflags : 
	        attributes->flags )|| "0" ,
	      )
    });

    int argnum;

    argnum=0;
    ret+=({
      sprintf("if(args != %d) wrong_number_of_args_error(%O,args,%d);\n",
	      sizeof(args),
	      name,
	      sizeof(args))
    });

    int sp=-sizeof(args);
    foreach(args, mapping arg)
      {
	if(arg->basetype != "mixed")
	{
	  ret+=({
	    sprintf("if(sp[%d].type != PIKE_T_%s)\n",
		    sp,upper_case(arg->basetype->text)),
	    sprintf("  SIMPLE_BAD_ARG_ERROR(%O,%d,%O);\n",
		    name,
		    argnum+1,
		    merge(arg->type)),
	  });
	}

	switch(objectp(arg->basetype) ? arg->basetype->text : arg->basetype )
	{
	  case "int":
	    ret+=({
	      sprintf("%s=sp[%d].integer;\n",
		      arg->name,
		      sp)
	    });
	    break;

	  case "float":
	    ret+=({
	      sprintf("%s=sp[%d].float_number;\n",
		      arg->name,
		      sp)
	    });
	    break;

	  case "mixed":
	    ret+=({
	      sprintf("%s=sp%+d;\n",
		      arg->name,
		      sp)
	    });
	    break;

	  default:
	    ret+=({
	      sprintf("%s=sp[%d].u.%s;\n",
		      arg->name,
		      sp,
		      arg->basetype)
	    });
	  
	}
        argnum++;
	sp++;
      }
    
    if(sizeof(body))
    {
      ret+=({
	sprintf("#line %d %O\n",body[0]->line,file),
	body,
      });
    }
    ret+=({
      "}\n"
    });

    if(sizeof(rest))
    {
      ret+=({
	sprintf("#line %d %O\n",rest[0]->line,file),
      })+
	recursive(replace,rest,PC.Token("INIT",0),addfuncs);
    }
      
  }


  write(merge(ret));
}
