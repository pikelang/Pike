#!/usr/bin/env pike
#define COMPOSE(X) Parser.Pike.reconstitute_with_line_numbers( X )
#define SPLIT(X,FN) Parser.Pike.group(Parser.Pike.hide_whitespaces(Parser.Pike.tokenize(Parser.Pike.split(X),FN)))
#define GOBBLE() ((sizeof(t)>i)?t[i++]:0)
#define PEEK()    ((sizeof(t)>i)?t[i]:0)
#define SYNTAX(X,T) do{werror("\n%s:%d:\tSyntax error:\t"+replace((X),"%","%%")+"\n",T->file,T->line);exit(1);}while(0)
#define ERROR(X,T) do{werror("\n%s:%d:\Error:\t"+replace((X),"%","%%")+"\n",T->file,T->line);exit(1);}while(0)
#define WARN(X,T) ERROR(X,T)
#define SEMICOLON(X)  if(GOBBLE()!=";") SYNTAX("Missing ; after "+(X),token);
#define NEED_CLASS(X) if(!current_class) current_class = get_class_define("_global",file,(tk?tk:token)->line);

#define IDENTIFIER(X) do{                                               \
  tk=GOBBLE();                                                          \
  if(!tk||arrayp(tk)||!is_identifier(tk))                               \
    SYNTAX("Expected identifier after '"+(X)+"', got "+sprintf("%O",(arrayp(tk)?tk[0]:tk||token)->text),(arrayp(tk)?tk[0]:tk||token));                \
} while(0);

#define TYPE(X) do{                                                        \
  tk=GOBBLE();                                                             \
  if(!tk||arrayp(tk))                                                      \
    SYNTAX("Expected type after "+(X),(tk?tk[0]:token));                   \
  if( tk == "?" || tk == "*" || tk == "&" )                                \
      tk = ({ tk, GOBBLE() });                                             \
  while( 1 )                                                               \
  {                                                                        \
    mixed tk2 = PEEK();                                                    \
    if(arrayp(tk2)&&tk2[0]=="(")                                           \
      tk=({tk,GOBBLE()});                                                  \
    if( tk2 == "." )                                                       \
      tk = ({ tk, GOBBLE(), GOBBLE() });                                   \
    else                                                                   \
      break;                                                               \
  }                                                                        \
  tk=parse_type(tk);                                                       \
} while(0);

string indent( string what, int amnt )
{
  if( !strlen(what) ) return "";
  string q = (" "*amnt);
  return q+((what/"\n")*("\n"+q));
}

string get_string_data()
{
  return gbl_data;
}

string gbl_data = "";
mapping ocache = ([]);

int data_offset( string what )
{
  int off;

  if(ocache[what])
    return ocache[what];

  if((off=search(gbl_data,what))!=-1)
    return ocache[what]=off;
//   write("New string: %O\n", what);

  gbl_data += what;

  return ocache[what]=(strlen(gbl_data)-strlen(what));
}

string S( string what, int|void nul, int|void nq, int|void ind )
{
  return sprintf("/* %O */\n%s(char*)__pgtk_string_data+0x%x",
                 (nq?what:replace(what,"%","%%")),
                 " "*ind,
                 (data_offset(what+(nul?"\0":"")))
                );
}

string unsillycaps(string what)
{
  string res=upper_case(what[0..0]);
  foreach(what[1..]/"", string q)
    if(lower_case(q)==q)
      res += q;
    else
      res += "_"+lower_case(q);
  return res;
}

