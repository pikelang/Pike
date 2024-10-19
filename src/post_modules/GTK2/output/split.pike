// Output one file per class as C, more or less.
inherit "util";
string head = "", sfhead="", dir, sdir;
string debug = "";
object parent;

protected string filename( Class c )
{
  if( sizeof( c->c_name() ) )
    return "p"+c->c_name()+".c";
  return "pgtk_globals.c";
}

array files = ({});
protected void output_class( Class cls, int lvl )
{
  string current_data = "";

  if( !cls->functions["_sprintf"] )
    cls->create_default_sprintf();

  cls->create_init_exit();

  /* Start output */
  current_data += "#define EXTPRG extern\n"+sfhead;

  if(cls->mixin_for)
    current_data += "#define CLASS_TYPE MIXIN\n";

  if( sizeof( cls->pre ) )
    current_data += COMPOSE( cls->pre );
  void output_thing( object thing )
  {
    if( mixed err=catch( current_data += thing->c_definition() ) )
      werror(thing->file+":"+thing->line+": Error: "+
             (stringp(err)?err:describe_backtrace(err))+"\n" );
  };

  foreach( sort( indices( cls->functions ) ), string fun )
    output_thing( cls->functions[ fun ] );

  foreach( sort( indices( cls->members ) ), string mem )
    output_thing( cls->members[ mem  ] );

  // Processing done. Actually write the file.
  write_file( dir + filename( cls ), current_data );
  files += ({ filename( cls ) });
}

string protos = "";

protected void build_protos( Class cls, int lvl )
{
  if( cls->name != "global" )
    protos+=("  "*lvl)+"EXTPRG struct program *"+glue_c_name(cls->c_name())+"_program;\n";
  foreach( sort( indices( cls->functions ) ), string f )
    protos += ("  "*lvl)+ " extern "+cls->functions[ f ]->c_prototype();
  foreach( sort( indices( cls->members ) ), string f )
    protos += ("  "*lvl)+ " extern "+cls->members[ f ]->c_prototype();
}

int init_n;
string initfun = "", exitfun="", type_switch="", toplevel ="";
protected void build_pike_fadds( Class cls, int lvl )
{
  string res = "";
  init_n++;

  if( mixed e = catch
  {
    void output_thing( mixed thing )
    {
      if( mixed err=catch( res += thing->pike_add() ) )
        werror(thing->file+":"+thing->line+": Error: "+
               (stringp(err)?err:describe_backtrace(err))+"\n" );
    };

    if( cls->name != "_global" )
    {
      exitfun += "  free_program( "+glue_c_name(cls->c_name())+"_program );\n";
      if( !cls->mixin_for )
	type_switch =
	            "#ifdef "+cls->c_type_define()+"\n"
	            "  if(PGTK_CHECK_TYPE(widget, "+cls->c_type_define()+"))\n"
		    "     return "+glue_c_name(cls->c_name())+"_program;\n"
                    "#endif\n"+type_switch;
    }
    res = "static void _"+init_n+"()\n{\n";

    if( cls->name != "_global" )
    {
      res +=
	"   start_new_program(); /* "+cls->name+" */\n" +
	"   Pike_compiler->new_program->id = " + cls->class_id() + ";\n";
      if( sizeof(cls->inherits) )
	foreach( cls->inherits, Class c )
	  res += "  low_inherit( "+glue_c_name(c->c_name())+
                 "_program,0,0,0,0,0,0);\n";
      else
      {
        res += "  ADD_STORAGE(struct "+
	  (cls->mixin_for? "mixin_wrapper":"object_wrapper")+
	  ");\n";
      }
    }

    foreach( sort( indices( cls->functions ) ), string f )
      output_thing( cls->functions[ f ] );
    foreach( sort( indices( cls->members ) ), string f )
      output_thing( cls->members[ f ] );
    if( cls->name != "_global" )
    {
      res += ("  "+glue_c_name(cls->c_name())+"_program = end_program();\n");

      res += ("  add_program_constant("+S(cls->pike_name(),1,0,26)+",\n"
	      "                       "+glue_c_name(cls->c_name())+"_program, ID_FINAL);\n");
//      predef::write("pike_name==%s\n",cls->pike_name());
/*
      string str=S(cls->pike_name(),1,0,26);
      predef::write("pike_name==%s\n",str);
      array aaa=str/"\n";
      string str1;
      sscanf(aaa[1],"%*[ \t](%s)",str1);
      predef::write("PSTR==%s\n",str1);
*/
    }
    res += "}\n\n";
    toplevel += res;

    if( cls->name == "_global" )
    {
      if(sizeof(cls->init))
	initfun += "  pgtk2__init();\n";
      if(sizeof(cls->exit))
	exitfun += "  pgtk2__exit();\n";
    }
  } )
    werror(cls->file+":"+cls->line+": Error: "+
           (stringp(e)?e:describe_backtrace(e))+"\n" );

}

