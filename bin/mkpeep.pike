#!/usr/local/bin/pike

/* $Id: mkpeep.pike,v 1.10 1999/03/13 02:06:56 marcus Exp $ */

#define JUMPBACK 3

string skipwhite(string s)
{
#if DEBUG > 9
  werror("skipwhite("+s+")\n");
#endif

  sscanf(s,"%*[ \t\n]%s",s);
  return s;
}

/* Find the matching parenthesis */
int find_end(string s)
{
  int e,parlvl=1;

#if DEBUG > 8
  werror("find_end("+s+")\n");
#endif
  
  for(e=1;e<strlen(s);e++)
  {
    switch(s[e])
    {
    case '(': case '{': case '[':
      parlvl++; break;
    case ')': case '}': case ']':
      parlvl--;
      if(!parlvl) return e;
      break;
    }
  }
  werror("Syntax error (1).\n");
  exit(1);
}


/* Splitline into components */
mixed split(string s)
{
  string *a,*b,tmp;
  int i,e,opcodes;
  string line=s;
  opcodes=0;

#ifdef DEBUG
  werror("split("+s+")\n");
#endif

  b=({});

  s=skipwhite(s);
  while(strlen(s))
  {
    switch(s[0])
    {
      /* Source / Target separator */
    case ':':
      b+=({":"});
      s=s[1..];
      break;

    case '!':
      b+=({"!"});
      s=s[1..];
      break;

      /* Identifier */
    case 'A'..'Z':
    case 'a'..'z':
    case '0'..'9':
    case '_':
      sscanf(s,"%[a-zA-Z0-9_]%s",tmp,s);
      b+=({"F_"+tmp});
      break;

      /* argument */
    case '(':
      i=find_end(s);
      b+=({ s[0..i] });
      s=s[i+1..strlen(s)];
      break;

      /* condition */
    case '[':
      i=find_end(s);
      b+=({ s[0..i] });
      s=s[i+1..strlen(s)];
      break;
    }

    s=skipwhite(s);
  }

  int i=search(b, ":");
  if(i==-1)
  {
    werror("Syntax error (%O).\n",b);
    return 0;
  }

  a=b[..i-1];
  b=b[i+1..];

  for(e=0;e<sizeof(a);e++)
    if(a[e][0]=='F')
      opcodes++;

#if 0
  /* It was a good idea, but it doesn't work */
  mixed qqqq=copy_value(b);
  i=0;
  while(sizeof(b))
  {
    if(b[0] != a[i])
      break;
    
    if(sizeof(b)>1 && b[1][0]!='F')
    {
      if(b[1] != sprintf("($%da)",i+1))
	break;
      b=b[2..];
    }else{
      b=b[1..];
    }
    i++;
    opcodes--;
  }
  if(i)
    werror("----------------------\n%d\n%O\n%O\n%O\n",opcodes,a,b,qqqq);
#endif

  i=0;
  for(e=0;e<sizeof(a);e++)
  {
    switch(a[e][0])
    {
    case '(':
      a[e]=a[e]+"==$"+i+"a";
      break;

    case '[':
      a[e]=a[e][1..strlen(a[e])-2];
      break;

    case 'F':
      i++;
      a[e]=a[e]+"==$"+i+"o";
      break;
    }
  }


  for(e=0;e<sizeof(a);e++)
  {
    if(a[e]=="!")
    {
      sscanf(a[e+1],"%s==%s",string op, string arg);
      a[e+1]=sprintf("%s != %s && %s !=-1",op,arg,arg);
      a=a[..e-1]+a[e+1..];
      e--;
    }
  }

  return ({a,b,opcodes, line,a});
}

/* Replace $[0-9]+(o|a) with something a C compiler can understand */
string treat(string expr)
{
  int e;
  string *tmp;
  tmp=expr/"$";
  for(e=1;e<sizeof(tmp);e++)
  {
    string num, type, rest;
    if(sscanf(tmp[e],"%d%c%s",num,type,rest)!=3)
    {
      werror("Syntax error (3).\n");
      exit(2);
    }
    num--;
    switch(type)
    {
    case 'a': tmp[e]="argument("+num+")"+rest; break;
    case 'o': tmp[e]="opcode("+num+")"+rest; break;
    }
  }
  return tmp*"";
}

