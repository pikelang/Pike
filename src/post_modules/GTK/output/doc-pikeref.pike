// For the up_to_date code.
inherit "split";


static string imgfile;
static mapping(string:int) ifcnt = ([]); // ImageFile count
static array all_consts;
static string imgfilename( string w )
{
  w = replace( lower_case(w), ".", "_")+"_#";
  return "images/"+w+".png";
}

// Output pike refdoc-style documentation from the full code-tree.
// Also generates nice readable 'source-code' as a bonus. :-)
static string make_example_image( string data, int toplevel )
{
  string tim = replace( imgfile, "#", (string)(++ifcnt[imgfile]));
  tim = replace( tim, "_1", "" );
  if( !file_stat( dir+"/"+tim ) )
    Process.create_process( ({"pike",
			      combine_path(__FILE__,
					   "../../make_example_image.pike"),
			      replace(data, "@@", "@"),
			      toplevel?"TOP":"SUB",
			      dir, tim
			    }))->wait();
  return ("@code{" + data + "@}\n"
	  "@xml{<image>../"+tim+"</image>@}\n");
}

static string fix_images( string data )
{
  string res = "";
  foreach( data /"\n", string d )
  {
    if( sscanf( d, "%*sTIMG:%s", d ) )
      res += make_example_image( d,1 )+"\n";
    else if( sscanf( d, "%*sIMG:%s", d ) )
      res += make_example_image( d,0 )+"\n";
    else
      res += d+"\n";
  }
  return res;
}

static string fix_const( string s )
{
  string a, b, c;
  string const_refs( string b )
  {
    array res = ({});
    foreach( all_consts, Constant c )
    {
      if( has_prefix( c->name, b ) )
	res +=({ "@["+c->pike_name()+"]"});
    }
    if( !sizeof( res ) )
      werror("Warning: CONST("+b+") did not expand to anything\n");
    return String.implode_nicely( res );
  };
  while( sscanf( s, "%sCONST(%s)%s", a, b, c ) == 3 )
    s = a+const_refs(b)+c;
  return s;
}

static string trim_xml( string what )
{
  string a, b, c;
  what = replace(what, "@", "@@");
  while( sscanf( what, "%s<br%*s>%s", a, b ) == 3 ) what = a+b;
  while( sscanf( what, "%s<p>%s", a, b ) == 2 )     what = a+b;
  while( sscanf( what, "%s</p>%s", a, b ) == 2 )    what = a+b;
  while( sscanf( what, "%s<b>%s</b>%s", a, b,c ) == 3 )
    what = a+"@b{"+b+"@}"+c;
  while( sscanf( what, "%s<i>%s</i>%s", a, b,c ) == 3 )
    what = a+"@i{"+b+"@}"+c;
  return what;
}


static string make_pike_refdoc( string pgtkdoc,
				mapping|void signals)
{
  string res =  "";
  if( !pgtkdoc || !strlen(pgtkdoc) )
    return "//!\n";

  pgtkdoc = fix_images( fix_const( trim_xml(pgtkdoc) ) );
  foreach( pgtkdoc/"\n", string s )
  {
    if( !strlen(s) )
      res += "//!\n";
    else if( s[0] == '!' )
      res += "//"+s+"\n";
    else 
      res += "//!"+s+"\n";
  }

  
  if( signals && sizeof(signals) )
  {
    res += "//!  Signals:\n";
    foreach( sort( indices(signals) ), string sig )
    {
      res += "//! @b{"+signals[sig]->pike_name()+"@}\n"+
	     make_pike_refdoc( signals[sig]->doc, 0 );
    }
  }
  return res;
}


static string module_name( Class cls )
{
  if( has_prefix( cls->name, "Gnome." ) ) return "Gnome";
  if( has_prefix( cls->name, "GDK." ) )   return "GDK";
  return "GTK";
}

static string class_name( Class cls, int|void nmn )
{
  string mn="";
  if(!nmn)
    mn = module_name( cls )+".";
  if( has_prefix( cls->name, "Gnome." ) ) return mn+cls->name[6..];
  if( has_prefix( cls->name, "GDK." ) )   return mn+cls->name[4..];
  if( has_prefix( cls->name, "GTK." ) )   return mn+cls->name[4..];
  return mn+cls->name;
}