string make_initfun()
{
  string res = "";
  string line = "";
  for( int i = 1; i<=init_n; i++ )
  {
    line += "_"+i+"(); ";
    if( sizeof( line ) > 70 )
    {
      res += "  "+line+"\n";
      line = "";
    }
  }
  if( sizeof(line) )
    res += "  "+line+"\n";
  return res;
}
void post_class_build()
{
}

array(string) output( mapping(string:Class) classes,
                      mapping(string:Constant) constants,
                      array(Node) global_code,
                      mapping(string:int) strings)
{
  head = Stdio.read_bytes( combine_path( sdir, "../pgtk.c.head" ) );
  if(!head) error("Failed to load ../pgtk.c.head\n");
  sfhead = replace( head, "PROTOTYPES", "#include \"prototypes.h\"" );
  traverse_class_tree( classes, output_class );
  post_class_build();
  traverse_class_tree( classes, build_protos );
  write_file( dir + "prototypes.h",  protos );
  write_file( dir + "time_stamp", (string)time() );

  string pre = "#define EXTPRG\n"+sfhead+"\n", res = "";
  
  pre += Parser.Pike.simple_reconstitute( global_code );

  // This code is here to optimize the global string data.
  // Basically, make sure that the constants/classes/functions etc.
  // is added in reverse length order.
  foreach( sort(indices(constants)), string c )
    if( mixed e = catch( initfun += constants[c]->pike_add()) )
      werror(constants[c]->file+":"+constants[c]->line+": Error: "+
             (stringp(e)?e:describe_backtrace(e))+"\n" );

  array q = ({});
  foreach( classes;; object c )
    q |= indices(c->functions) | indices(c->members);
  q = Array.uniq2( sort(q) ); sort(map(q,sizeof),q);
  foreach( reverse(q), string w ) S(w);
  
  traverse_class_tree( classes, build_pike_fadds );


  mapping done = ([]);
  foreach( classes;; object c )
    foreach( c->signals; string n; object s )
      if( !done[n] )
        done[n] = s;

  q = sort(indices( done )); sort(map(q,sizeof),q);
  foreach( reverse(q), string w ) S("s_"+w,1);

  foreach( sort(indices( done )), string w )
    initfun += done[w]->pike_add();

  if(sizeof(strings))
  {
    pre += "struct pike_string * pgtk2_pstr_vector["+sizeof(strings)+"];\n\n";
    foreach( strings; string str; int idx )
    {
      initfun += sprintf("\n  /* %O */\n", str);
      initfun += "  pgtk2_pstr_vector[" + idx + "] = make_shared_static_string(" +
        S(str,1,2) + "," + sizeof(str) + ",0);\n";
    }
    exitfun += #"
  {
    int i;
    for( i=0; i<NELEM(pgtk2_pstr_vector); i++ )
      free_string( pgtk2_pstr_vector[i] );
  }
";
  }

  pre += get_string_data()+"\n\n";
  files = ({ "pgtk.c" }) + files;

  write_file( dir+"pgtk.c",
              pre + toplevel +
              "PIKE_MODULE_INIT {\n"+make_initfun()+initfun+"}\n\n"
              "PIKE_MODULE_EXIT {\n"+exitfun+"}\n\n"
	      "struct program *pgtk2_type_to_program(GObject *widget)\n"
              "{\n"
	      "  if (!widget) return pg2_object_program;\n"
              +type_switch+
	      "  return pg2_object_program;\n}\n\n" );

  return files;
}

int up_to_date( )
{
  int last_time = (int)Stdio.read_bytes( dir+"time_stamp" );
  if(!last_time )
    return 0;
  foreach( map(get_dir( sdir ), lambda(string s){return sdir+s;})|
           ({ __FILE__, combine_path( __FILE__, "../../build_pgtk.pike" )}),
           string f )
  {
    Stdio.Stat s;
    if( !(s = file_stat( f ) ) ) {
      werror("Failed to stat "+f+"\n");
      return 0;
    }
    if( s->mtime > last_time )
      return 0;
  }
  return 1;
}


protected void create( string _s, string _d, object p )
{
  sdir = _s;
  dir = _d;
  parent = p;
}

