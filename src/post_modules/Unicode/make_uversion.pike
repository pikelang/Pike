
void main(int n, array args)
{
  werror("\n\n%O\n\n", args);
  if(n!=2) exit(1);
  string file = Stdio.read_file(args[1]);
  if(!file) exit(1);
  string ver;
  sscanf(file, "%*sVersion %s\n", ver);
  if(!ver) exit(1);
  ver = String.trim_all_whites(ver);
  if(!sizeof(ver)) exit(1);
  write("#define UVERSION %O\n", ver);
}
