#!/usr/bin/env pike


string destination_dir = "";

/* Start of relevant parts of program_id.h */
#define PROG_IMAGE_IMAGE_ID             100
#define PROG_IMAGE_COLORTABLE_ID        101
#define PROG_IMAGE_LAYER_ID             102
#define PROG_IMAGE_FONT_ID              103
#define PROG_IMAGE_POLY_ID              104
#define PROG_IMAGE_COLOR_COLOR_ID       200
#define PROG_STDIO_FD_ID                  1

/* End of it */

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

string make_c_string( string from )
{
  string line = "\"";
  string res = "";
  int c;
  for( int i=0; i<strlen( from ); i++ )
  {
    switch( (c=from[i]) )
    {
     case '.':
     case 'a'..'z':
     case 'A'..'Z':
     case 0300..0376: // 377 breaks some versions of gcc.
     case '_': case ' ':
       line += from[i..i];
       break;
     default:
       line += sprintf("\\%o", c );
       break;
    }
    if( strlen( line ) > 75 )
    {
      res += line+"\"\n";
      line="\"";
    }
  }
  return res+line+"\"";
}

string get_string_data()
{
  return "const char __pgtk_string_data[] =\n"+
         make_c_string(gbl_data)+";\n\n\n";
}

string gbl_data = "";
mapping ocache = ([]);

int data_offset( string what )
{
  int off;

  if(ocache[what])
    return ocache[what];

  if((off=search(gbl_data,what))!=-1)
  {
    if( gbl_data[ off..off+strlen(what)-1 ] != what )
      werror( "Search returned illegal string match! %O != %O\n",
	      gbl_data[ off..off+strlen(what)-1 ], what );
    return ocache[what]=off;
  }
//   write("New string: %O\n", what);

  gbl_data += what;

  return data_offset( what );
}

string S( string what, int|void nul, int|void nq, int|void ind )
{
  if( nq == 2 )
    return sprintf("(PSTR+0x%x)",data_offset(what+(nul?"\0":"")));
  return sprintf("/* %O */\n%s(PSTR+0x%x)",
                 (nq?what:replace(what,"%","%%")),
                 " "*ind,
                 (data_offset(what+(nul?"\0":"")))
                );
}

string unsillycaps(string what)
{
  string res=upper_case(what[0..0]);
  if( what[0] == '_' )
  {
    res += what[1..1];
    what = what[1..];
  }
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
  static string _sprintf(int fmt)
  {
    return fmt=='O' && sprintf("Function( %O, %O )",name, return_type );
  }

  string pike_type( )
  {
    string rt = return_type->pike_type( 1 );
    if( parent->name != "_global" && has_prefix(rt, "void" ) )
      rt = parent->pike_type( 1 );
    array res =  ({ });
    foreach( arg_types, Type t )
    {
      if( t->name != "null" )
	res += ({ t->pike_type( 0 ) });
    }
    return "function("+ res*"," +":"+rt+")";
  }

  string pike_name()
  {
    return name;
  }

  int is_static()
  {
    return (name=="create"||(name[0]=='_'));
  }

  string pike_add( )
  {
    string type = function_type( pike_type( ) );
#ifdef EXTERMINATE_MIXED
    if( search( pike_type(), "mixed" ) != -1 )
    {
      write( "Hmm: "+parent->pike_name()+"->"+pike_name()+": "+pike_type()+
	     "\n" );
      write( "     "+(arg_types->pike_type()*",")+"\n");
      write( "     "+return_type->pike_type( 1 )+"\n");
      write( "     "+parent->pike_type( 1 )+"\n");
    }
#endif
    string res="";
    void low_do_emit( string name )
    {
      res += sprintf("    quick_add_function(%s,%d,p%s,%s,%d,\n                          "
                     "%s,OPT_EXTERNAL_DEPEND|OPT_SIDE_EFFECT);\n",
                     S(name,0,1,27), strlen(name),
                     c_name(), S(type,0,2,27),
                     strlen(type), (is_static()?"ID_STATIC":"0"));
    };
    array names = ({ name });
    switch( name )
    {
     case "union": names += ({ "`|" }); break;
     case "subtract": names += ({ "`-" }); break;
     case "intersect": names += ({ "`&" }); break;
     case "equal": names += ({ "`==" }); break;
     case "lt": names += ({ "`<" }); break;
     case "gt": names += ({ "`>" }); break;
     case "xor": names += ({ "`^" }); break;
     case "not": names += ({ "`~" }); break;
     case "_add": names += ({ "`+" }); break;
     case "_index": names += ({ "`[]", "`->" }); break;
    }


    foreach( names, string n ) low_do_emit( n );
    return res;
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
      if( name == "create" )
      {
        if( has_prefix( parent->name, "Gnome" ) )
          emit("    pgtk_verify_gnome_setup();\n" );
        else
          emit("    pgtk_verify_setup();\n" );
        emit("  pgtk_verify_not_inited();\n");
        emit( "  THIS->obj =  (void *)");
      } else
        emit("  pgtk_verify_inited();\n");

      if( return_type->name != "void" )
      {
        emit("  {\n");
        emit("  "+return_type->c_declare( rv = a+1, 1 ) );
        emit("    a"+rv+" = ");
        if( classes[ return_type->name ] )
          emit( " ("+classes[ return_type->name ]->c_type()+" *)" );
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
        if( name != "create" )
          emit("  RETURN_THIS();\n");
        else
        {
          emit( "  my_pop_n_elems( args );\n" );
          emit( "  push_int( 0 );\n" );
        }
      }
      a = 0;
      foreach( arg_types, Type t )
      {
        if( t->c_free( a ) )
          emit( t->c_free( a ) );
        a += t->c_stack_consumed( a );
      }
      if( name == "create" )
        emit( "  pgtk__init_object( Pike_fp->current_object );\n");
      emit("}\n\n");
    }
    return res;
  }
}

