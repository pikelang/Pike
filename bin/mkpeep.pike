#!/usr/local/bin/pike

#include <simulate.h>

string skipwhite(string s)
{
#if DEBUG > 9
  perror("skipwhite("+s+")\n");
#endif

  sscanf(s,"%*[ \t\n]%s",s);
  return s;
}

/* Find the matching parenthesis */
int find_end(string s)
{
  int e,parlvl=1;

#if DEBUG > 8
  perror("find_end("+s+")\n");
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
  perror("Syntax error.\n");
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
  perror("split("+s+")\n");
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
    perror("Syntax error.\n");
    return 0;
  }

  a=b[..i-1];
  b=b[i+1..];

  for(e=0;e<sizeof(a);e++)
    if(a[e][0]=='F')
      opcodes++;

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
      a[e+1]=replace(a[e+1],"==","!=");
      a=a[..e-1]+a[e+1..];
      e--;
    }
  }

#ifdef DEBUG
  perror(sprintf("%O\n",({a,b})));
#endif

  return ({a,b,opcodes, line});
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
      perror("Syntax error.\n");
      exit(2);
    }
    num--;
    switch(type)
    {
    case 'a': tmp[e]="argument("+num+")"+rest; break;
    case 'o': tmp[e]="opcode("+num+")"+rest; break;
    }
  }
  return implode(tmp,"");
}

/* Dump C co(d|r)e */
void dump2(mixed *data,int ind)
{
  int e,i,max,maxe;
  mixed a,b,d,tmp;
  string test;
  int wrote_switch;
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
    foreach(m_values(foo),d)
    {
      if(m_sizeof(d)>max)
      {
	max=m_sizeof(d);
	maxe=e;
      }
      e++;
    }

    /* If zero, done */
    if(max <= 1) break;

    test=m_indices(foo)[maxe];
    
    wrote_switch++;
    write(sprintf("%*nswitch(%s)\n",ind,treat(test)));
    write(sprintf("%*n{\n",ind));

    d=m_values(foo)[maxe];
    a=m_indices(d);
    b=m_values(d);


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

    if(sizeof(data))
      write(sprintf("%*ndefault:\n",ind));
    ind+=2;
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
	test=treat(implode(d[0]," && "));
	write(sprintf("%*nif(%s)\n",ind,test));
      }
      write(sprintf("%*n{\n",ind));
      ind+=2;


      for(i=0;i<sizeof(d[1]);i++)
      {
	if(i+1<sizeof(d[1]) && d[1][i+1][0]=='(')
	{
	  string tmp=d[1][i+1];
	  tmp=treat(tmp[1..strlen(tmp)-2]);
	  write(sprintf("%*nINT32 arg%d=%s;\n",ind,i,tmp));
	  i++;
	}
      }

      if(sizeof(d[1]))
      {
	write(sprintf("%*ncopy_shared_string(current_file,instr(0)->file);\n",ind));
	write(sprintf("%*ncurrent_line=instr(0)->line;\n",ind));
      }

      write(sprintf("%*npop_n_opcodes(%d);\n",ind,d[2]));

      int q;

      for(i=0;i<sizeof(d[1]);i++)
      {
	if(i+1<sizeof(d[1]) && d[1][i+1][0]=='(')
	{
	  write(sprintf("%*ninsert(%s,arg%d);\n",ind,d[1][i],i));
	  i++;
	}else{
	  write(sprintf("%*ninsert2(%s);\n",ind,d[1][i]));
	}
	q++;
      }
      if(sizeof(d[1]))
      {
	if(q)
	  write(sprintf("%*nfifo_len+=%d;\n",ind,q));
	write(sprintf("%*nfree_string(current_file);\n",ind));
	write(sprintf("%*ndebug();\n",ind));
      }
      write(sprintf("%*ncontinue;\n",ind));

      ind-=2;
      write(sprintf("%*n}\n",ind,test));
    }
  }

  while(wrote_switch--)
  {
    ind-=2;
    write(sprintf("%*n}\n",ind));
  }
}
  


int main(int argc, string *argv)
{
  int e,max,maxe;
  string f;
  mapping foo=([]);
  array(array(array(string))) data=({});

  mapping tests=([]);

  f=read_bytes(argv[1]);
  foreach(explode(f,"\n"),f)
  {
    string *a,*b;
    mapping tmp;

    sscanf(f,"%s#",f);
    if(!strlen(f)) continue;
    data+=({split(f)});
  }

//  write(sprintf("%O\n",data));

  write("  len=instrbuf.s.len/sizeof(p_instr);\n");
  write("  instructions=(p_instr *)instrbuf.s.str;\n");
  write("  instrbuf.s.str=0;\n");
  write("  fifo_len=0;\n");
  write("  init_bytecode();\n\n");
  write("  for(eye=0;eye<len;)\n  {\n");
  write("    INT32 current_line;\n");
  write("    struct pike_string *current_file;\n");
  write("\n");

  dump2(data,4);

  write("    advance();\n");
  write("  }\n");
  write("  for(eye=0;eye<len;eye++) free_string(instructions[eye].file);\n");
  write("  free((char *)instructions);\n");

  return 0;
}

