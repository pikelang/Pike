
#pike __REAL_VERSION__

#if constant(GTK.Window)
constant description = "Pike image viewer (diet).";
int main(int argc, array(string) argv)
{
  if(argc<2) { werror("Usage: pike -x pv files...\n"); return 1; }
  Tools.PV(argv[1..][*])->signal_connect("destroy", lambda() {
						      if(--argc<2) exit(0);
						    });
  return -1;
}
#endif
