int main(int argc, array(string) argv)
{
  if(argc<2) { werror("Usage: pike -x pv files...\n"); return 1; }
  Tools.PV(argv[1..][*]);
  return -1;
}
