// Output one file per class as C, more or less.
inherit "util";
string head = "", sfhead="", dir, sdir;
string debug = "";
object parent;


static string prefix( Class c )
{
  return "p"+c->c_name();
}

static string filename( Class c )
{
  return prefix(c)+".c";
}

string current_data = "";
static void emit( string what )
{
  current_data += what;
}

static void output_class( Class cls, int lvl )
{
  emit( sfhead );
  if( sizeof( cls->pre ) )
    emit( COMPOSE( cls->pre ) );
  foreach( sort( indices( cls->functions ) ), string fun ) if(string err=catch
  {
    Function f = cls->functions[ fun ];
    emit( "void "+prefix(cls)+"_"+f->name+"( INT32 args )\n" );
    if( f->body  && f->body != ";" )
    {
      emit( COMPOSE( f->body ) );
    } else {
      emit( sprintf("#line %d %O\n", f->line, f->file ) );
      emit("{\n");
      int a = 0;
      foreach( f->arg_types, Type t )
      {
        if( t->c_declare( a ) )
          emit( t->c_declare( a ) );
        a += t->c_stack_consumed( a );
      }

      a = 0;
      int required;
      foreach( f->arg_types, Type t )
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
      foreach( f->arg_types, Type t )
      {
        if( t->c_fetch_from_stack( a ) )
          emit( t->c_fetch_from_stack( a ) );
        a += t->c_stack_consumed( a );
      }


      /* Arguments now fetched. Time to call the function. :-) */
      int rv = a;
      if( f->return_type->name != "void" )
      {
        emit("  {\n");
        emit("  "+f->return_type->c_declare( rv = a+1 ));
        emit("    a"+rv+" = ");
      }

      emit( "  "+f->c_name() + "( " );
      if( f->name != "create" )
        emit( cls->c_cast( "THIS->obj" ) );
      a = 0;
      array args = ({});

      foreach( f->arg_types, Type t )
      {
        if( t->c_pass_to_function( a ) )
          args += ({  t->c_pass_to_function( a ) });
        a += t->c_stack_consumed( a );
      }
      if( sizeof( args ) )
        emit( (f->name=="create"?"":", ")+(args*", ") );
      emit( " );\n");
      if( f->return_type->name != "void" )
      {
        emit("    my_pop_n_elems(args);\n");
        emit("  "+f->return_type->push( "a"+rv )+"\n");
        emit("  }\n");
      }
      else
      {
        emit("  RETURN_THIS();\n");
      }
      a = 0;
      foreach( f->arg_types, Type t )
      {
        if( t->c_free( a ) )
          emit( t->c_free( a ) );
        a += t->c_stack_consumed( a );
      }
      emit("}\n\n");
    }
  })
  {
    if( catch {
      werror(cls->functions[fun]->body->file+":"+
             cls->functions[fun]->body->line+": Error: "+
             (stringp(err)?err:describe_backtrace(err))+"\n" );
    } )
    {
      werror(cls->file+":"+
             cls->line+": Error: "+
             (stringp(err)?err:describe_backtrace(err))+"\n" );
    }
    exit(1);
  }
  // Processing done. Actually write the file.
  Stdio.File f;
  string file = dir+filename(cls);
  f = Stdio.File();
  if( f->open( file, "r" ) )
  {
    if( f->read() != current_data )
      werror("  Updating "+filename(cls)+"\n");
    else
    {
      current_data="";
      return;
    }
  } else
    werror("  Creating "+filename(cls)+" (%d bytes)\n", sizeof(current_data));
  f = Stdio.File( );
  if( !f->open( dir + filename(cls), "wct" ) )
  {
    werror("Failed to open "+dir + filename(cls)+"\n");
    exit(1);
  }
  f->write( current_data );
  current_data = "";
}

void output( mapping(string:Class) classes,
             mapping(string:Constant) constants,
             array(Node) global_code )
{
  head = Stdio.read_bytes( combine_path( sdir, "../pgtk.c.head" ) );
  sfhead = replace( head, "PROTOTYPES", "#include <prototypes.h>" );
  traverse_class_tree( classes, output_class );
}

int up_to_date( )
{
  return 0;
}


void create( string _s, string _d, object p )
{
  sdir = _s;
  dir = _d;
  parent = p;
}