class Function(Class parent,
               string name,
               Type return_type,
               array(Type) arg_types,
               array(string) arg_names,
               mixed body,
               array require,
               string doc,
               string file,
               int line)
{
  static string _sprintf()
  {
    return sprintf("Function( %O, %O )",name, return_type );
  }

  static string prefix( Class c )
  {
    if( c->c_name() == "" )
      return "pgtk";
    return "p"+c->c_name();
    
  }

  string c_name( )
  {
    string base = parent->c_name();
    if( parent->c_name() == "" )
      return "gtk_"+name;
    if( name == "create" )
      return base+"_new";
    return base+"_"+name;
  }

  string c_prototype()
  {
    return "void p"+c_name()+"( INT32 args );\n";
  }

  string c_defenition()
  {
    string res = "";
    void emit( string what )
    {
      res += what;
    };
    emit( "void p"+c_name()+"( INT32 args )\n" );
    if( body  && body != ";" )
    {
      emit( COMPOSE( body ) );
    } else {
      emit( sprintf("#line %d %O\n", line, file ) );
      emit("{\n");
      int a = 0;
      foreach( arg_types, Type t )
      {
        if( t->c_declare( a ) )
          emit( t->c_declare( a ) );
        a += t->c_stack_consumed( a );
      }

      a = 0;
      int required;
      foreach( arg_types, Type t )
      {
        a += t->c_stack_consumed( a );
        if( !t->opt ) required = a;
      }
      if( required )
        emit( "  if( args < "+required+" )\n"
              "    Pike_error("+S("Too few arguments, %d required, got %d\n",
                                  1,1,16)+",\n"
              "               "+required+", args);\n");

      a = 0;
      foreach( arg_types, Type t )
      {
        if( t->c_fetch_from_stack( a ) )
          emit( t->c_fetch_from_stack( a ) );
        a += t->c_stack_consumed( a );
      }


      /* Arguments now fetched. Time to call the function. :-) */
      int rv = a;
      if( return_type->name != "void" )
      {
        emit("  {\n");
        emit("  "+return_type->c_declare( rv = a+1 ));
        emit("    a"+rv+" = ");
      }

      emit( "  "+c_name() + "( " );
      if( name != "create" )
        emit( parent->c_cast( "THIS->obj" ) );
      a = 0;
      array args = ({});

      foreach( arg_types, Type t )
      {
        if( t->c_pass_to_function( a ) )
          args += ({  t->c_pass_to_function( a ) });
        a += t->c_stack_consumed( a );
      }
      if( sizeof( args ) )
        emit( (name=="create"?"":", ")+(args*", ") );
      emit( " );\n");
      if( return_type->name != "void" )
      {
        emit("    my_pop_n_elems(args);\n");
        emit("  "+return_type->push( "a"+rv )+"\n");
        emit("  }\n");
      }
      else
      {
        emit("  RETURN_THIS();\n");
      }
      a = 0;
      foreach( arg_types, Type t )
      {
        if( t->c_free( a ) )
          emit( t->c_free( a ) );
        a += t->c_stack_consumed( a );
      }
      emit("}\n\n");
    }
    return res;
  }
}

class Signal( string name )
{
  string doc = "";

  static string _sprintf()
  {
    return sprintf("Signal( %O )",name );
  }
}
  
class Member( string name, Type type, int set,
              string file, int line, Class parent )
{
  string doc = "";

  string c_name( )
  {
    if( parent->c_name() == "" )
      return "gtk_"+name;
    return parent->c_name()+"_get_"+name;
  }

  string c_prototype()
  {
    return "void p"+c_name()+"( INT32 args );\n";
  }

  string c_defenition()
  {
    return
      "void p"+c_name()+#"( INT32 args )\n"
      "{\n"
      "  if( args )\n"
      "    Pike_error("+S("Too many arguments.\n",1,1,16)+");\n"
      +type->direct_push( parent->c_cast( "THIS->obj" ) +"->"+name )+
      "}\n\n";
  }

  static string _sprintf()
  {
    return sprintf("Member( %O /* %O */ )",name,type );
  }
}
  

class Type
{
  int star, amp, opt, copy, ref;
  array(Type) subtypes;
  Type array_type;
  
  string name;
  string modifiers;
  
  array _s_modifiers;
  void create( string n )
  {
    array q = n/"|";
    if( sizeof(q) != 1 )
    {
      subtypes = map( q, Type );
      int ind;
      if( (ind = search( subtypes->name, "void" )) != -1 )
      {
        opt = 1;
        subtypes = subtypes[..ind-1] + subtypes[ind+1..];
      }
      name = subtypes[0]->name;
      modifiers = subtypes[0]->modifiers;
      star = subtypes[0]->star;
      amp = subtypes[0]->amp;
      if( sizeof( subtypes ) == 1 )
        subtypes = 0;
    }
    else
    {
      amp = sscanf( n, "&%s", n );
      star = sscanf( n, "*%s", n );
      opt = sscanf(n, "?%s", n );
      name = n;
      if( sscanf( name, "%[^(](%s", name, modifiers ) == 2 )
        modifiers = modifiers[..sizeof(modifiers)-2];
      foreach( get_modifiers(), string modifier )
      {
        switch( modifier )
        {
         case "ref":
           ref = 1;
           _s_modifiers -= ({ "ref" });
           break;

         case "copy":
           copy = 1;
           _s_modifiers -= ({ "copy" });
           break;
        }
      }
    }
  }

  string _sprintf()
  {
    if( subtypes )
      return subtypes->_sprintf()*" | ";
    array q = get_modifiers();
    if( q == ({}) )
      return name;
    return (opt?"void|":"") + name+"( "+q*","+" )";
  }

