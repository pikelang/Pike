/* Note: This is not yet a test-suite. */

class Throttler
{
  void request( Shuffler.Shuffle s,
		int amount,
		function(int:void) callback )
  {
    call_out( callback, 0.1, 10 );
  }

  void give_back( Shuffler.Shuffle s, int amount )
  {
  }
}

int main(int argc, array argv)
{
  Shuffler.Shuffler s = Shuffler.Shuffler( );

  s->set_throttler( Throttler() );
  Shuffler.Shuffle sf = s->shuffle( Stdio.stdout );

  foreach( argv[1..], string x )
  {
    Stdio.File fd = Stdio.File( );
    if( fd->open( x, "r" ) )
      sf->add_source( fd );
    else
      sf->add_source( x+": No such file\n" );
  }

  sf->set_done_callback( lambda(){ exit(0); } );
  sf->start();

  return -1;
}