static string make_function_doc( Function f, Class c )
{
  if( f->name == "_sprintf" || f->name == "destruct" )
    return "";

  string vtype;
  string pike_type_name( Type t )
  {
    if( t->name == "void" )
      return vtype;
    if( parent->classes[ t->name ] )
      return class_name(parent->classes[ t->name ]);
    return t->doc_type();
  };

  if( c->name == "_global" )
    vtype = "void";
  else
    vtype = c->doc_name(); // "this_program";
    
  
  string res = "\n";

  res = (f->is_static()?"static ":"")+
      pike_type_name( f->return_type||f->type )+
      " "+f->pike_name()+"( ";
  int i,j;
  if( f->arg_types )
    for( int i = 0; i<sizeof(f->arg_types); i++ )
    {
      if( j != 0 && f->arg_types[i]->name != "null" )
        res+= ", ";
      if( f->arg_types[i]->name != "null" )
      {
	j++;
	string t = pike_type_name(f->arg_types[i]);
	if ((< "callback", "callback|void" >)[t]) {
	  if (t == "callback") {
	    res += "function "+f->arg_names[i] + "_cb, " +
	      "mixed " + f->arg_names[i] + "_arg";
	  } else {
	    res += "function|void "+f->arg_names[i] + "_cb, " +
	      "mixed|void " + f->arg_names[i] + "_arg";
	  }
	} else {
	  res += t+" "+f->arg_names[i];
	}
      }
    }
  if( j ) res += " ";
  res += ");";
  res += "\n";
  imgfile=imgfilename(c->name+"_"+f->name);
  if( !f->doc || !strlen( f->doc ) )
  {
    werror("Warning:"+f->file+":"+f->line+": "
	   +c->name+"->"+f->name+" not documented\n" );
    res += "//!\n";
  }
  else
    res += make_pike_refdoc( f->doc, 0 );
  //  res += "{\n  // defined in\n  // "+f->file+":"+f->line+"\n}";
  return res;
}

static void output_class( Class cls, int lvl )
{
  if( mixed e = catch {
  string result = "";
  array functions = ({});
  imgfile=imgfilename(cls->name);
  if( !cls->doc || !strlen( cls->doc ) )
    werror("Warning:"+cls->file+":"+cls->line+": "
	   +cls->name+" not documented\n" );

  result =  make_pike_refdoc( cls->doc, cls->signals );

  if( cls->inherits )
    result += "\ninherit "+cls->inherits->doc_name()+";\n\n";
  else
    result += "\n";

  foreach( indices( cls->functions ), string fun )
    functions += ({({ cls->functions[ fun ]->pike_name(),
                    make_function_doc( cls->functions[fun],cls ) })});
  
  foreach( indices( cls->members ), string fun )
    functions += ({({ cls->members[fun]->pike_name(),
                    make_function_doc( cls->members[fun],cls ) })});
  
  sort( functions );
  result += column( functions, 1 )*"\n";
  if( cls->pike_name() == "_global" )
  {
    Stdio.mkdirhier( dir + "GTK.pmod/" );
    write_file(  dir + "GTK.pmod/module.pmod",
                 "inherit GTKSupport;\n\n"+
                 constants+result );
    constants="";
  }
  else
  {
    Stdio.mkdirhier( dir + ""+module_name(cls)+".pmod/" );
    write_file(  dir + ""+module_name(cls)+".pmod/"+
                 class_name(cls,1)+".pike",result );
  }
  })
  {
    if( stringp( e ) )
      werror(e+"\n");
    else
      throw( e );
  };
}

string constants="";
static void output_constant( Constant c )
{
  imgfile=imgfilename("const_"+c->name);
  constants += "constant "+c->pike_name()+";\n"+
            make_pike_refdoc( c->doc, 0 )+"\n\n";
}

array(string) output( mapping(string:Class) classes,
                      mapping(string:Constant) constants,
                      array(Node) global_code )
{
  all_consts = values(constants);
  foreach( sort(indices(constants)), string c )
    output_constant( constants[ c ] );
  traverse_class_tree( classes, output_class );
}
