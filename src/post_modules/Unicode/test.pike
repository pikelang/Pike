#define FILE "NormalizationTest-3.1.0.txt"

#define c1 c[0]
#define c2 c[1]
#define c3 c[2]
#define c4 c[3]
#define c5 c[4]

void main()
{
  int tests, fail, part, opl;
  write("Performing Unicode normalization tests\n");
  write("See http://www.unicode.org/Public/3.1-Update/" FILE "\n" );

  foreach( Stdio.File( FILE,"r" )->read()/"\n", string l )
  {
    if( !strlen( l ) )
      continue;

    if( l[0] == '@' )
    {
      if( opl ) write("Done. "+(tests-opl+1)+" tests.\n" );
      write("\n");
      opl = tests+1;
      write( replace( l[1..], " #", ":") +"\n" );
      part++;
      continue;
    }
    if( !strlen( l ) )
      continue;

    string decode_hex( string d )
    {
      return (string)column(array_sscanf(d,"%{%x%*[ ]%}")[0],0);
    };
    
    array c = map( l/";"[..4], decode_hex );

    string hex( string x )
    {
      return sprintf("%{%04X %}",(array(int))x);
    };

    void test( string ok,
	       string method,
	       string ... t )
    {
      foreach( t, string tt )
	if( Unicode.normalize( tt, method ) != ok )
	{
	  write("\n");
	  write("Test %d/%s failed:\n"
		"expected: %s\n"
		"got:      %s\n"
		"input:    %s\n",tests/6,method,
		 hex(ok), hex(Unicode.normalize(tt,method)), hex(tt));
	  fail++;
	  return;
	}
    };

    
    test( c2, "NFC", c1, c2, c3 );
    test( c4, "NFC", c4, c5 );

    test( c3, "NFD", c1, c2, c3 );
    test( c5, "NFD", c4, c5 );

    test( c4, "NFKC", c1, c2, c3, c4, c5 );
    test( c5, "NFKD", c1, c2, c3, c4, c5 );
    tests += 6;
  }
  write( "Done. "+(tests-opl+1)+" tests.\n" );
  if( fail )
  {
    write( "Summary: %d/%d tests failed\n", fail, tests );
    exit( 1 );
  }
  exit( 0 );
}
