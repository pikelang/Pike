mapping(string:string) global_groupings=(["}":"{",")":"(","]":"["]);

array(string) split(string data)
{
  int start;
  int line=1;
  array(string) ret=({});
  int pos;
  data+="\0";

  while(1)
  {
    int start=pos;

//    werror("::::%c\n",data[pos]);

    switch(data[pos])
    {
      case '\0':
	return ret;

      case '#':
      {
	pos=search(data,"\n",pos);
	if(pos==-1)
	  error("Failed to find end of preprocessor statement.\n");

	while(data[pos-1]=='\\') pos=search(data,"\n",pos+1);
	break;

      case 'a'..'z':
      case 'A'..'Z':
      case '_':
	while(1)
	{
	  switch(data[pos])
	  {
	    case 'a'..'z':
	    case 'A'..'Z':
	    case '0'..'9':
	    case '_':
	      pos++;
	      continue;
	  }
	  break;
	}
	break;

      case '.':
	if(data[start..start+2]=="...")
	{
	  pos+=2;
	  break;
	}

      case '0'..'9':
	if(data[pos]=='0' && (data[pos+1]=='x' || data[pos+1]=='X'))
	{
	  pos+=2;
	  while(1)
	  {
	    switch(data[pos])
	    {
	      case '0'..'9':
	      case 'a'..'f':
	      case 'A'..'F':
		pos++;
		continue;
	    }
	    break;
	  }
	  break;
	}
	while(data[pos]>='0' && data[pos]<='9') pos++;
	if(data[pos]=='.')
	{
	  pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	  if(data[pos]=='e' || data[pos]=='E')
	  {
	    pos++;
	    while(data[pos]>='0' && data[pos]<='9') pos++;
	  }
	}
	break;

      default:
	werror("Unknown token %s\n",data[pos..pos+5]);
	exit(1);

      case  '`':
	while(data[pos]=='`') data[pos]++;

      case '/':
      case '{': case '}':
      case '[': case ']':
      case '(': case ')':
      case ';':
      case ',':
      case '*': case '%':
      case '?': case ':':
      case '&': case '|': case '^':
      case '!': case '~':
      case '=':
      case '+':
      case '-':
      case '<': case '>':
	switch(data[pos..pos+1])
	{
	  case "//":
	    pos=search(data,"\n",pos);
	    break;

	  case "/*":
	    pos=search(data,"*/",pos);
	    pos+=2;
	    break;

	  case "<<": case ">>":
	    if(data[pos+2]=='=') pos++;
	  case "==": case "<=": case ">=":
	  case "*=": case "/=": case "%=":
	  case "&=": case "|=": case "^=":
	  case "+=": case "-=":
	  case "++": case "--":
	  case "&&": case "||":
	  case "->":
	    pos++;
	  default:
	    pos++;
	}
	break;


      case ' ':
      case '\n':
      case '\r':
      case '\t':
	while(1)
	{
	  switch(data[pos])
	  {
	    case ' ':
	    case '\n':
	    case '\r':
	    case '\t':
	      pos++;
	      continue;
	  }
	  break;
	}
	break;

	case '\'':
	  pos++;
	  if(data[pos]=='\\') pos++;
	  pos=search(data, "'", pos)+1;
	  break;

	case '"':
	{
	  int q,s;
	  while(1)
	  {
	    q=search(data,"\"",pos+1);
	    s=search(data,"\\",pos+1);
	    if(q==-1) q=strlen(data)-1;
	    if(s==-1) s=strlen(data)-1;

	    if(q<s)
	    {
	      pos=q+1;
	      break;
	    }else{
	      pos=s+1;
	    }
	  }
	  break;
	}
      }
    }

    ret+=({ data[start..pos-1] });
  }
}


class Token
{
  int line;
  string text;
  string file;

  void create(string t, int l, void|string f)
    {
      text=t;
      line=l;
      file=f;
    }

  string _sprintf(int how)
    {
      switch(how)
      {
	case 's':
	  return text;
	case 'O':
	  return sprintf("Token(%O,%O,%d)",text,file,line);
      }
    }

  int `==(mixed foo)
    {
      return (objectp(foo) ? foo->text : foo) == text;
    }

  string `+(string ... s)
    {
      return predef::`+(text,@s);
    }

  string ``+(string ... s)
    {
      return predef::`+(@s,text);
    }

  mixed _cast(string to)
    {
      if(to=="string") return text;
    }
}

array(Token) tokenize(array(string) s, void|string file)
{
  array(Token) ret=allocate(sizeof(s));
  int line=1;
  for(int e=0;e<sizeof(s);e++)
  {
    ret[e]=Token(s[e],line,file);
    line+=sizeof(s[e]/"\n")-1;
  }
  return ret;
}


array group(array(Token) tokens, void|mapping groupings)
{
  array(Token) ret=({});
  if(!groupings) groupings=global_groupings;
  foreach(tokens, Token token)
  {
    ret+=({ token });
    if(string rev=groupings [ token->text ])
    {
      for(int q=sizeof(ret)-1;q>=0;q--)
      {
	if(ret[q] == rev)
	{
	  ret=ret[..q-1]+({ ret[q..] });
	  break;
	}
      }
    }
  }
  return ret;
}

/* This module must work with Pike 7.0 */
#if constant(Array.flatten)
#define FLATTEN Array.flatten
#else
#define FLATTEN flatten
array flatten(array a)
{
  array ret=({});
  foreach(a, a) ret+=arrayp(a)?flatten(a):({a});
  return ret;
}
#endif

string simple_reconstitute(array(Token) tokens)
{
  return FLATTEN(tokens->text) * "";
}

string reconstitute_with_line_numbers(array(string|object(Token)|array) tokens)
{
  int line=1;
  string file;
  string ret="";
  foreach(FLATTEN(tokens), mixed tok)
    {
      if(objectp(tok))
      {
	if((tok->line && tok->line != line) ||
	   (tok->file && tok->file != file))
	{
	  if(strlen(ret) && ret[-1]!='\n') ret+="\n";
	  ret+=sprintf("#line %d %O\n",tok->line,tok->file);
	  line=tok->line;
	  file=tok->file;
	}
	tok=tok->text;
      }
      ret+=tok;
      line+=sizeof(tok/"\n")-1;
    }

  return ret;
}