  array get_modifiers()
  {
    if( _s_modifiers )
      return _s_modifiers;
    if( !modifiers )
      return ({});
    _s_modifiers = SPLIT(modifiers,"-")/({","});
    for( int i = 0; i<sizeof(_s_modifiers); i++ )
      _s_modifiers[i] = Array.flatten(_s_modifiers[i])->text*"";
    return _s_modifiers;
  }
  
  static string declare, fetch, pass, free, _push;
  static int consumed = 1;
  static int c_inited;
  static string array_size;
  static string _dpush;
  string direct_push( string vv )
  {
    if(!c_inited) c_init();
    if( amp )  vv = "&("+vv+")";
    if( star ) vv = "*("+vv+")";

    switch( name )
    {
     case "array":
       if( !array_size )   throw("Cannot push array of unknown size");
       if( !array_type )   throw("Cannot push array(mixed)"); 
       return
         "  {\n"
         "    int i;\n"
         "    for( i = 0; i<"+array_size+"; i++ )\n"
         "    "+array_type->direct_push( "("+vv+"[i])" )+"\n"
         "    f_aggregate( i );\n"
         " }\n";

     default:
       if( !copy && !ref )
         return push( vv );

       string res ="  {\n  "+c_declare(256);
       if( copy )
         res += "    a256 = xalloc( sizeof( a256[0] ) );\n"
                "    *a256=*("+vv+");\n";
       else
         res += "    a256 = ("+vv+");\n";

       if( ref )
       {
         Class c;
         if( !(c = classes[ name ]) )
           throw(sprintf("Cannot reference %O, it's not a class type!\n",
                         array_type));
         if( c->is_gtkobject() )
           res+="    gtk_object_ref( GTK_OBJECT( a256 ) );\n";
         else
           res+="    "+c->c_name()+"_ref( a256 );\n";
       }
       res += "    "+push("a256")+"\n  }\n";
       return res;
    }
  }
  
