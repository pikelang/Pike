// For the up_to_date code.
inherit "split";

// Output pike refdoc-style documentation from the full code-tree.
// Also generates nice readable 'source-code' as a bonus. :-)

static string make_pike_refdoc( string pgtkdoc,
                              mapping|void signals )
{
  if( !pgtkdoc || !strlen(pgtkdoc) )
    return "";
  string res =  "//! "+(pgtkdoc/"\n"*"\n//! ")+"\n";
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
  string vtype;
  string pike_type_name( Type t )
  {
    if( t->name == "void" )
      return vtype;
    if( parent->classes[ t->name ] )
      return class_name(parent->classes[ t->name ]);
    return t->pike_type();
  };

  if( c->name == "_global" )
    vtype = "void";
  else
    vtype = "this_program"; // c->pike_name();
    
  
  string res = "";

  res = (f->is_static()?"static ":"")+
      pike_type_name( f->return_type||f->type )+
      " "+f->pike_name()+"( ";
  int i;
  if( f->arg_types )
    for( int i = 0; i<sizeof(f->arg_types); i++ )
    {
      if( i != 0 )
        res+= ", ";
      res += pike_type_name(f->arg_types[i])+" "+f->arg_names[i];
    }
  if( i ) res += " ";
  res += ")";
  res += "\n";
  res += make_pike_refdoc( f->doc, 0 );
  res += "{\n  // defined in\n  // "+f->file+":"+f->line+"\n}";
  return res;
}

static void output_class( Class cls, int lvl )
{
  string result = "";
  array functions = ({});
  result =  make_pike_refdoc( cls->doc, cls->signals );
  if( cls->inherits )
    result += "inherit "+cls->inherits->pike_name()+";\n";

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
    Stdio.mkdirhier( dir + "refdoc/GTK.pmod/" );
    write_file(  dir + "refdoc/GTK.pmod/module.pmod",
                 "inherit GTKSupport;\n\n"+
                 constants+result );
    constants="";
  }
  else
  {
    Stdio.mkdirhier( dir + "refdoc/"+module_name(cls)+".pmod/" );
    write_file(  dir + "refdoc/"+module_name(cls)+".pmod/"+
                 class_name(cls,1)+".pike",result );
  }
}

string constants="";
static void output_constant( Constant c )
{
  constants += "constant "+c->pike_name()+";\n"+
            make_pike_refdoc( c->doc, 0 );
}

array(string) output( mapping(string:Class) classes,
                      mapping(string:Constant) constants,
                      array(Node) global_code )
{

  foreach( sort(indices(constants)), string c )
    output_constant( constants[ c ] );

  traverse_class_tree( classes, output_class );
  
}
