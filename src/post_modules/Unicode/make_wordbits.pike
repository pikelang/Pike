#include "hsize.h"

int parse_type( string s )
{
  switch( s[0] )
  {
    case 'L':
      return 1;
    case 'N':
      return 1;
    case 'M':
      if( s == "Mn" )
	return 1;
    default:
      return 0;
  }
}

void main()
{
  multiset is_wordchar = (<>);
  int last_was,c;

  write( "static const struct {\n"
	 "  int start; int end;\n"
	 "} ranges[] = {\n" );
  foreach( Stdio.stdin.read()/"\n", string line )
  {
    sscanf( line, "%s#", line );
    if( !sizeof( line ) )
      continue;
    array data = line / ";";
    if( sizeof( data ) != 15 )
      continue;

    if( sscanf( data[0], "%x", c ) && data[1][0] != '<' )
      if( parse_type( data[2] ) )
      {
	if( !sizeof(data[5]) && !last_was )
	  last_was = c;
      }
      else if( last_was )
      {
	write( "  {0x%06x,0x%06x},\n", last_was, c-1 );
	last_was=0;
      }
  }
  if( last_was )
  {
    write( "  {0x%06x,0x%06x}\n", last_was, c );
    last_was=0;
  }
  write( "};\n");
}
