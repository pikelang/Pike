
string glue_c_name (string c_name)
{
  sscanf (c_name, "%s_%s", string prefix, c_name);
  return "p" + prefix + "2_" + c_name;
}

void describe_class_tree( Class c )
{
  mapping circular = ([]);
  
/*
  while( c->inherits && !circular[c]++ )
    c = c->inherits;
  if( circular[c] )
  {
    foreach( indices(circular), Class c )
      werror(c->file+":"+c->line+":\tCircular class tree!\n");
    exit(1);
  }
  circular = ([]);
*/

  void rec_describe_tree( Class c, int lvl, array tree )
  {
    werror(("  "*lvl)+c->file+":"+c->line+":\t!\n");
/*
    if( circular[ c ]++ )
    {
      werror(c->file+":"+c->line+":\tRecursive class tree!\n");
      exit(1);
    }
*/
    array q = c->references->c;
    sort( q->name, q );
    foreach( q, c )
      rec_describe_tree( c, lvl+1, tree+({c}) );
  };
  rec_describe_tree( c, 1, ({c}) );
}

int is_close_enough( string a, string b )
{
  array q = (a/"\n"-({""}));
  array w = (b/"\n"-({""}));
  return equal( sort(q),sort(w) );
}

// Like Stdio.write_file, but does not overwrite with identical data, 
// thus avoiding unessesary recompilations.
void write_file( string file, string data )
{
  string trim( string f ){
    array q = f/"/";
    return q[sizeof(q)-2..]*"/";
  };
  Stdio.File f = Stdio.File();

  if( f->open( file, "r" ) )
    if( !is_close_enough(f->read(),data) )
      werror("  Updating "+trim(file)+"\n");
    else
      return;
  else
    werror("  Creating "+trim(file)+"\n");

  f = Stdio.File( );
  if( !f->open( file, "wct" ) )
  {
    werror("Failed to open "+file+"\n");
    exit(1);
  }
  f->write( data );
}

void traverse_class_tree( mapping(string:Class) classes, function cb )
{
  mapping done = ([]);
  void output_class( Class c, int lvl, array(Class) tree )
  {

    if( done[ c ]++ )
    {
/*
      foreach( tree, Class c )
        werror(c->file+":"+c->line+":\tRecursive class tree!\n");
      exit(1);
*/
      return;
    }

    cb( c, lvl );
    array q = c->references->c;
    sort( q->name, q );
    foreach( q, c )
      if( min(1, @rows(done, c->inherits)) )
	output_class( c, lvl+1,tree+({c}) );
  };

  foreach( sort(indices(classes)), string cls ) {
    if( !sizeof(classes[cls]->inherits) ) {
      output_class( classes[cls], 0,({classes[cls]}) );
    }  else {
//      write("class==%s  inherits==%O\n",cls,classes[cls]->inherits);
    }
  }
  foreach( values(classes), object c )
    if( !done[c] )
    {
      werror(c->file+":"+c->line+":\tOrphan class\n");
      describe_class_tree( c );
      exit(1);
    }
}
