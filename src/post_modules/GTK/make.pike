string process_argv( string from )
{
  if( from == "wj" ) return 0;
  if( from == "w" ) return 0;
  if( from == "--") return 0;
  if( from == "1") return 0;
  return replace( from , "-g", "" );
}

int main( int argc, array argv )
{
  exit(Process.create_process( map( argv[1..], process_argv ) -({0}) )->wait());
}