  string push( string vv )
  {
    if( _push )
      return sprintf( _push, vv );

    switch( name )
    {
     case "string":
       _push = "  PGTK_PUSH_GCHAR( %s );";
       if( search( get_modifiers(), "free"  ) != -1 )
         _push += "\n  g_free( %[0]s );";
       return sprintf( _push, vv );

     case "int":
     case "uint":
       return sprintf( (_push = "  PGTK_PUSH_INT( %s );"), vv );

     case "float":
     case "double":
       return sprintf( (_push="  push_float( (FLOAT_TYPE)%s );"), vv );

     default:
       if( classes[name] )
         return classes[name]->push( vv );
       throw(sprintf("Cannot push %O, %s is not a class", this_object(),
                     name));
    }
  }

  
  static void c_init()
  {
    c_inited = 1;
    if( subtypes )
    {
      throw(sprintf("Complex types cannot be handled automatically (%O)\n",
                    this_object()));
    }
    if( Class c = classes[ name ] )
    {
      declare = classes[name]->c_declare( -1 );
      fetch = classes[name]->c_fetch_from_stack( -1 );
      pass = classes[name]->c_pass_to_function( -1 );
    }
    else switch( name )
    {
     case "null":
       declare = 0;
       fetch = 0;
       pass = "NULL";
       consumed = 0;
       break;

     case "function":
       if( modifiers != "callback" )
       {
         throw(sprintf("Complex types cannot be handled automatically (%O)\n",
                       this_object()));
       }

       /* Fallthrough */
     case "callback": /* actually 2 args */
       consumed = 2;
       declare = ("struct signal_data *cb%d = 0;\n"
                  "struct svalue *cb_tmp1 = 0, *cb_tmp2 = 0;\n");
       fetch =("  cb%[0]d = xalloc( sizeof( struct signal_data ) );\n"
               "  assign_svalue_no_free( &b->cb, Pike_sp[%[0]d-args] );\n"
               "  assign_svalue_no_free( &b->args,Pike_sp[%[0]d-args+1] );\n");

       pass  = ("(void *)pgtk_button_func_wrapper, cb");
       break;

     case "int":
     case "uint":
       declare = "  gint a%[0]d = 0;\n";
       if( name == "uint")
         declare = "  guint a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gint)PGTK_GETINT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "float":
       declare = "  gfloat a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gint)PGTK_GETFLT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "double":
       declare = "  gdouble a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gint)PGTK_GETFLT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "string":
       declare = "  gchar *a%[0]d = 0;\n";
       fetch =
             " if( Pike_sp[%[0]d-args].type != PIKE_T_STRING )\n"
             "   Pike_error( "+S("Illegal argument %d, expected string\n",1,0,16)+",\n                %[0]d);\n"
             "  a%[0]d = PGTK_GETSTR( &Pike_sp[%[0]d-args] );\n";
       free = "   PGTK_FREESTR( a%[0]d );\n";
       pass = "a%[0]d";
       break;

     case "array": /* Oh my. */
       {
         string sub, pt, process, check, lfree, check_size;
         int end_null, nofree, do_loopfree;
         foreach( get_modifiers(), mixed opt )
         {
           switch( opt )
           {
            case "string":
              array_type = parse_type( SPLIT("string","type") );
              sub = "gchar **a%[0]d;";
              check = "PGTK_ISSTR(&_a%[0]d->item[_i%[0]d])";
              process = "PGTK_GETSTR(&_a%[0]d->item[_i%[0]d])";
              do_loopfree = 1;
              lfree = "  PGTK_FREESTR(a%[0]d[i%[0]d])";
              break;
            case "int":
              array_type = parse_type( SPLIT("int","type") );
              sub = "gint **a%[0]d;";
              pt = 0;
              check = "PGTK_ISINT(&_a%[0]d->item[_i%[0]d])";
              process = "(gint)PGTK_GETINT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "uint":
              array_type = parse_type( SPLIT("uint","type") );
              sub = "guint **a%[0]d;";
              pt = 0;
              check = "PGTK_ISINT(&_a%[0]d->item[_i%[0]d])";
              process = "(guint)PGTK_GETINT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "float":
              array_type = parse_type( SPLIT("float","type") );
              sub = "gfloat **a%[0]d;";
              pt = 0;
              check = "PGTK_ISFLT(&_a%[0]d->item[_i%[0]d])";
              process = "(gfloat)PGTK_GETFLT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "double":
              array_type = parse_type( SPLIT("double","type") );
              sub = "gfloat **a%[0]d;";
              pt = 0;
              check = "PGTK_ISFLT(&_a%[0]d->item[_i%[0]d])";
              process = "(gdouble)PGTK_GETFLT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "null":
            case "0":
              end_null = 1;
              break;
            case "nofree":
              nofree = 1;
              break;
            default:
              if( sscanf( opt, "size=%s", array_size ) )
              {
                check_size =
                     "  if( _a%[0]d->size != "+array_size+" )\n"
                     "    Pike_error("+S("Illegal array size, wanted %d, "
                                         "got %d\n", 1, 0, 16)+", "+
                           array_size+", _a%[0]d->size );\n";
              }
              else
              {
                if( array_type ||
                    !(array_type = parse_type( SPLIT( opt, "type" ) ) ) )
                  throw( sprintf("Unknown array option %O\n", opt) );
              }
              break;
           }
         }
         if(!sub && !array_type)
         {
           throw( sprintf("Cannot push %O", this_object()) );
         }
         declare = "  int _i%[0]d;\n  struct array *_a%[0]d = 0;\n  " +
                 sub+"\n";
         pass = "a%d";
         fetch =
#"
  if( Pike_sp[%[0]d-args].type != PIKE_T_ARRAY )
    Pike_error("+S("Bad argument %d, expected array\n", 1,0,16)+#",\n                %[0]d);
  _a%[0]d = Pike_sp[%[0]d-args].u.array;
"+(check_size||"")+
"  a%[0]d = g_malloc0( sizeof( a%[0]d[0] ) * (_a%[0]d->size "+
             (end_null?"+1)":")")+#");
  for( _i%[0]d = 0; _i%[0]d < _a%[0]d->size; _i%[0]d++ )
  {
    if( !"+check+#" )
    {
      free( a%[0]d );
      Pike_error( "+S("Wrong type array argument (%d).\n", 1,0,20)+#",\n                    %[0]d);\n
    }
    a%[0]d[ _i%[0]d ] = "+process+#";
  }
";
         if( !nofree )
         {
           free="";
           if( do_loopfree )
             free =
                  "  for( _i%[0]d = 0; _i%[0]d < _a%[0]d->size; _i%[0]d++ )\n"+
                  "  "+lfree+";\n";
           free += "  g_free( a%[0]d );\n";
         }
       }
       break;
     default: /* Not as bad as an array, but still. :-) */
       if( !classes[ name ] )
         throw("Cannot handle the type "+name+"\n");
       return;
    }

    if( opt ) /* optional */
    {
      fetch = "  if( args > %[0]d ) {\n"+ indent(fetch,2) +" }\n";
      if( free )
        free = "  if( args > %[0]d ) {\n"+ indent(free,2) +" }\n";
    } else if( declare )
      declare = replace( declare, " = 0", "");
      
    if( star )  pass = "*"+pass;
    if( amp )   pass = "&"+pass;
  }