/* Dump C co(d|r)e */
void dump2(mixed *data,int ind)
{
  int e,i,max,maxe;
  mixed a,b,d,tmp;
  string test;
  mapping foo;
  mixed cons, var;

  foo=([]);

  while(1)
  {
    foo=([]);

    /* First we create a mapping:
     * foo [ meta variable ] [ condition ] = ({ lines });
     */
    foreach(data,d)
    {
      a=d[0];
      b=d[1];
      for(e=0;e<sizeof(a);e++)
      {
	if(sscanf(a[e],"F_%[A-Z0-9_]==%s",cons,var)==2 ||
	   sscanf(a[e],"(%d)==%s",cons,var)==2 ||
	   sscanf(a[e],"%d==%s",cons,var)==2)
	{
	  if(!foo[var]) foo[var]=([]);
	  if(!foo[var][a[e]]) foo[var][a[e]]=({});
	  foo[var][a[e]]+=({d});
	}
      }
    }

    /* Check what variable has most values */
    max=maxe=e=0; 
    foreach(values(foo),d)
    {
      if(sizeof(d)>max)
      {
	max=sizeof(d);
	maxe=e;
      }
      e++;
    }

    /* If zero, done */
    if(max <= 1) break;

    test=indices(foo)[maxe];
    
    write(sprintf("%*nswitch(%s)\n",ind,treat(test)));
    write(sprintf("%*n{\n",ind));

    d=values(foo)[maxe];
    a=indices(d);
    b=values(d);


    /* foo: variable
     * a[x] : condition
     * b[x] : line
     */

    for(e=0;e<sizeof(a);e++)
    {
      /* The lines b[e] are removed from data as they
       * will be treated below
       */
      data-=b[e];

      if(sscanf(a[e],"(%s)==%s",cons,var)!=2)
	sscanf(a[e],"%s==%s",cons,var);
      
      write(sprintf("%*ncase %s:\n",ind,cons+""));

      foreach(b[e],d) d[0]-=({a[e]});
      dump2(b[e],ind+2);
      write(sprintf("%*n  break;\n",ind));
      write("\n");
    }

    write(sprintf("%*n}\n",ind));
  }
  
  /* Take care of whatever is left */
  if(sizeof(data))
  {
    foreach(data,d)
    {
      mixed q;

      write(sprintf("%*n/* %s */\n",ind,d[3]));

      
      if(sizeof(d[0]))
      {
	string test;
	test=treat(d[0]*" && ");
	write(sprintf("%*nif(%s)\n",ind,test));
      }
      write(sprintf("%*n{\n",ind));
      ind+=2;
      write("%*ndo_optimization(%d,\n",ind,d[2]);

      for(i=0;i<sizeof(d[1]);i++)
      {
	if(i+1<sizeof(d[1]) && d[1][i+1][0]=='(')
	{
	  string tmp=d[1][i+1];
	  write("%*n                2,%s,%s,\n",ind,d[1][i],treat(tmp[1..strlen(tmp)-2]));
	  i++;
	}else{
	  write("%*n                1,%s,\n",ind,d[1][i]);
	}
      }
      write("%*n                0);\n",ind);

      write(sprintf("%*ncontinue;\n",ind));
      ind-=2;
      write(sprintf("%*n}\n",ind,test));
    }
  }
}
  


int main(int argc, string *argv)
{
  int e,max,maxe;
  string f;
  mapping foo=([]);
  array(array(array(string))) data=({});

  mapping tests=([]);

  f=cpp(Stdio.read_bytes(argv[1]),argv[1]);
  foreach(f/"\n",f)
  {
    string *a,*b;
    mapping tmp;

    sscanf(f,"%s#",f);
    foreach(f/";",f)
      {
	f=skipwhite(f);
	if(!strlen(f)) continue;
	data+=({split(f)});
      }
  }

//  write(sprintf("%O\n",data));

  write("  len=instrbuf.s.len/sizeof(p_instr);\n");
  write("  instructions=(p_instr *)instrbuf.s.str;\n");
  write("  instrbuf.s.str=0;\n");
  write("  fifo_len=0;\n");
  write("  init_bytecode();\n\n");
  write("  for(eye=0;eye<len || fifo_len;)\n  {\n");
  write("    INT32 current_line;\n");
  write("    struct pike_string *current_file;\n");
  write("\n");
  write("#ifdef DEBUG\n");
  write("    if(a_flag>5) {\n");
  write("      fprintf(stderr,\"#%d,%d:\",eye,fifo_len);\n");
  write("      fprintf(stderr,\" %s(%d)\",  get_token_name(opcode(0)),argument(0));\n");
  write("      fprintf(stderr,\" %s(%d)\",  get_token_name(opcode(1)),argument(1));\n");
  write("      fprintf(stderr,\" %s(%d)\",  get_token_name(opcode(2)),argument(2));\n");
  write("      fprintf(stderr,\" %s(%d)\\n\",get_token_name(opcode(3)),argument(3));\n");
  write("    }\n");
  write("#endif\n\n");

  dump2(data,4);

  write("    advance();\n");
  write("  }\n");
  write("  for(eye=0;eye<len;eye++) free_string(instructions[eye].file);\n");
  write("  free((char *)instructions);\n");

  return 0;
}

