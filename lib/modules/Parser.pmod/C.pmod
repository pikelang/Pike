/* This file needs to support pikes that don't understand "#pike".
 * Some of them fail when they see an unknown cpp directive.
 */
/* #pike __REAL_VERSION__ */

mapping(string:string) global_groupings=(["{":"}","(":")","[":"]"]);

array(string) split(string data)
{
  int start;
  int line=1;
  array(string) ret=({});
  int pos;
  data += "\n\0";	/* End sentinel. */

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
      case 128..65536: // Lets simplify things for now...
      case '_':
	while(1)
	{
	  switch(data[pos])
	  {
           case '$': // allowed in some C (notably digital)
           case 'a'..'z':
           case 'A'..'Z':
           case '0'..'9':
           case 128..65536: // Lets simplify things for now...
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
	  pos+=3;
	  break;
	}
	if(data[start..start+1]=="..")
	{
	  pos+=3;
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
	throw( ({sprintf("Unknown token %O\n",data[pos..pos+20]) }) );

      case  '`':
	while(data[pos]=='`') data[pos]++;

      case '\\': pos++; continue; /* IGNORED */

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
      case '@':
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
      case '\14':
	while(1)
	{
	  switch(data[pos])
	  {
	    case ' ':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\14':
	      pos++;
	      continue;
	  }
	  break;
	}
	break;

	case '\'':
	  pos++;
	  if(data[pos]=='\\') pos++;
          int end=search(data, "'", pos+1)+1;
          if(!end)
            throw( ({sprintf("Unknown token %O\n",data[pos..pos+20]) }) );
          pos=end;
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
  string trailing_whitespaces="";

  void create(string t, void|int l, void|string f, void|string space)
    {
      text=t;
      line=l;
      file=f;
      if(space) trailing_whitespaces=space;
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

  mixed cast(string to)
    {
      if(to=="string") return text;
    }
}

/* FIXME:
 */
array(Token) tokenize(array(string) s, void|string file)
{
  array(Token) ret=allocate(sizeof(s));
  int line=1;
  for(int e=0;e<sizeof(s);e++)
  {
    ret[e]=Token(s[e],line,file);
    if(s[e][0]=='#')
    {
      if( (sscanf(s[e],"#%*[ \t\14]%d%*[ \t\14]\"%s\"", line,file) == 4) ||
          (sscanf(s[e],"#%*[ \t\14]line%*[ \t\14]%d%*[ \t\14]\"%s\"",line,file)==5))
        line--;
    }
    line+=sizeof(s[e]/"\n")-1;
  }
  return ret;
}

array group(array(string|Token) tokens, void|mapping groupings)
{
#if constant(ADT.Stack)
  ADT.Stack stack=ADT.Stack();
#else
  Stack.stack stack=Stack.stack();
#endif
  array(Token) ret=({});
  mapping actions=([]);

  if(!groupings) groupings=global_groupings;

  foreach((array)groupings,[string x, string y])
  {
    actions[x]=1;
    actions[y]=2;
  }

  foreach(tokens, Token token)
  {
    switch(actions[(string)token])
    {
      case 0: ret+=({token}); break;
      case 1: stack->push(ret); ret=({token}); break;
      case 2:
	if (!sizeof(ret) || !stack->ptr ||
	    (groupings[(string)ret[0]] != (string)token)) {
	  // Mismatch
	  werror(sprintf("**** Grouping mismatch token=%O\n"
			 "**** tokens: ({ %{%O, %}})\n"
			 "**** ret: ({ %{%O, %}})\n"
			 "**** stackdepth: %d\n",
			 token->text, tokens->text,
			 ret->text, stack->ptr));
	  return ret;
	}
	ret=stack->pop()+({ ret + ({token}) });
    }
  }
  return ret;
}

/* FIXME:
 * This actually strips all preprocessing tokens
 */
array strip_line_statements(array tokens)
{
  array(Token) ret=({});
  foreach(tokens, array|object(Token) t)
    {
      if(arrayp(t))
      {
	ret+=({ strip_line_statements(t) });
      }else{
	if( ((string)t) [0] != '#')
	  ret+=({t});
      }
    }
  return ret;
  
}

array hide_whitespaces(array tokens)
{
  array(Token) ret=({tokens[0]});
  foreach(tokens[1..], array|object(Token) t)
    {
      if(arrayp(t))
      {
	ret+=({ hide_whitespaces(t) });
      }else{
	switch( ((string)t) [0])
	{
	  case ' ':
	  case '\t':
	  case '\14':
	  case '\n':
	    mixed tmp=ret[-1];
	    while(arrayp(tmp)) tmp=tmp[-1];
	    tmp->trailing_whitespaces+=(string)t;
	    break;

	  default:
	    ret+=({t});
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

string simple_reconstitute(array(string|object(Token)|array) tokens)
{
  string ret="";
  foreach(FLATTEN(tokens), mixed tok)
    {
      if(objectp(tok))
	tok=tok->text + tok->trailing_whitespaces;
      ret+=tok;
    }

  return ret;
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
	  line=tok->line;
	  if(tok->file) file=tok->file;
	  ret+=sprintf("#line %d %O\n",line,file);
	}
	tok=tok->text + tok->trailing_whitespaces;
      }
      ret+=tok;
      line+=sizeof(tok/"\n")-1;
    }

  return ret;
}