  int c_stack_consumed( int a )
  {
    if( !c_inited )c_init();
    return consumed;
  }
  string c_declare( int a )
  {
    if( !c_inited )c_init();
    return declare && sprintf(declare, a);
  }

  string c_free( int a )
  {
    if( !c_inited )c_init();
    return free  && sprintf(free, a);
  }

  string c_fetch_from_stack( int a )
  {
    if( !c_inited )c_init();
    return fetch && sprintf(fetch,a);
  }

  string c_pass_to_function( int a )
  {
    if( !c_inited )c_init();
    return pass && sprintf(pass,a);
  }
}

class Class( string name, string file, int line )
{
  Class inherits;
  mapping(string:Function) functions = ([]);
  mapping(string:Signal)   signals   = ([]);
  mapping(string:Member)   members   = ([]);
  string doc;

  array pre  = ({});
  string post = "";


  string _pass;
  string _fetch;
  string _cname;
  string _cdcl;


  int is_gtkobject()
  {
    if( name == "GTK.Object" )
      return 1;
    if( inherits )
      return inherits->is_gtkobject();
    return 0;
  }

  string c_name( )
  {
    if( _cname )
      return _cname;
    if( name == "_global" )  return "";
    string mn = (name/".")[0];
    string cn = (name/".")[-1];
    if( mn == cn )
      return mn;
    if( mn == "gnome" && (search( lower_case(cn), "applet" ) == 0) )
      return unsillycaps(lower_case(cn));
    cn = replace( cn, ({ "GL", "GC","XML" }), ({ "Gl","Gc","Xml"}) );
    return _cname = (lower_case(mn)+"_"+lower_case(unsillycaps(cn)));
  }

  string c_fetch_from_stack( int i )
  {
    if( !_fetch )
    {
      string n = c_name();
      if( n[1] != 'd' )
      {
        _fetch=
#"
  if( Pike_sp[ %[0]d - args ].type != PIKE_T_OBJECT )
    a%[0]d = NULL;
  else
    a%[0]d = "+c_cast("get_pgtkobject(Pike_sp[%[0]d-args].u.object, p"+
                      n+"_program)")+";\n";
      }
      else
      {
        string s = (name/".")[-1];
        _fetch=
#"
  if( Pike_sp[ %[0]d - args ].type != PIKE_T_OBJECT )
    a%[0]d = NULL;
  else
    a%[0]d = get_gdkobject(Pike_sp[%[0]d-args].u.object, "+s+");\n";
      }
    }
    if( i < 0 ) return _fetch;
    return sprintf( _fetch, i );
  }

  string c_pass_to_function( int i )
  {
    if( !_pass ) _pass = c_cast( "a%[0]d" );
    if( i < 0 ) return _pass;
    return sprintf( _pass, i );
  }

  string c_declare( int n )
  {
    if( !_cdcl ) {
      _cdcl = "  "+(name-".")+" *a%[0]d = 0;\n";
      _cdcl = replace(_cdcl, "GTK", "Gtk" );
      _cdcl = replace(_cdcl, "GDK", "Gdk" );
      _cdcl = replace(_cdcl, "GNOME", "Gnome" );
    }
    if( n < 0 )  return _cdcl;
    return sprintf( _cdcl, n );
  }

  string c_cast( string x )
  {
    return upper_case(c_name())+"("+x+")";
  }

  string _push;
  string push( string vv )
  {
    if( _push ) return sprintf( _push, vv );
    string mn = (name/".")[0];
    string nn = (name/".")[-1];
    switch( mn )
    {
     case "GDK":
       _push="  push_gdkobject( %s, "+nn+" );";
       break;
     case "GTK":
     case "Gnome":
       _push="  push_gtkobjectclass( %s, p"+c_name()+"_program );";
       break;
    }
    return sprintf( _push, vv );
  }

  string direct_push( string x, int amp, int star )
  {
    if( amp )  x = "&("+x+")";
    if( star)  x = "*("+x+")";
    return push( x );
  }
  

  static string _sprintf()
  {
    return sprintf("Class( %O /* %d funcs. */ )",name,
                   sizeof(functions)+sizeof(members) );
  }

  class Ref( string file, int line, Class c ) {  }

  array(Ref) references = ({ });

