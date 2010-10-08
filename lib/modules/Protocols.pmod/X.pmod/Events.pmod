
class ClientMessage
{
  int window;
  int type;	// Atom
  int x;	// Atom
  int time;
  string to_string()
  {
    return sprintf("%4c%4c%4c%4c%4c%4c%4c%4c",
		   0, window, type, x, time, 0,0,0);
  }
}