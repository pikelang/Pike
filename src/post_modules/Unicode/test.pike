#! /usr/bin/env pike

// $Id$

#define c1 c[0]
#define c2 c[1]
#define c3 c[2]
#define c4 c[3]
#define c5 c[4]

int tests, fail;
array(string) argv = ({"dummy", combine_path (__FILE__, "..")});

array(int) a()
{
  int part, opl;

  foreach( Stdio.File( argv[1]+"/NormalizationTest.txt","r" )->read()/"\n", string l )
  {
    if( !sizeof( l ) || has_prefix(l, "#"))
      continue;

    if( l[0] == '@' )
    {
      if( opl ) werror("Done. "+(tests-opl+1)+" tests.\n" );
      werror("\n");
      opl = tests+1;
      werror( replace( l[1..], " #", ":") +"\n" );
      part++;
      continue;
    }
    if( !sizeof( l ) )
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
	  werror("\n");
	  werror("Test %d/%s failed:\n"
		"expected: %s\n"
		"got:      %s\n"
		"input:    %s\n",tests/6,method,
		 hex(ok), hex(Unicode.normalize(tt,method)), hex(tt));
	  fail++;
	  return;
	}
      tests++;
    };

    
    test( c2, "NFC", c1, c2, c3 );
    test( c4, "NFC", c4, c5 );

    test( c3, "NFD", c1, c2, c3 );
    test( c5, "NFD", c4, c5 );

    test( c4, "NFKC", c1, c2, c3, c4, c5 );
    test( c5, "NFKD", c1, c2, c3, c4, c5 );
  }
  werror( "Done. "+(tests-opl+1)+" tests.\n" );

  return ({tests, fail});
}

void main(int argc, array argv)
{

  werror("Performing Unicode normalization tests\n");
  werror("See http://www.unicode.org/Public/3.2-Update/NormalizationTest-3.2.0.txt\n");
  if( argc<2 || has_value( argv, "--help" ) )
  {
    werror("\nUsage %s <path>\nwhere path is the path to the directory with the NormalizationTest.txt file.\n",
	  argv[0]);
    exit(0);
  }

  global::argv = argv;
  a();
}