  Class add_function( object f )
  {
    functions[ f->name ] = f;
    return this_object();
  }

  Class add_signal( Signal s )
  {
    signals[ s->name ] = s;
    return this_object();
  }
  
  Class add_member( Member m )
  {
    members[ m->name ] = m;
    return this_object();
  }
  
  Class add_ref( string f, int l, Class c )
  {
    references += ({ Ref( f, l, c ) });
    return this_object();
  }
}

mapping(string:Class) classes = ([]);
mapping(string:Constant) constants = ([]);
array global_pre = ({});

class Constant( string name, Type t, string file, int line )
{
  string doc = "";
  static string _sprintf()
  {
    return sprintf("Constant( %O /* %O */ )",name,t );
  }
}

Constant get_constant_def( string name, Type t, string file, int line )
{
  Constant res;
  if( res = constants[ name ] )
  {
    write( file+":"+line+":\tError:\tRedefining constant "+name+"\n");
    write( file+":"+line+":\tError:\tPreviously defined in "+
           res->file+":"+res->line+"\n");
    exit(1);
  }
  return constants[name] = Constant( name, t, file, line );
}

Class get_class_ref( string name, string file, int line,
                     Class p )
{
  Class res = classes[ name ];
  if(!res)
    res = classes[ name ] = Class( name, 0, 0 );
  if( p )
    return res->add_ref( file, line,p );
  return res;
}

Class get_class_define( string name, string file, int line )
{
  Class res = classes[name];
  if( res )
  {
    if( res->file )
    {
      werror("\n");
      werror(file+":"+line+"\tError: "+name+" redefined\n" );
      werror(res->file+":"+res->line+"\t       Previous defenition\n" );
      exit(1);
    }
    res->file = file;
    res->line = line;
  }
  else
    res = classes[name] = Class(name,file,line);
  return res;
}

mapping(string:Type) types = ([]);
Type parse_type( mixed t )
{
  string tt;
  if(!sizeof(t))
    return 0;
  if( arrayp(t) )
  {
    t = Array.flatten(t);
    tt = t->text*"";
    t = t[0]; // only used for line-number in error messages
  }
  else
    tt = t->text;
  if(!strlen(tt))
    SYNTAX("Expected type",t); 
  if( tt[0] == '"' ) // No types are strings...
    SYNTAX("Expected type",t); 
  if( types[tt] ) return types[tt];
  Type ty = Type( tt );
  types[tt] = ty;
  switch( ty->name )
  {
   case "int":
   case "mapping":
   case "object":
   case "mixed":
   case "float":
   case "double":
   case "string":
   case "null":
   case "void":
   case "function":
   case "callback":
   case "array":
   case "Stdio.File":
   case "Image.Image":
   case "Image.Color.Color":
     break;
   default:
     if( ty->name[0..0] != "G" )
       SYNTAX(sprintf("%O is not a valid type",tt),t);
  }
  return ty;
}

int is_identifier( object t )
{
  if( arrayp(t) )
    return 0;
  if( sizeof(t->text/""-
     "ab_cde123456789fghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"/"" ) )
    return 0;
  if( t->text[0] >= '0' && t->text[0] <= '9' )
    return 0;
  return 1;
}

multiset options;
int verify_required( array r )
{
  if(!options)
    options = mkmultiset( ((Stdio.read_bytes( "options" )||"")-" ")/"\n" );
  foreach( r, string opt )
    if( !sscanf( opt, "define %s", opt ) && !options[ opt ] )
      return 0;
  return 1;
}

array parse_args( array tokens )
{
  if(!sizeof(tokens))
    return ({ ({}), ({}) });
  array(array(string)) q = tokens / ({","});
  array types = ({});
  array names = ({});
  foreach( q, array one )
  {
    if( !sizeof( one ) )
      SYNTAX("Illegal argument defenition ''", tokens[0]);
    if( one[0] == "null"  && sizeof(one) == 1 )
    {
      types += ({ parse_type( one[0] ) });
      names += ({ "undef" });
    } else {
      if( sizeof(one) < 2 )
        SYNTAX("Illegal argument defenition '"+(one->text*" ")+"'", one[0] );
      types += ({ parse_type( one[..sizeof(one)-2] ) });
      if( !is_identifier( one[-1] ) )
        SYNTAX( "Expected identifier after type", (arrayp(one[-1])?one[-1][0]:one[-1]) );
      names += ({ one[-1]->text });
    }
  }
  return ({ types, names });
}

