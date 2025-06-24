// For the up_to_date code.
inherit "split";

protected string imgfile;
protected mapping(string:int) ifcnt = ([]); // ImageFile count
protected array all_consts;
protected string imgfilename( string w )
{
  w = replace( lower_case(w), ".", "_")+"_#";
  return "images/"+w+".png";
}

// Output pike refdoc-style documentation from the full code-tree.
// Also generates nice readable 'source-code' as a bonus. :-)
protected string make_example_image( string data, int toplevel )
{
  string tim = replace( imgfile, "#", (string)(++ifcnt[imgfile]));
  tim = replace( tim, "_1", "" );

  if( !file_stat( dir+"/"+tim ) )
    Process.create_process( ({master()->_pike_file_name,
			      "-DNOT_INSTALLED",
			      "-DPRECOMPILED_SEARCH_MORE",
			      "-m"+master()->_master_file_name,
			      combine_path(__FILE__,
					   "../../make_example_image.pike"),
			      replace(data, "@@", "@"),
			      toplevel?"TOP":"SUB",
			      dir, tim
			    }),
			    ([ "stderr":Stdio.stderr,
			       "stdout":Stdio.stdout ]) )->wait();
  return ("@expr{" + data + "@}\n"
	  "@xml{<image>../"+tim+"</image>@}\n");
}

protected string fix_images( string data )
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

protected string fix_const( string s )
{
  string a, b, c;
  string const_refs( string b )
  {
    array res = ({});
    sort(all_consts->name, all_consts);
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

protected string trim_xml( string what )
{
  string a, b, c, d;
  what = replace(what, "@", "@@");
  while( sscanf( what, "%s<br%*s>%s", a, b ) == 3 ) what = a+b;
  while( sscanf( what, "%s<p>%s", a, b ) == 2 )     what = a+b;
  while( sscanf( what, "%s</p>%s", a, b ) == 2 )    what = a+b;

  while( sscanf( what, "%s<b>%s</b>%s", a, b,c ) == 3 )
    what = a+"@b{"+b+"@}"+c;
  while( sscanf( what, "%s<i>%s</i>%s", a, b,c ) == 3 )
    what = a+"@i{"+b+"@}"+c;
  while( sscanf( what, "%s<tt>%s</tt>%s", a, b,c ) == 3 )
    what = a+"@tt{"+b+"@}"+c;
  while( sscanf( what, "%s<pre>%s</pre>%s", a,b,c ) == 3 )
    what = a+"@pre{"+b+"@}"+c;

  while( sscanf( what, "%s<blockquote>%s</blockquote>%s", a,b,c ) == 3 )
    what = a+b+c;
  while( sscanf( what, "%s<font%*s>%s</font>%s", a,b,c ) == 4 )
    what = a+b+c;
  while( sscanf( what, "%s<a%s>%s</a>%s", a,b,c,d ) == 4)
    what = a+"@xml{<url "+b+">"+c+"</url>@}"+d;

  while( sscanf( what, "%s<dl>%s</dl>%s", a,b,c ) == 3 ) {
    what = a + "@dl\n";
    object p = Parser.XML.Tree.parse_input("<dl>"+b+"</dl>");
    p = p->get_first_element("dl");
    foreach(p->get_children(), object n) {
      if(n->get_any_name()=="dt")
        what += " @item " + (n->get_children()->render_xml()*"" - "\n") + "\n";
      else if(n->get_any_name()=="dd")
	what += " " + n->get_children()->render_xml()*"" + "\n";
      else if(sizeof(String.trim(n->get_text())))
	werror("Warning: Discarding HTML subtree: %O\n", n->render_xml());
    }
    what += " @enddl\n" + c;
  }

  int table;
  while( sscanf( what, "%s<table>%s</table>%s", a,b,c ) == 3 ) {
    what = a + "@xml{<matrix>@}" + b + "@xml{</matrix>@}" + c;
    table = 1;
  }

  if(table) {
    while( sscanf( what, "%s<tr>%s</tr>%s", a,b,c ) == 3 )
      what = a + "@xml{<r>@}" + b + "@xml{</r>@}" + c;
    while( sscanf( what, "%s<td>%s</td>%s", a,b,c ) == 3 )
      what = a + "@xml{<c>@}" + b + "@xml{</c>@}" + c;
  }

  what = replace(what, "&nbsp;", " ");

  //  if(has_value(what,"<"))
  //    werror("Tag?: %O\n\n", what);
  return what;
}


protected string make_pike_refdoc( string pgtkdoc,
				mapping|void signals)
{
  string res =  "";
  if( !pgtkdoc || !sizeof(pgtkdoc) )
    return "//!\n";

  pgtkdoc = fix_images( fix_const( trim_xml(pgtkdoc) ) );
  foreach( pgtkdoc/"\n", string s )
  {
    if( !sizeof(s) )
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


protected string module_name( Class cls )
{
  string mn = (cls->name/".")[0];
  if (mn == "G") mn = "GTK2";
  return mn;
}

protected string class_name( Class cls, int|void nmn )
{
  array(string) a = cls->name/".";
  if ((a[0] == "G") || (sizeof(a) == 1)) a = ({ "GTK2" }) + a;
  if (nmn) a = a[1..];
  return a * ".";
}


protected string make_function_doc( GtkFunction f, Class c )
{
  if( f->name == "_sprintf" ||
      (f->name == "_destruct" && (< 0, "" >)[f->doc] ) )
    return "";

  string vtype;
  string pike_type_name( Type t )
  {
    if( f->pike_name() == "create" ) return "void";
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

  res = (f->is_protected()?"protected ":"")+
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
  if( !f->doc || !sizeof( f->doc ) )
  {
    werror("Warning: "+f->file+":"+f->line+": "
	   +c->name+"->"+f->name+" not documented\n" );
    res += "//!\n";
  }
  else
  {
    if(has_value(f->doc, "@"))
    {
      res += ("//" + (f->doc/"\n")[*])*"\n" + "\n";
    }
    else
      res += make_pike_refdoc( f->doc, 0 );
  }
  //  res += "{\n  // defined in\n  // "+f->file+":"+f->line+"\n}";
  return res;
}

protected void output_class( Class cls, int lvl )
{
  if( mixed e = catch {
  string result = "";
  array functions = ({});
  imgfile=imgfilename(cls->name);
  if( cls->name!="_global" && (!cls->doc || !sizeof( cls->doc )) )
    werror("Warning: "+cls->file+":"+cls->line+": "
	   +cls->name+" not documented\n" );

  result += sprintf("// Automatically generated from %q.\n"
                    "// Do NOT edit.\n"
                    "\n",
                    basename(cls->file));
  result += make_pike_refdoc( cls->doc, cls->signals );

  foreach( cls->inherits, Class i )
    result += "\ninherit "+i->doc_name()+";\n//!\n";
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
    Stdio.mkdirhier( dir + "GTK2.pmod/" );
    write_file(  dir + "GTK2.pmod/module.pmod",
                 /*"inherit GTKSupport;\n\n" + */
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
protected void output_constant( Constant c )
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
