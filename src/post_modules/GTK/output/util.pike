
void describe_class_tree( Class c )
{
  mapping circular = ([]);
  while( c->inherits && !circular[c]++ )
    c = c->inherits;
  if( circular[c] )
  {
    foreach( indices(circular), Class c )
      werror(c->file+":"+c->line+":\tCircular class tree!\n");
    exit(1);
  }
  circular = ([]);

  void rec_describe_tree( Class c, int lvl, array tree )
  {
    werror(("  "*lvl)+c->file+":"+c->line+":\t!\n");
    if( circular[ c ]++ )
    {
      werror(c->file+":"+c->line+":\tRecursive class tree!\n");
      exit(1);
    }
    array q = c->references->c;
    sort( q->name, q );
    foreach( q, c )
      rec_describe_tree( c, lvl+1, tree+({c}) );
  };
  rec_describe_tree( c, 1, ({c}) );
}

void traverse_class_tree( mapping(string:Class) classes, function cb )
{
  mapping done = ([]);
  void output_class( Class c, int lvl, array(Class) tree )
  {
    if( done[ c ]++ )
    {
      foreach( tree, Class c )
        werror(c->file+":"+c->line+":\tRecursive class tree!\n");
      exit(1);
    }
    cb( c, lvl );
    array q = c->references->c;
    sort( q->name, q );
    foreach( q, c ) output_class( c, lvl+1,tree+({c}) );
  };

  foreach( sort(indices(classes)), string cls )
    if( !classes[cls]->inherits )
      output_class( classes[cls], 0,({classes[cls]}) );
  foreach( values(classes), object c )
    if( !done[c] )
    {
      werror(c->file+":"+c->line+":\tOrphan class\n");
      describe_class_tree( c );
      exit(1);
    }
}