string parse_pre_file( string file )
{
  array current_require = ({});
  Class current_class;
  mixed current_scope;

  if( file[0] != '/' )
    file = combine_path( getcwd(), file );

  array t = SPLIT(Stdio.read_file(file),file);
  // pass 1: Find valid preprocessor macros.
  int i, have_pp;
  for( int i = 0; i<sizeof(t); i++ )
  {
    object pp = t[i];
    string fname;
    if( strlen(pp->text) &&
        pp->text[0] == '#' &&
        sscanf( pp->text, "#include \"%s.inc\"", fname ) )
    {
      fname = combine_path( file, "../"+fname+".inc" );
      have_pp = 1;
      string data = Stdio.read_bytes( fname );
      if(!data)
        ERROR("Failed to read "+fname, pp );
      t[i] = SPLIT( data, fname );
    }
  }
  if( have_pp )
    t = SPLIT(COMPOSE(t),file);

  i = 0;
  do
  {
    mixed type, name, args, body; // used in several cases below
    array(Type) arg_types;
    array(string) arg_names;
    mixed tk,token = GOBBLE();
    string doc = "";
    if( objectp( token ) )
    {
      switch( token->text )
      {
       case "require":
         array q = ({});
         while( (tk = GOBBLE() ) != ";" )
           q += ({ tk });
         current_require += ({ (q->text*" ") });
         break;
       case "endrequire":
         current_require = current_require[ .. sizeof(current_require)-2 ];
         SEMICOLON("endrequire");
         break;
       case "class":
         tk = GOBBLE();
         while( PEEK() != ";" )
           tk = ({ tk, GOBBLE() });
         if(!arrayp(tk)) tk = ({ tk });
         current_scope=current_class=
           get_class_define(Array.flatten(tk)->text*"",file,tk[-1]->line);
         SEMICOLON("class");
         break;
       case "member":
         NEED_CLASS("member");
         TYPE("member");  type = tk;
         IDENTIFIER("member"); name = tk;
         SEMICOLON("member");
         current_class->add_member( current_scope =
                                    Member( name->text, type, 0,
                                            file, tk->line, current_class) );
         break;
       case "setmember":
         NEED_CLASS("member");
         TYPE("member");  type = tk;
         IDENTIFIER("member"); name = tk;
         SEMICOLON("setmember");
         current_class->add_member( current_scope =
                                    Member( name->text,type, 1,
                                            file, tk->line, current_class) );
         break;

       case "signal":
         NEED_CLASS("signal");
         IDENTIFIER("member"); name = tk;
         SEMICOLON("signal");
         current_class->add_signal( current_scope = Signal( name->text ) );
         break;

       case "constant":
         TYPE("constant");  type = tk;
         IDENTIFIER("constant"); name = tk;
         SEMICOLON("constant");
         current_scope = get_constant_def( name->text,type,file,name->line );
         break;

       case "DISABLED":
         GOBBLE();
         SEMICOLON("DISABLED");
         break;
         
       case "add_global":
         tk = GOBBLE();
         if( !arrayp(tk) || tk[0] != "{" || tk[-1] != "}" )
           SYNTAX("Expected add_global { code };",token);
         SEMICOLON("add_global");
         global_pre += tk[1..sizeof(tk)-2];
         break;
         
       case "inherit":
         NEED_CLASS("inherit");
         tk = GOBBLE();
         while( PEEK() != ";" )
           tk = ({ tk, GOBBLE() });
         if(!arrayp(tk)) tk = ({ tk });
         current_class->inherits
           =  get_class_ref(Array.flatten(tk)->text*"",
                            file,
                            tk[-1]->line,
                            current_class);
         SEMICOLON("inherit");
         break;

       case "%":
         NEED_CLASS("%{ %}");
         if( !arrayp( token = GOBBLE() ) )
           SYNTAX("Missing '{' after '%'",token);
         if( sizeof( token ) < 4 )
           SYNTAX( "Expected %{ code %}", token[0] );
         if( token[0] != "{" )
           SYNTAX( "Expected '{' after '%'", token[0] );
         if( token[-2] != "%" || token[-1] != "}" )
           SYNTAX( "Expected '%}' after '%{'", token[-1] );
         token = token[1..sizeof(token)-3];
         if( verify_required( current_require ) )
           if( current_class )
             current_class->pre += token;
         break;
       default:
         if( token->text[..1] == "/*" ) // comment
           continue;

         if( !strlen(token->text) )
           continue;

         if( token->text[0] == '#' )
         {
           if( !search( token->text, "#line" ) )
           {
             token->text="";
             continue;
           }
           else
             SYNTAX("Unexpected preprocessor code. Use %{ %}",token );
         }
         if( sscanf( token->text, "//!%s", string doc ) )
         {
           NEED_CLASS("documentation");
           current_scope->doc += doc+"\n";
           continue;
         }

         /*
          * type identifier( type identifier, ... )
          * ;  || { code }
          * makes a function defenition.
          */
         if(!current_class )
           SYNTAX("Need class before function defenition\n",  token);
         i--;
         NEED_CLASS("function defenition");
         TYPE("function defenition");       type = tk;
         IDENTIFIER("function defenition"); name = tk;
         args = GOBBLE();
         if(!arrayp(args) || args[0] != "(" || args[-1] != ")")
           SYNTAX("Expected argument array",args);
         string doc = "";
         for(;;)
         {
           body = GOBBLE();
           if(!body || arrayp(body) )
             break;
           if( body->text[..2] == "//!" )
             doc += body->text[2..];
           else
             break;
         }
         if( !body || (arrayp(body) && (body[0] != "{" || body[-1] != "}"))
             || (objectp(body) && body!=";" ))
           SYNTAX("Expected ; or function block",
                  (arrayp(body)?body[0]:body||token));
         [arg_types,arg_names] = parse_args( args[1..sizeof(args)-2] );
         Function f = Function( current_class, name->text, type,
                                arg_types, arg_names, body, 
                                current_require, doc, file, token->line );
         current_class->add_function( f );
         current_scope = f;
      }
    }
    else /* token is an array (group) */
    {
      werror("UGT: %O\n", token );
      SYNTAX(token[0]->text+" not expected here",token[0]);
    }
  } while( i < sizeof(t) );
}


