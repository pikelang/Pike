#!/usr/local/bin/pike

int main(int argc, array(string) argv)
{
  array(string) funcs=(Stdio.read_file(argv[1])/"\nOPCODE");
  array(int) lineno=allocate(sizeof(funcs));

  lineno[0]=1;

  for(int e=1;e<sizeof(funcs);e++)
    lineno[e]=lineno[e-1]+sizeof(funcs[e-1]/"\n");

  for(int e=1;e<sizeof(funcs);e++)
  {
    int x=e+1;
    while(!sscanf(funcs[e],"%sBREAK",funcs[e]))
    {
      string junk,rest;
      if(!sscanf(funcs[x],"%s\n%s", junk, rest))
      {
	werror("%s:%d: Two opcodes in a row?\n",argv[1],lineno[e]);
	exit(1);
      }
      x++;
      if(search(junk,"TAIL")==-1 &&
	 search((funcs[e]/"\n")[0],"TAIL")==-1)
      {
	werror("Missing TAIL after line %d\n",lineno[e]);
//	werror("junk=%O\n",junk);
	exit(1);
      }
      funcs[e]+=rest;
    }
  }
  for(int e=1;e<sizeof(funcs);e++)
  {
//    werror("funcs[%d]=%O\n",e,funcs[e]);
    int args;
    string type, opcode, name, body;
    if(sscanf(funcs[e],"%d%s(%s,%*[ \t]\"%s\")%s",args,type, opcode, name,  body)!=6)
    {
      werror("%s:%d: Opcode sscanf failed.\n",argv[1],lineno[e]);
      exit(1);
    }
    sscanf(opcode,"%*[ \t]%s",opcode);
    opcode=reverse(opcode);
    sscanf(opcode,"%*[ \t]%s",opcode);
    opcode=reverse(opcode);

    write("# %d %O\n",lineno[e],argv[1]);
    write("void i_%s(",opcode);
    for(int x=0;x<args;x++) write("%sINT32 arg%d",x?", ":"",x+1);
    if(!args) write("void");
    write(") {");
    write(body);
    write("}\n\n");
  }
}
