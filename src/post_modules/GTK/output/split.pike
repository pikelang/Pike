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

array files = ({});
static void output_class( Class cls, int lvl )
{
  emit( sfhead );
  if( sizeof( cls->pre ) )
    emit( COMPOSE( cls->pre ) );
  void output_thing( object thing )
  {
    if( mixed err=catch( emit( thing->c_defenition() ) ) )
      werror(thing->file+":"+thing->line+": Error: "+
             (stringp(err)?err:describe_backtrace(err))+"\n" );
  };

  foreach( sort( indices( cls->functions ) ), string fun )
    output_thing( cls->functions[ fun ] );

  foreach( sort( indices( cls->members ) ), string mem )
    output_thing( cls->members[ mem  ] );



  // Processing done. Actually write the file.
  Stdio.File f;
  files += ({ filename(cls) });
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

string protos = "";

static void build_protos( Class cls, int lvl )
{
  if( cls->name != "global" )
    protos += ("  "*lvl) + "extern program p"+cls->c_name()+"_program;\n";
  foreach( sort( indices( cls->functions ) ), string f )
    protos += ("  "*lvl)+ " extern "+cls->functions[ f ]->c_prototype();
  foreach( sort( indices( cls->members ) ), string f )
    protos += ("  "*lvl)+ " extern "+cls->members[ f ]->c_prototype();

}

array(string) output( mapping(string:Class) classes,
                      mapping(string:Constant) constants,
                      array(Node) global_code )
{
  head = Stdio.read_bytes( combine_path( sdir, "../pgtk.c.head" ) );
  sfhead = replace( head, "PROTOTYPES", "#include <prototypes.h>" );
  traverse_class_tree( classes, output_class );
  traverse_class_tree( classes, build_protos );
  if( Stdio.read_bytes( dir + "prototypes.h" ) != protos )
  {
    werror("  Updating prototypes.h\n" );
    Stdio.write_file( dir + "prototypes.h", protos );
  }
  protos = "";
  Stdio.write_file( dir+"time_stamp", (string)time() );
//   traverse_class_tree( classes, output_fadds );
  return files;
}

int up_to_date( )
{
  int last_time = (int)Stdio.read_bytes( dir+"time_stamp" );
  if(!last_time )
    return 0;
  foreach( map(get_dir( sdir ), lambda(string s){return sdir+s;})|
           ({ __FILE__, combine_path( __FILE__, "../../new_build_pgtk.pike" )}),
           string f )
  {
    Stdio.Stat s;
    if( !(s = file_stat( f ) ) )
      werror("Failed to stat "+f+"\n");
    if( s->mtime > last_time )
      return 0;
  }
  return 1;
}


void create( string _s, string _d, object p )
{
  sdir = _s;
  dir = _d;
  parent = p;
}