void main(int argc, array argv)
{
  string source_dir = "source/";
  string destination_dir = "";
  string output;
  foreach( argv[1..], string option )
  {
    if( sscanf( option, "--source=%s", option ) )
      source_dir = option;
    else if( sscanf( option, "--destination=%s", option ) )
      destination_dir = option;
    else if( option == "--help" )
    {
      write(
#" %s [--help] [--sources=directory] plugin.pike

   --help: This help
   --source: Specify another directory for the source files.
              The default is source/
   --destination: Specify another directory for the destination files.
                  The default is the current directory
", argv[0] );
      exit(1);
    }
    else
      output = option;
  }
  if( source_dir[0] != '/' )
    source_dir = combine_path( getcwd(), source_dir );
  if( !strlen(destination_dir) || destination_dir[0] != '/' )
    destination_dir = combine_path( getcwd(), destination_dir );

  if(!output)
  {
    werror("You must specify an output plugin\n");
    exit(1);
  }

  add_constant( "Class", Class );
  add_constant( "unsillycaps", unsillycaps );
  add_constant( "Type", Type );
  add_constant( "S", S );
  add_constant( "data_offset", data_offset );
  add_constant( "get_string_data", get_string_data );

  object|array(object) q;
  add_constant( "Node", typeof( q ) );
  add_constant( "Function", Function );
  add_constant( "Constant", Constant );
  add_constant( "COMPOSE", Parser.Pike.reconstitute_with_line_numbers );
  object output_plugin = ((program)output)( source_dir, destination_dir,
                                            this_object() );

  if( output_plugin->up_to_date() )
  {
    exit(0);
  }
  werror("Parsing input files...     ");
  int t1 = gauge {
  foreach( glob( "*.pre",get_dir( source_dir )), string f)
    parse_pre_file( source_dir + f );
  };
  Class c = get_class_ref("_global", "*.c", 0, 0);
  foreach( glob( "*.c",get_dir( source_dir )), string f)
    c->pre += SPLIT( Stdio.read_bytes( source_dir+f ),source_dir+f);

  werror("%4.1fs\n", t1);
  werror("Checking inherits ...\n");

  string print_refs( array c, string name  )
  {
    foreach( c, object r )
      werror( r->file+":"+r->line+"   "+name+" not defined\n");
  };


  int err;
  foreach( indices( classes ), string c )
    if( classes[c]->inherits && !classes[c]->inherits->line )
    {
      err++;
      print_refs( classes[c]->inherits->references,
                  classes[c]->inherits->name );
    }
  if( err )
    exit(1);
  werror("Outputting result files...\n");
  int t2 = gauge {
    array files =
          output_plugin->output( classes, constants, global_pre );
    if( files )
      Stdio.write_file( destination_dir+"/files_to_compile",
                        replace(files*"\n",".c",".o") );
  };
  werror("Total time spent...        %4.1fs\n",t1+t2);
}
