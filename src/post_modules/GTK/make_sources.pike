void main(int argc, array argv)
{
  string dest = getcwd();
  if(!cd(argv[1]))
  {
    werror("*** Cannot CD to "+argv[1]+"\n");
    exit(1);
  }
  string s = "";
  foreach(get_dir("."), string f)
  {
    if(f[0] != '.' && f[0] != '#' && f[-1] != '~' && Stdio.file_size(f)>0)
      s += "source/"+f+" ";
  }
  Stdio.File(dest+"/sources", "wct")->write( s[..strlen(s)-2] );
}
