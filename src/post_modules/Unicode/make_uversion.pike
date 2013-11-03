
void main(int n, array args)
{
  if(n!=3) exit(1);
  string file = Stdio.read_file(args[1]);
  if(!file) exit(1);
  function write = Stdio.File(args[2], "cwt")->write;
  string ver;
  sscanf(file, "%*sVersion %s\n", ver);
  if(!ver) exit(1);
  ver = String.trim_all_whites(ver);
  if(!sizeof(ver)) exit(1);
  write("#define UVERSION %O\n", ver);
}
