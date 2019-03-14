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

class FakeStream
{
  function cb, ccb;

  void create()
  {
    if( cb )
      cb( 0, "some random data some more random data even more random data\n" );
    call_out( create, 0.1 );
  }
  
  void set_read_callback( function _cb )
  {
    cb = _cb;
  }

  void set_close_callback( function _cb )
  {
    ccb = _cb;
  }
}

class FakeFile
{
  string q = "This is the fake file";
  string read( int len )
  {
    string x = q[..len-1];
    q = q[len..];
    return x;
  }
}

void shuffle_http( Shuffler.Shuffler s )
{
  Shuffler.Shuffle sf = s->shuffle( Stdio.stderr );

  Stdio.File fd = Stdio.File();
  fd->connect( "www.lysator.liu.se", 80 );
  fd->write("GET / HTTP/1.0\r\n"
	    "Host: www.lysator.liu.se\r\n"
	    "User-Agent: Pike\r\n"
	    "\r\n");
  sf->add_source( "The source of http://www.lysator.liu.se/\n");
  sf->add_source( fd );
  sf->add_source( "A faked stream\n" );
  sf->add_source( FakeStream() );
  sf->add_source( "A faked file\n" );
  sf->add_source( FakeFile() );
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
