
void main()
{
  write("static const int _rtl[] = {\n");
  int last_mode;
  foreach( Stdio.stdin.read()/"\n", string line )
  {
    sscanf( line, "%s#", line );
    if( !sizeof( line ) )
      continue;
    array data = line / ";";
    if( sizeof( data ) != 15 )
      continue;

    int mode = (< "R", "AL", "RLE", "RLO" >)[ data[4] ]; // RTL mode

    if( mode != last_mode )
    {
      int c;
      sscanf( data[0], "%x", c );
      write("  0x%04x,\n", c);
      last_mode = mode;
    }
  }
  write("};\n");
}