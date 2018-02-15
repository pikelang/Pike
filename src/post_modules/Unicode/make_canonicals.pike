#include "hsize.h"

void main()
{
  mapping classes = ([]);
  foreach( Stdio.stdin.read()/"\n", string line )
  {
    sscanf( line, "%s#", line );
    if( !sizeof( line ) )
      continue;
    array data = line / ";";
    if( sizeof( data ) != 15 )
      continue;
    int c, cc;
    sscanf( data[0], "%x", c );
    sscanf( data[3], "%d", cc );
    if( cc )
      classes[c] = cc;
  }

  write( "static const struct canonical_cl _ca[] = {\n" );
  mapping top=([]);
  int i;
  foreach( reverse(sort( indices( classes ) )), int c )
    write( "{%d,%d},\n", c, classes[c]);
  write( "};\n" );
}
