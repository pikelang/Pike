#!/usr/local/bin/pike
/*
   C(...)=callback function
   V=void
   I=int
   D=double
   F=float
   U=unsigned int/int
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

   #     = like =, but add count
   !     = like =, but no vector version available

   image object support:

   w     = width of image
   h     = height of image
   f     = format of image
   t     = type of image
   i     = image data

*/

/* FIXME: The callback *_fun svalues ought to be registred with the
 *        garbage_collector, and cleaned up on exit.
 */

#include "features.pike";
#include "constants.pike";

void error(string msg, mixed ... args)
{
  if(sizeof(args))
    msg = sprintf(msg, @args);
  werror(msg+"\n");
  exit(1);
}

array(string|array(string)) special_234(int mi, int mx, string ty)
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
  for(i=0; i<mx; i++) {
    string t = baset;
    if(!i)
      t+="|array("+baset+")";
    if(i>=mi || i>0)
      t+="|void";
    typ+=({t});
  }
  return ({typ,tm,rows((['i':"ZT_INT",'f':"ZT_FLOAT",'d':"ZT_DOUBLE"]),
		       values(rt))*"|",rt});
}


array(string) gen_func(string name, string ty)
{
  string res="", got="", prot, vdec, vret, fu=name;
  array novec, args=({}), argt=({});
  int r234, argt_cut=-1, img_obj=0;
  string rtypes;
  string callback,after_variables;

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
  case 'F':
    prot=":float";
    vdec="float";
    vret="push_float";
    break;
  default:
    error("%s: Unknown return type '%c'.", name, ty[0]);
  }

  res += "static void f_"+name+"(INT32 args)\n{\n"+
    (vdec?("  "+vdec+" res;\n"):"");

  int a=1;
  for(int i=1; i<sizeof(ty); i++)
  {
    switch(ty[i]) {

    case 'C':
      string internal_args;
      sscanf(ty[i..],"C(%s)",internal_args);
      if(!args)
	error("First character after C must be '(' (%s).",ty);

      callback=
	"struct svalue "+name+"_fun;\n\n"
	"void "+name+"_cb_wrapper(";
      int n;
      array a1=({}),a2=({});;
      if(!sizeof(internal_args))
	a1+=({"void"});
      else
	foreach(internal_args/"",string s)
	  switch(s) {
	  case "I":
	    a1 += ({ "int arg"+(++n) });
	    a2 += ({ "  push_int(arg"+n+");" });
	    break;
	  case "U":
	    a1 += ({ "unsigned int arg"+(++n) });
	    a2 += ({ "  push_int((int)arg"+n+");" });
	    break;
	  }
      callback += a1*", " +")\n{\n";
      callback += a2*"\n";
      callback += "\n  apply_svalue(&"+name+"_fun, "+sizeof(internal_args)+");\n";
      callback += "  pop_stack();\n";
      callback += "}";

      argt += ({"function"});
      args += ({ "(void *)&"+name+"_cb_wrapper" });

      after_variables =
	"  if("+name+"_fun.type)\n"
	"    free_svalue(&"+name+"_fun);\n"
	"  assign_svalue_no_free(&"+name+"_fun, &Pike_sp[-args+"+(a-1)+"]);\n";

      i+=sizeof(internal_args)+2;
      break;

    case 'S':
      argt += ({"string"});
      args += ({ "arg"+a });
      res += "  char *arg"+a+";\n";
      got += "  arg"+a+"=Pike_sp["+(a-1)+"-args].u.string->str;\n";
      a++;
      break;

      
    case 'U':
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
    case 'R':
      argt += ({"float"});
      args += ({ "arg"+a });
      res += "  double arg"+a+";\n";
      got += "  arg"+a+"=(double)Pike_sp["+(a-1)+"-args].u.float_number;\n";
      a++;
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

    default:
      error("%s: Unknown parameter type '%c'.", name, ty[i]);
    }
}

  if(img_obj) {
    argt += ({"object"});
    res += "  struct zimage img;\n";
    got += "  check_img_arg(Pike_sp["+(a-1)+"-args].u.object, &img, "+a+
      ", \""+name+"\");\n";
  }

  if(after_variables)
    res+=after_variables;
    
  prot = (argt*",")+prot;

  if(sizeof(argt))
    res += "\n  check_all_args(\""+name+"\", args, "+
      ((Array.map((argt_cut<0?argt:argt[..argt_cut-1]),
		  lambda(string t) {
		    return Array.map(t/"|",
				     lambda(string t) {
				       return "BIT_"+upper_case(t);
				     })*"|";
		  }))+
       (r234?({"BIT_MANY|BIT_MIXED|BIT_VOID"}):({}))+({"0"}))*", "+");\n";

  if(sizeof(got))
    res += "\n"+got+"\n";

  switch(r234)
  {
  case 0:
    res += (vret?"  res=":"  ")+fu+"("+(args*",")+");\n";
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
  }

  if(img_obj) {
    res += "  release_img(&img);\n";
  }
  res += "  pop_n_elems(args);\n";
  res += (vret? "  "+vret+"(res);\n":/*"  push_int(0);\n"*/"");
  res += "}\n";
  if(callback)
    res = callback+"\n\n"+res;
  return ({res,prot});
}

string gen()
{
  string res = "";
  array t = Array.transpose(func_misc);
  array fn=t[0];
  mapping ty = mkmapping(@t);
  mapping prot = ([]);
//   foreach(indices(func_cat), string cat) {
//     fn += func_cat[cat];
//     ty |= mkmapping(func_cat[cat],
// 		    rows(({cat}), allocate(sizeof(func_cat[cat]))));
//   }
  sort(fn);
  foreach(fn, string f) {
    array(string) r = gen_func(f, ty[f]);
    res += sprintf("#ifndef MISSING_%s\n", upper_case(f)) +
      r[0] +
      sprintf("#endif /* MISSING_%s */\n\n", upper_case(f));
    prot[f]=r[1];
  }
  res += "void add_auto_funcs_glut()\n{\n";
  res += "  pre_init();\n";
  foreach(fn, string f)
    res +=
      sprintf("#ifndef MISSING_%s\n", upper_case(f)) +
      "  add_function_constant(\""+f+"\", f_"+f+",\n\t\t\t\"function("+
      prot[f]+")\", OPT_SIDE_EFFECT);\n"
      "#endif\n";
  foreach(sort(indices(constants)), string co)
    res += "  add_integer_constant(\""+co+"\", "+constants[co]+", 0);\n";
  res += "  post_init();\n";
  res += "}\n";
  return res;
}

void main(int argc, array(string) argv)
{
  array(string) f = Stdio.File("stdin")->read()/"@@";
  write(({f[0],gen(),@f[1..]})*"");
}
