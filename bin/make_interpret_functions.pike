#!/usr/local/bin/pike

#ifdef OLD
import ".";
#define PC .Pike
#else /* !OLD */
#if constant(Parser.Pike)
#define PC Parser.Pike
#else /* !constant(Parser.Pike) */
#define PC Parser.C
#endif /* constant(Parser.Pike) */
#endif /* OLD */

mapping ops=([]);

array fixit2(array x)
{
  array ret=({});
  array opcodes=({});
  for(int e=0;e<sizeof(x);e++)
    {
      if(objectp(x[e]) && ops[(string)x[e]])
      {
	opcodes+=x[e..e]+fixit2(x[e+1])[0];
	array tmp=fixit2((x[e+1]/ ({ PC.Token(",") }) )[-1][0]);
	
	ret+=({ tmp[0] });
	opcodes+=tmp[1];
	e++;
      }
      else if(arrayp(x[e]))
      {
	array tmp=fixit2(x[e]);
	ret+=tmp[0];
	opcodes+=tmp[1];
      }
      else
      {
	ret+=({x[e]});
      }
    }
  return ({ ret, opcodes });
}

array fixit(array x)
{
  array ret=({});
  for(int e=0;e<sizeof(x);e++)
    {
      if(objectp(x[e]) && ops[(string)x[e]])
      {
//	werror("DOING %s\n",(string)(x[e+1][1]));
	array tmp=fixit2(x[e+1]);
	ret+=({x[e]}) + tmp[0] + tmp[1];
	e++;

	/* Skip traling ; */
	if(e+1<sizeof(x) && objectp(x[e+1]) && ";" == (string)x[e+1])
	{
	  object tmp=x[e+1];
	  tmp->text="";
	  ret+=({tmp});
	  e++;
	}
      }
      else
      {
	ret+=({x[e]});
      }
    }
  return ret;
}

array filter_out_cases(array x)
{
  array y=x/({ PC.Token("CASE") });
  for(int e=1;e<sizeof(y);e++) y[e]=y[e][1..];
  return y*({});
}

array fix_cases(array x)
{
  int start=0;
  int undone=0;
  array ret=({});

  while(1)
  {
    int casepos=search(x,PC.Token("CASE"), start);
    werror("%O\n",PC.simple_reconstitute(x[casepos..casepos+1]));
    if(casepos == -1) break;
    int donepos=search(x,PC.Token("DONE"),casepos+1);
    
    ret+=x[undone..casepos-1]+
      ({
	PC.Token("OPCODE0"),
	  ({
	    PC.Token("("),
	      x[casepos+1][1],
	      PC.Token(","),
	      PC.Token(sprintf("%O",(string)x[casepos+1][1])),
	      PC.Token(","),
	      ({ PC.Token("{") })+
	      filter_out_cases(x[casepos+2..donepos-1])+
	      ({  PC.Token("}") }),
	      PC.Token(")"),
	      PC.Token(";"),
	      })
	  });
    
    start=casepos+1;
    undone=donepos+1;
  }

  return ret + x[undone..];
}

int main(int argc, array(string) argv)
{
  string file=argv[1];
  mixed x=Stdio.read_file(file);
  x=PC.split(x);
  x=PC.tokenize(x,file);
  x=PC.hide_whitespaces(x);
  x=PC.group(x);

  for(int e=0;e<=2;e++)
    foreach( ({"","_JUMP","_TAIL","_TAILJUMP"}) , string postfix)
      ops[sprintf("OPCODE%d%s",e,postfix)]=1;

//  werror("%O\n",ops);

  x=fix_cases(x);
  x=fixit(x);

  if(getenv("PIKE_DEBUG_PRECOMPILER"))
    write(PC.simple_reconstitute(x));
  else
    write(PC.reconstitute_with_line_numbers(x));
}
