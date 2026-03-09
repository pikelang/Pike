#pike __REAL_VERSION__

constant description = "Pike image viewer (diet).";

#if constant(Tools.PV)
int main(int argc, array(string) argv)
{
  if(argc<2) { werror("Usage: pike -x pv files...\n"); return 1; }
#if constant(Tools.PVApp)
  Tools.PVApp()->run(argc, argv);
  return 0;
#else
  Tools.PV(argv[1..][*])->signal_connect("destroy", lambda() {
						      if(--argc<2) exit(0);
						    });
  return -1;
#endif
}
#endif
