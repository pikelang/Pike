#!/usr/local/bin/pike
/*
   V=void
   I=int
   D=double
   F=float
   E=enum
   B=bitfield
   O=bool
   R=double/float
   Q=int/float
   Z=(byte/)double/float/int/short

   +XXXX = 3/4/array(3/4)
   +XXX  = 2/3/4/array(2/3/4)
   +XX   = 1/2/array(1/2)
   +X    = 1/2/3/4/array(1/2/3/4)
   =XXXX = 4/array(4)
   =XXX  = 3/array(3)
   @X    = 1/array(n)
   ]X    = array() (No size limit)
   [nnX  = array() (Limited to nn elements)

   #     = like =, but add count
   !     = like =, but no vector version available

   image object support:

   w     = width of image
   h     = height of image
   f     = format of image
   t     = type of image
   i     = image data

*/

inherit "features";
inherit "constants";

void error(string msg, mixed ... args)
{
  if(sizeof(args))
    msg = sprintf(msg, @args);
  werror(msg+"\n");
  exit(1);
}

/*
 *! Generate an array with information about how the function may
 *! be called. This function is used to generate prototype information
 *! for functions which may accept 1/2/3/4/array() parameters.
 *!
 *! @arg mi
 *! Minimum number of arguments which should be accepted. Only the mi first
 *! arguments will be required, the rest may be void.
 *!
 *! @arg mx
 *! Maximum number of arguments which should be accepted.
 *!
 *! @arg a
 *! If 0, output allows for non-arrays.
 *! If 1, output only allows for arrays.
 *!
 *! @arg ty
 *! Argument type, one of E, B, I, O, D, F, R, Z, Q.
 *!
 *! @returns
 *! array(array(string), string, string, string) where
 *! the first element is an array containing the prototype for
 *! each paramter for this function.
 *!
 *! The second element is some strange thing.
 *!
 *! The third element is the types of values that may be passed
 *! as parameters to the function. This is used to check input to
 *! the function.
 *!
 *! The fourth element describes, as a string, what types of input the
 *! function accepts. This value is used to choose which lowlevel GL function
 *! to call.
 */
array(string|array(string)) special_234(int mi, int mx, string ty, int|void a)
{
  string tm="BIT_FLOAT|BIT_INT", baset="float|int", rt="if";
  array(string) typ=({});
  int i, ad;
  if(sizeof(Array.uniq(values(ty)))!=1)
    error("Unparsable 234 type '%s'.", ty);
  switch(ty[0]) {
  case 'E':
  case 'B':
  case 'I':
  case 'O':
    baset="int";
    tm="BIT_INT";
    rt="i";
    break;
  case 'D':
    ad=1;  /* fallthrough */
  case 'F':
    baset="float";
    tm="BIT_FLOAT";
    rt=(ad?"":"f");
    break;
  case 'R':
    rt="f";
  case 'Z':
    ad=1;
    break;
  case 'Q':
    break;
  default:
    error("Unknown 234 type '%s'.", ty[0..0]);
  }
  if(ad)
    rt+="d";
  if(a)
    typ+=({"array("+baset+")"});
  else for(i=0; i<mx; i++) {
    string t = baset;
    if(!i)
      t+="|array("+baset+")";
    if(i>=mi || i>0)
      t+="|void";
    typ+=({t});
  }
  array ret = ({typ,tm,rows((['i':"ZT_INT",'f':"ZT_FLOAT",'d':"ZT_DOUBLE"]),
		       values(rt))*"|",rt});
  return ret;
}

