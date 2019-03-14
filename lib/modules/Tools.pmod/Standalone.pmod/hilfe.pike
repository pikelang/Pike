
#pike __REAL_VERSION__

constant description = "Hubbes Interactive LPC FrontEnd.";

int main(int argc, array(string) argv)
{
  Tools.Hilfe.StdinHilfe( (argv[1..]*" ")/"#" );
  return 0;
}