class Signal( string name )
{
  string doc = "";

  string pike_name( )
  {
    return replace( name, "-", "_" );
  }

  string pike_add( )
  {
    return "  add_string_constant( "+S("s_"+pike_name(),1,0,27)+", "+
           S(pike_name(),1,0,27)+", 0 );\n";
  }

  static string _sprintf(int fmt)
  {
    return fmt=='O' && sprintf("Signal( %O )",name );
  }
}

string function_type( string what )
{
  return __parse_pike_type( String.trim_all_whites( what ) );
}

class Member( string name, Type type, int set,
              string file, int line, Class parent )
{
  string doc = "";

  int is_static(){ return 0; }

  string pike_type( )
  {
    if( set )
      return "function("+type->pike_type(0)+":"+parent->pike_type(1)+")";
    else
      return "function(void:"+type->pike_type(1)+")";
  }

  string pike_add( )
  {
    string tp = function_type( pike_type( ) );
    if( set || classes[ type->name ] )
      return sprintf("    quick_add_function(%s,%d,p%s,\n                       %s,%d,"
                     "0,OPT_EXTERNAL_DEPEND);\n",
                     S(name,0,1,27), strlen(name),
                     c_name(), S(tp,0,2,27), strlen(tp));
    return sprintf("    quick_add_function(%s,%d,p%s,\n                       %s,%d,"
                   "0,OPT_EXTERNAL_DEPEND);\n",
                   S("get_"+name,0,1,27), strlen("get_"+name),
                   c_name(), S(tp,0,2,27), strlen(tp));
  }

  string pike_name()
  {
    return set ? "set_"+name : "get_"+name;
  }

  string c_name( )
  {
    if( parent->c_name() == "" )
      return "gtk_"+name;
    return set ? parent->c_name()+"_"+name : parent->c_name()+"_get_"+name;
  }

  string c_prototype()
  {
    return "void p"+c_name()+"( INT32 args );\n";
  }

  string c_defenition()
  {
    if( set )
      return
        "void p"+c_name()+"( INT32 args )\n"
        "{\n"
        + type->c_declare( 0 )+
        "  if( args != 1 )\n"
        "    Pike_error("+S("Wrong number of arguments.\n",1,1,16)+");\n"
        + type->c_fetch_from_stack( 0 ) +
        "  "+parent->c_cast( "THIS->obj" )+"->"+name[4..]+" = "+type->c_pass_to_function(0)+";\n"
        "  RETURN_THIS();\n"
        "}\n";
    else
      return
        "void p"+c_name()+#"( INT32 args )\n"
        "{\n"
        "  if( args )\n"
        "    Pike_error("+S("Too many arguments.\n",1,1,16)+");\n"
        +type->direct_push( parent->c_cast( "THIS->obj" ) +"->"+name )+
        "\n}\n\n";
  }

  static string _sprintf(int fmt)
  {
    return fmt=='O' && sprintf("Member( %O /* %O */ )",name,type );
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

  string doc_type( )
  {
    if( subtypes )
      return subtypes->doc_type()*"|";
    string optp = "";
    if( opt )
      optp = "|void";
    switch( name )
    {
      case "uint":
	return "int"+optp;
      case "array":
	if( array_type )
	  return "array("+array_type->doc_type()+")"+optp;
	return name+optp;

      case "double": /* int can be used for better precision */
	return "int|float"+optp;

      case "callback":
	return "callback"+optp;

      case "function":
	if( has_value( get_modifiers(), "callback" ) )
	  return "callback"+optp;
	return "function"+optp;

      case "void": /* needed for return types */
	return "void";

      case "GTK.CtreeNode":
	name = "GTK.CTreeNode";

      default:
	if( classes[ name ] )
	  name = classes[ name ]->pike_name();
	return name+optp;
    }
  }

  string pike_type( int is, int|void nc )
  {
    if( subtypes )
      return subtypes->pike_type(is,1)*"|";
    string optp = "";
    if( opt )
      optp = "|void";
    switch( name )
    {
      case "uint":
	return "int"+optp;
      case "array":
	if( !nc && !c_inited )
	  catch(c_init());
	if( array_type )
	  return "array("+array_type->pike_type(is,1)+")"+optp;
	/* fall-through */
      case "int":
      case "string":
      case "string...":
      case "mixed":
      case "mapping":
      case "float": 
      case "object":
	return name+optp;

      case "double": /* int can be used for better precision */
	return "int|float"+optp;

      case "Image.Image":
	return "object(implements "+PROG_IMAGE_IMAGE_ID+")"+optp;
      case "Stdio.File":
	return "object(implements "+PROG_STDIO_FD_ID+")"+optp;
      case "Image.Color.Color":
	return "object(implements "+PROG_IMAGE_COLOR_COLOR_ID+")"+optp;

      case "GDK.Atom": // implemented in pike
	return "object"+optp;

      case "callback":
	return "function"+optp+",mixed"+optp;
      case "function":
	if( has_value( get_modifiers(), "callback" ) )
	  return "function"+optp+",mixed"+optp;
	return "function"+optp;
      case "void": /* needed for return types */
	return "void";

      case "GTK.CtreeNode":
	name = "GTK.CTreeNode";

      default:
	if( classes[ name ] )
	  if( nc || opt )
	    return classes[ name ]->pike_type(is) + optp;
	  else
	    return (is ? classes[ name ]->pike_type(1) :
		    (classes[ name ]->pike_type(0)+ "|zero"));
	throw(sprintf("Unknown type %O", this_object()));
	return "mixed"+optp;
    }
  }

  void create( string n )
  {
    array q = n/"|";
    if( sizeof(q) != 1 )
    {
      subtypes = map( q, Type );
      int ind;
      if( (ind = has_value( subtypes->name, "void" )) )
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

  string _sprintf(int fmt)
  {
    if(fmt != 'O') return UNDEFINED;
    if( subtypes )
      return subtypes->_sprintf(fmt)*" | ";
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
       if( !array_size )
         throw(sprintf("Cannot push array of unknown size (%O)",
                       this_object()));
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
         res += "    a256 = (void *)xalloc( sizeof( a256[0] ) );\n"
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
       if( has_value( get_modifiers(), "free"  ) )
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
       declare = ("  struct signal_data *cb%[0]d = 0;\n");
       fetch =("  cb%[0]d = (void*)xalloc( sizeof( struct signal_data ) );\n"
               "  assign_svalue_no_free(&cb%[0]d->cb,  Pike_sp+%[0]d-args);\n"
               "  assign_svalue_no_free(&cb%[0]d->args,Pike_sp+%[0]d+1-args);\n");

       pass  = ("(void *)pgtk_buttonfuncwrapper, cb%[0]d");
       break;

     case "int":
     case "uint":
       declare = "  gint a%[0]d = 0;\n";
       if( name == "uint")
         declare = " guint a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gint)PGTK_GETINT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "float":
       declare = "  gfloat a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gfloat)PGTK_GETFLT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "double":
       declare = "  gdouble a%[0]d = 0;\n";
       fetch = "  a%[0]d = (gint)PGTK_GETFLT(&Pike_sp[%[0]d-args]);\n";
       pass =  "a%[0]d";
       break;

     case "string":
       declare = " CONST gchar *a%[0]d = 0;\n";
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
              lfree = "  PGTK_FREESTR(a%[0]d[_i%[0]d])";
              break;
            case "int":
              array_type = parse_type( SPLIT("int","type") );
              sub = "gint *a%[0]d;";
              pt = 0;
              check = "PGTK_ISINT(&_a%[0]d->item[_i%[0]d])";
              process = "(gint)PGTK_GETINT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "time_t":
              array_type = parse_type( SPLIT("int","type") );
              sub = "time_t *a%[0]d;";
              pt = 0;
              check = "PGTK_ISINT(&_a%[0]d->item[_i%[0]d])";
              process = "(time_t)PGTK_GETINT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "uint":
              array_type = parse_type( SPLIT("uint","type") );
              sub = "guint *a%[0]d;";
              pt = 0;
              check = "PGTK_ISINT(&_a%[0]d->item[_i%[0]d])";
              process = "(guint)PGTK_GETINT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "float":
              array_type = parse_type( SPLIT("float","type") );
              sub = "gfloat *a%[0]d;";
              pt = 0;
              check = "PGTK_ISFLT(&_a%[0]d->item[_i%[0]d])";
              process = "(gfloat)PGTK_GETFLT(&_a%[0]d->item[_i%[0]d])";
              break;
            case "double":
              array_type = parse_type( SPLIT("double","type") );
              sub = "gdouble *a%[0]d;";
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
              else if( has_prefix(opt, "cast" )  )
                ;
              else
              {
                if( array_type ||
                    !(array_type = parse_type( SPLIT( opt, "type" ) ) ) )
                  throw( sprintf("Unknown array option %O\n", opt) );
              }
              break;
           }
         }
//          if(!sub && !array_type)
//          {
//            throw( sprintf("Cannot push %O", this_object()) );
//          }
         declare = "  int _i%[0]d;\n  struct array *_a%[0]d = 0;\n  " +
                 "CONST "+sub+"\n";
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
  string c_declare( int a, int|void const )
  {
    if( !c_inited )c_init();
    if(!declare) return 0;
    if( const && !free )
      return sprintf( replace( declare, "CONST", "const" ), a );
    return sprintf( replace( declare, "CONST", "" ), a );
           
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
    if( pass )
    {
      foreach( get_modifiers(), string m )
        if( classes[m] )
          return classes[m]->c_cast( sprintf(pass,a) );
        else if( sscanf( m,"cast=%s", m ) )
          return replace(m,"_"," ")+sprintf(pass,a);

      return sprintf(pass,a);
    }
  }
}

int last_class_id = 1000;

class Class( string name, string file, int line )
{
  Class inherits;
  mapping(string:Function) functions = ([]);
  mapping(string:Signal)   signals   = ([]);
  mapping(string:Member)   members   = ([]);
  string doc = "";

  array pre  = ({});
  string post = "";


  string _pass;
  string _fetch;
  string _cname;
  string _cdcl;


  int _class_id;
  
  int class_id()
  {
    if( _class_id ) return _class_id;
    return (_class_id = last_class_id++);
  }

  string pike_type(int is)
  {
    if( name == "_global" )
      return "mixed";
//     return "object("+(is?"is ":"implements ")+class_id()+")";
    return "object(implements "+class_id()+")";
  }
  
  string doc_name()
  {
    return name;
  }

  string pike_name()
  {
    if( sscanf(name, "GTK.%s", string pn ) )
      return pn;
    return (replace(name,"GDK","Gdk")-".");
  }


  void create_default_sprintf( )
  {
    if( name == "_global" ) return 0;
    add_function( Function(this_object(),
                           "_sprintf",
                           Type("string"), ({
                             Type("int"),
                             Type("mapping")
                           }), ({
                             "flags",
                             "options",
                           }),
                           SPLIT(
                             "{\n  pgtk_default__sprintf( args, "+
                             data_offset( name )+","+strlen(name)+
                             " );\n}\n",
                             file),
                           ({}),
                           "Not normally called directly, used by sprintf()",
                           file, line ) );
  }
  
  string c_type_define()
  {
    array q = c_name()/"_";
    return upper_case( q[0]+"_TYPE_"+(q[1..]*"_") );
  }

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
    if( mn == "Gnome" && (has_prefix( cn, "Applet" ) ) )
      return lower_case(unsillycaps(cn));
    cn = replace( cn, ({ "GL", "GC","XML","CList","CTree" }),
                      ({ "Gl","Gc","Xml","Clist","Ctree"}) );
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
    a%[0]d = get_gdkobject(Pike_sp[%[0]d-args].u.object,"+lower_case(s)+");\n";
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

  string c_type( )
  {
    string bt = name-".";
    bt = replace(bt, "GTK", "Gtk" );
    bt = replace(bt, "GDK", "Gdk" );
    bt = replace(bt, "GNOME", "Gnome" );
    return bt;
  }

  string c_declare( int n )
  {
    if( !_cdcl )
      _cdcl = "  "+c_type()+" *a%[0]d = 0;\n";
    if( n < 0 )  return _cdcl;
    return sprintf( _cdcl, n );
  }

  string c_cast( string x )
  {
    string f = upper_case(c_name())+"("+x+")";
    if( has_prefix(f,"GDK_") )
      return "((Gdk"+((name-"_")/".")[1]+"*)"+x+")";
    return f;
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
       _push="  push_gdkobject( %s, "+lower_case(nn)+" );";
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
  

  static string _sprintf(int fmt)
  {
    return fmt=='O' && sprintf("Class( %O /* %d funcs. */ )",name,
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

class Constant( string name, Type type, string file, int line )
{
  string doc = "";

  string pike_name( )
  {
    string pn;
    if( sscanf( name, "GTK_%s", pn ) )
      return pn;
    return name;
  }

  string pike_add( )
  {
    switch( type->name )
    {
     case "string":
       return "  add_string_constant( "+S(pike_name(),1,0,30)+", "+name+", 0 );\n";
     case "int":
       return "  add_integer_constant( "+S(pike_name(),1,0,30)+", "+name+", 0 );\n";
     default:
       werror(file+":"+line+":\tCannot add consatnt of type %O\n",type);
       exit(1);
    }
  }

  static string _sprintf(int fmt)
  {
    return fmt=='O' && sprintf("Constant( %O /* %O */ )",name,type );
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
   case "uint":
   case "mapping":
   case "object":
   case "mixed":
   case "float":
   case "double":
   case "string":
   case "string...":
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
    options = mkmultiset( ((Stdio.read_bytes( destination_dir+"options" )||"")
                           -" ")/"\n" );
  foreach( r, string opt )
    if( !options[ opt- " " ] )
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
  array current_unrequire = ({});
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
         continue;
	case "not":
	 q = ({});
         while( (tk = GOBBLE() ) != ";" )
           q += ({ tk });
         current_unrequire += ({ (q->text*" ") });
         continue;
       case "endrequire":
         current_require = current_require[ .. sizeof(current_require)-2 ];
         SEMICOLON("endrequire");
         continue;
       case "endnot":
         current_unrequire = current_unrequire[ .. sizeof(current_unrequire)-2 ];
         SEMICOLON("endnot");
         continue;
      }
    }
    if( !verify_required( current_require ) ||
	(sizeof( current_unrequire ) && verify_required(current_unrequire)))
      continue;

    if( objectp( token ) )
    {
      switch( token->text )
      {
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
                                    Member( "set_"+name->text,type, 1,
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
           if( has_prefix( token->text, "#line" ) )
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
             doc += body->text[2..]+"\n";
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
      SYNTAX(token[0]->text+" not expected here",token[0]);
    }
  } while( i < sizeof(t) );
}


void main(int argc, array argv)
{
  string source_dir = "source/";
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
  if (source_dir[-1] != '/')
    source_dir += "/";
  if( !strlen(destination_dir) || destination_dir[0] != '/' )
    destination_dir = combine_path( getcwd(), destination_dir );
  if (destination_dir[-1] != '/')
    destination_dir += "/";

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

  if (string files = Stdio.read_file (destination_dir + "/files_to_compile")) {
    int ok = 1;
    foreach (files / " ", string file)
      if (!file_stat (destination_dir + "/" + file)) {ok = 0; break;}
    if( ok && output_plugin->up_to_date() )
    {
      exit(0);
    }
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
                        replace(files*" ",".c",".o") );
  };
  werror("Total time spent...        %4.1fs\n",t1+t2);
}