array(string) gen_func(string name, string ty)
{
  string res="", got="", prot, vdec, vret, vcast, fu=name;
  array novec, args=({}), argt=({});
  int r234, argt_cut=-1, img_obj=0, polya=-1;
  string rtypes;

  switch(ty[0]) {
  case 'V':
    prot=":void";
    break;
  case 'I':
  case 'O':
  case 'E':
    prot=":int";
    vdec="INT32";
    vret="push_int";
    break;
  case 'S':
    prot=":string";
    vdec="const GLubyte *";
    vret="push_text";
    vcast="(char *)";
    break;
  default:
    error("%s: Unknown return type '%c'.", name, ty[0]);
  }

  res += "static void f_"+name+"(INT32 args)\n{\n"+
    (vdec?("  "+vdec+" res;\n"):"");

  int a=1;
  for(int i=1; i<sizeof(ty); i++)
    switch(ty[i]) {
    case 'B':
    case 'E':
    case 'O':
    case 'I':
      argt += ({"int"});
      args += ({ "arg"+a });
      res += "  INT32 arg"+a+";\n";
      got += "  arg"+a+"=Pike_sp["+(a-1)+"-args].u.integer;\n";
      a++;
      break;
    case 'D':
      argt += ({"float"});
      args += ({ "arg"+a });
      res += "  double arg"+a+";\n";
      got += "  arg"+a+"=Pike_sp["+(a-1)+"-args].u.float_number;\n";
      a++;
      break;
    case 'F':
      argt += ({"float"});
      args += ({ "arg"+a });
      res += "  float arg"+a+";\n";
      got += "  arg"+a+"=Pike_sp["+(a-1)+"-args].u.float_number;\n";
      a++;
      break;
    case 'Z':
      argt += ({"float|int"});
      args += ({ "Pike_sp["+(a-1)+"-args]" });
      polya = (a++)-1;
      break;
    case '+':
      int mi, mx;
      switch(sizeof(ty[i+1..])) {
      case 1:
	mi = 1; mx = 4; break;
      case 2:
	mi = 1; mx = 2; break;
      case 3:
	mi = 2; mx = 4; break;
      case 4:
	mi = 3; mx = 4; break;
      default:
	error("Can't understand + followed by %d chars.\n", sizeof(ty[i+1..]));
      }
      array plusfix = special_234(mi, mx, ty[i+1..]);
      res += "  struct zvalue4 zv4;\n";
      argt_cut = sizeof(argt);
      argt += plusfix[0];
      res += "\n  int r234=check_234_args(\""+name+"\", args-"+(a-1)+", "+
	mi+", "+mx+", "+plusfix[1]+", "+plusfix[2]+", &zv4);\n";
      r234=1;
      rtypes=plusfix[3];
      i=sizeof(ty);
      novec=indices(allocate(mx+1))[mi..];
      break;
    case '#':
      fu += sizeof(ty[i+1..]);
      /* fallthrough */
    case '!':
    case '=':
      array eqfix = special_234(sizeof(ty[i+1..]), sizeof(ty[i+1..]),
				ty[i+1..]);
      res += "  struct zvalue4 zv4;\n";
      argt_cut = sizeof(argt);
      argt += eqfix[0];
      res += "\n  check_234_args(\""+name+"\", args-"+(a-1)+", "+
	sizeof(ty[i+1..])+", "+sizeof(ty[i+1..])+", "+eqfix[1]+", "+
	eqfix[2]+", &zv4);\n";
      r234=2;
      rtypes=eqfix[3];
      if(ty[i]=='!')
	novec=indices(ty[i+1..]);
      i=sizeof(ty);
      break;
    case '@':
      array atfix = special_234(1, 1, ty[i+1..]);
      res += "  union zvalue16 zv16;\n";
      argt_cut = sizeof(argt);
      argt += atfix[0];
      res += "\n  int r1n=check_1n_args(\""+name+"\", args-"+(a-1)+", "+
	atfix[1]+", "+atfix[2]+", &zv16);\n";
      r234=3;
      rtypes=atfix[3];
      i=sizeof(ty);
      break;
    case '[':
      int nn;
      string rst;
      sscanf(ty[i+1..], "%d%s", nn, rst);
      array arrfix = special_234(nn, nn, rst, 1);
      res += "  union zvalue16 zv16;\n  int r1n;\n";
      argt += arrfix[0];
      got += "  if(Pike_sp["+(a-1)+"-args].u.array->size != "+nn+")\n"
	     "    Pike_error(\""+name+": Array length is wrong (is %d, should be "+nn+")\\n\", Pike_sp["+(a-1)+"-args].u.array->size);\n\n";
      got += "  r1n=check_1n_args(\""+name+"\", args-"+(a-1)+", "+
	arrfix[1]+", "+arrfix[2]+", &zv16);\n";
      r234=-1;
      rtypes=arrfix[3];
      i=sizeof(ty);
      break;
    case ']':
      arrfix = special_234(0, 0, ty[i+1..], 1);
      res += "  union zvalue *zv;\n  int r1n;\n";
      argt += arrfix[0];
      got += "  r1n=check_1unlimited_args(\""+name+"\", args-"+(a-1)+", "+
	arrfix[1]+", "+arrfix[2]+", &zv);\n";
      r234=-2;
      rtypes=arrfix[3];
      i=sizeof(ty);
      break;
    case 'w':
      img_obj=1;
      args += ({ "img.width" });
      break;
    case 'h':
      img_obj=1;
      args += ({ "img.height" });
      break;
    case 'f':
      img_obj=1;
      args += ({ "img.format" });
      break;
    case 't':
      img_obj=1;
      args += ({ "img.type" });
      break;
    case 'i':
      img_obj=1;
      args += ({ "img.pixels" });
      break;
    case '&':
      argt += ({"object"});
      args += ({ "get_mem_object(Pike_sp+("+(a-1)+"-args))" });
      break;
    default:
      error("%s: Unknown parameter type '%c'.", name, ty[i]);
    }

  if(img_obj) {
    argt += ({"object|mapping(string:object)"});
    res += "  struct zimage img;\n";
    got += "  check_img_arg(&Pike_sp["+(a-1)+"-args], &img, "+a+
      ", \""+name+"\");\n";
  }

  prot = (argt*",")+prot;

  if(sizeof(argt))
    res += "\n  check_all_args(\""+name+"\", args, "+
      ((Array.map((argt_cut<0?argt:argt[..argt_cut-1]),
		  lambda(string t) {
		    string t2;
		    while(3==sscanf(t, "%s(%*s)%s", t, t2))
		      t+=t2;
		    return Array.map(t/"|",
				     lambda(string t) {
				       return "BIT_"+upper_case(t);
				     })*"|";
		  }))+
       (r234>0?({"BIT_MANY|BIT_MIXED|BIT_VOID"}):({}))+({"0"}))*", "+");\n";

  if(sizeof(got))
    res += "\n"+got+"\n";

  switch(r234)
  {
  case 0:
    if(polya<0)
      res += (vret?"  res=":"  ")+fu+"("+(args*",")+");\n";
    else
      res += "  switch("+args[polya]+".type) {\n"+
	Array.map(argt[polya]/"|", lambda(string t) 
				   {
				     array(string) a = copy_value(args);
				     a[polya] += ".u";
				     switch(t) {
				     case "int":
				       a[polya]+=".integer"; break;
				     case "float":
				       a[polya]+=".float_number"; break;
				     }
				     return "    case T_"+upper_case(t)+": "+
				       (vret?"res=":"")+fu+t[0..0]+"("+
				       (a*",")+"); break;\n";
				   })*""+"  }\n";
    break;
  case 1:
    if(sizeof(rtypes)==1)
      res += "  switch(r234) {\n"+Array.map(novec, lambda(int i, string vret,
							  string r, string fu,
							  array(string) args) {
						     return "    case "+i+": "+
						       (vret?"res=":"")+
						       fu+i+"v("+
						       (args+({"zv4.v."+r}))*
						       ","+"); break;";
						   }, vret, rtypes, fu, args)*
	"\n"+"\n  }\n";
    else
      res += "  switch(zv4.ty) {\n"+
	Array.map(indices(rtypes),
		  lambda(int i, string r, string fu, string vret,
			 array(string) args, array(int) novec) {
		    return "    case ZT_"+
		    (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+
		      ": switch(r234) {\n"+
		      Array.map(novec, lambda(int i, string vret, string r,
					      string fu, array(string) args) {
					 return "      case "+i+": "+
					   (vret?"res=":"")+
					   fu+i+r+"v("+
					   (args+({"zv4.v."+r}))*
					   ","+"); break;";
				       }, vret, r[i..i], fu, args)*
		      "\n"+"\n    }\n    break;";
		  }, rtypes, fu, vret, args, novec)*"\n"+
	"\n  }\n";
    break;
  case 2:
    if(sizeof(rtypes)==1)
      res += (vret?"  res=":"  ")+fu+"("+
	((args+(novec? Array.map(novec, lambda(int i, string rtypes) {
					  return "zv4.v."+rtypes+"["+i+"]";
					}, rtypes):({"zv4.v."+rtypes})))*",")+
	");\n";
    else
      res += "  switch(zv4.ty) {\n"+
	Array.map(indices(rtypes),
		  lambda(int i, string r, string fu, string vret,
			 array(string) args, array(int) novec) {
		    return "    case ZT_"+
		    (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+": "+
		      (vret?"res=":"")+fu+r[i..i]+(novec?"(":"v(")+
		      ((args+(novec? Array.map(novec, lambda(int i, string r) {
							return "zv4.v."+r+
							  "["+i+"]";
						      }, r[i..i]):
			      ({"zv4.v."+r[i..i]})))*",")+"); break;";
		  }, rtypes, fu, vret, args, novec)*"\n"+
	"\n  }\n";
    break;
  case 3:
    if(sizeof(rtypes)==1)
      res += "  if(r1n&ZT_ARRAY)\n"+
	(vret?"    res=":"    ")+fu+"v("+(args+({"zv16."+rtypes}))*","+
	");\n  else\n"+
	(vret?"    res=":"    ")+fu+"("+(args+({"zv16."+rtypes+"[0]"}))*","+
	");\n";
    else
      res += "  switch(r1n) {\n"+
	Array.map(indices(rtypes),
		  lambda(int i, string r, string fu, string vret,
			 array(string) args, array(int) novec) {
		    return "    case ZT_"+
		    (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+": "+
		      (vret?"res=":"")+fu+r[i..i]+"("+
		      ((args+({"zv16."+r[i..i]+"[0]"}))*",")+"); break;\n"+
		      "    case ZT_ARRAY|ZT_"+
		    (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+": "+
		      (vret?"res=":"")+fu+r[i..i]+"v("+
		      ((args+({"zv16."+r[i..i]}))*",")+"); break;";
		  }, rtypes, fu, vret, args, novec)*"\n"+
	"\n  }\n";
    break;
  case -1:
    res += "  switch(r1n) {\n"+
      Array.map(indices(rtypes),
		lambda(int i, string r, string fu, string vret,
		       array(string) args, array(int) novec) {
		  return "    case ZT_ARRAY|ZT_"+
		  (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+": "+
		    (vret?"res=":"")+fu+r[i..i]+"("+
		    ((args+({"zv16."+r[i..i]}))*",")+"); break;";
		}, rtypes, fu, vret, args, novec)*"\n"+
      "\n  }\n";
    break;

  case -2:
    res += "  switch(r1n) {\n"+
      Array.map(indices(rtypes),
		lambda(int i, string r, string fu, string vret,
		       array(string) args, array(int) novec) {
		  return "    case ZT_ARRAY|ZT_"+
		  (['i':"INT",'f':"FLOAT",'d':"DOUBLE"])[r[i]]+": "+
		    (vret?"res=":"")+fu+r[i..i]+"("+
		    ((args+({ "(void *)zv" }))*",")+"); break;";
		}, rtypes, fu, vret, args, novec)*"\n"+
      "\n  }\n";
    res += "  if (zv != NULL)\n    free(zv);\n";
    break;
  }

  if(img_obj) {
    res += "  release_img(&img);\n";
  }
  res += "  pop_n_elems(args);\n";
  res += (vret? "  "+vret+"("+(vcast||"")+"res);\n":/*"  push_int(0);\n"*/"");
  res += "}\n\n";
  return ({res,prot});
}

string gen()
{
  string res = "";
  array t = Array.transpose(func_misc);
  array fn=t[0];
  mapping ty = mkmapping(@t);
  mapping prot = ([]);
  foreach(indices(func_cat), string cat) {
    fn += func_cat[cat];
    ty |= mkmapping(func_cat[cat],
		    rows(({cat}), allocate(sizeof(func_cat[cat]))));
  }
  sort(fn);
  foreach(fn, string f) {
    array(string) r = gen_func(f, ty[f]);
    res += r[0];
    prot[f]=r[1];
  }
  res += "void GL_add_auto_funcs()\n{\n";
  res += "  pre_init();\n";
  foreach(fn, string f)
    res += "  add_function_constant(\""+f+"\", f_"+f+",\n\t\t\t\"function("+
      prot[f]+")\", OPT_SIDE_EFFECT);\n";
  foreach(sort(indices(constants)), string co) {
    int val = constants[co];
    if (val >= 1 << 31)
      res += sprintf (#"\
#ifdef INT64
  push_int64(%dLL);
#else
  push_int(%d);
  push_int(31);
  f_lsh(2);
  push_int(%d);
  f_or(2);
#endif
  simple_add_constant(%O, Pike_sp-1, 0);
  pop_stack();
",
		      val, val >> 31, val & ((1 << 31) - 1), co);
    else
      res += "  add_integer_constant(\""+co+"\", "+
	(string)val+", 0);\n";
  }
  res += "  post_init();\n";
  res += "}\n";
  return res;
}

void main(int argc, array(string) argv)
{
  array(string) f = Stdio.File("stdin")->read()/"@@";
  write(({
    "#line 1 \"auto.c.in\"\n",
    f[0],
    sprintf("\n#line %d \"auto.c (generated by %s)\"\n",
	    String.count (f[0], "\n") + 4, __FILE__),
    gen(),
    sprintf("\n#line %d \"auto.c.in\"\n", sizeof(f[0]/"\n")),
    @f[1..]})*"");
}
