
void main()
{
  string a, b;
  mixed from, to;
  mapping repl=([]);
  foreach(Stdio.read_bytes("keysyms")/"\n", string line)
    if(sscanf(replace(line,"\t", " ")-" ", "%s:%s", a, b)==2)
      repl[a]=("0x"+b);

  mapping patterns = ([]);

  foreach(Stdio.read_bytes("compose")/"\n", 
	  string line)
  {
    if(sscanf(line, "%s:%s", from, to)==2)
    {
      from = replace(from,"\t"," ")/" "-({""});
      for(int i=0;i<sizeof(from); i++)
      {
	from[i] -= " ";
	from[i] -= " ";
	if(!repl[from[i]])
	  werror("Unknown keysym: '"+from[i]+"'\n");
	from[i] = repl[from[i]];
      }
      to -= " ";
      if(!repl[to])
	werror("Unknown keysym: '"+to+"'\n");
      to = repl[to];
      string p = "";
      int res = (int)to;
      for(int i=0;i<sizeof(from)-1; i++)
      {
	p+=sprintf("%4c", (int)from[i]);
	patterns[p]=({}); // continue..
      }
      p+=sprintf("%4c", (int)from[i]);
      patterns[p]=res;
    }
  }
  werror("Have "+sizeof(patterns)+" compose rules\n");
  write(encode_value(patterns));
}
