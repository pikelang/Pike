inherit "split";

#define LIMIT 150*1024
#define OVERFLOW 16*1024

array files = ({});
String.Buffer current_data = String.Buffer(LIMIT + OVERFLOW);
int fcount;

void output_current_data()
{
  // Processing done. Actually write the file.
  if( sizeof( current_data ) )
  {
    if( sizeof(current_data)>LIMIT+OVERFLOW )
      werror("WARNING: current_data resized (content size = %d).\n",
	     sizeof(current_data));
    fcount++;
    write_file( dir + "pgtk_"+(fcount)+".c", current_data->get() );
    files += ({ "pgtk_"+(fcount)+".c" });
  }
}

void post_class_build()
{
  output_current_data();
}

static void output_class( Class cls, int lvl )
{
  if( !cls->functions["_sprintf"] )
    cls->create_default_sprintf();

  cls->create_init_exit();

  /* Start output */
  if(!sizeof( current_data ) )
    current_data->add( "#define EXTPRG extern\n", sfhead );

  if( sizeof( cls->pre ) )
    current_data->add( COMPOSE( cls->pre ) );

  void output_thing( object thing )
  {
    if( mixed err=catch( current_data->add(thing->c_definition()) ) )
      werror(thing->file+":"+thing->line+": Error: "+
             (stringp(err)?err:describe_backtrace(err))+"\n" );
  };

  foreach( sort( indices( cls->functions ) ), string fun )
    output_thing( cls->functions[ fun ] );

  foreach( sort( indices( cls->members ) ), string mem )
    output_thing( cls->members[ mem  ] );

  foreach( sort( indices( cls->properties ) ), string prop )
    output_thing( cls->properties[ prop ] );

  if( sizeof( current_data ) > LIMIT )
    output_current_data( );
}
