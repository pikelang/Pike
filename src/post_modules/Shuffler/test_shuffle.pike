/* Note: This is not yet a test-suite. */

class Throttler
{
  void request( Shuffler.Shuffle s,
		int amount,
		function(int:void) callback )
  {
    call_out( callback, 0.01, 10 );
  }

  void give_back( Shuffler.Shuffle s, int amount )
  {
  }
}

void shuffle_http( Shuffler.Shuffler s )
{
  Shuffler.Shuffle sf = s->shuffle( Stdio.stderr );

  Stdio.File fd = Stdio.File();
  fd->connect( "www.animenfo.com", 80 );
  fd->write("GET /animefansub/animebydate.php HTTP/1.0\r\n"
	    "Host: www.animenfo.com\r\n"
	    "User-Agent: Pike\r\n"
	    "\r\n");
  sf->add_source( fd );
  sf->start();
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

  sf->set_done_callback( lambda(){shuffle_http(s);} );
  sf->start();

  return -1;
}
