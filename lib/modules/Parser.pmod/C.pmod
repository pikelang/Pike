//
// This file needs to support old pikes that e.g. don't understand
// "#pike". Some of them fail when they see an unknown cpp directive.
//
// #pike __REAL_VERSION__
//
// $Id: C.pmod,v 1.43 2004/04/04 17:32:26 mast Exp $

//! Splits the @[data] string into an array of tokens. An additional
//! element with a newline will be added to the resulting array of
//! tokens. If the optional argument @[state] is provided the split
//! function is able to pause and resume splitting inside /**/ tokens.
//! The @[state] argument should be an initially empty mapping, in
//! which split will store its state between successive calls.
array(string) split(string data, void|mapping state)
{
  int start;
  int line=1;
  array(string) ret=({});
  int pos;
  if(data=="") return ({"\n"});
  data += "\n\0";	/* End sentinel. */

  if(state && state->in_token) {
    switch(state->remains[0..1]) {

    case "/*":
      if(sizeof(state->remains)>2 && state->remains[-1]=='*'
	 && data[0]=='/') {
	ret += ({ state->remains + "/" });
	pos++;
	m_delete(state, "remains");
	break;
      }
      pos = search(data, "*/");
      if(pos==-1) {
	state->in_token = 1;
	state->remains += data[..sizeof(data)-2];
	return ret;
      }
      ret += ({ state->remains + data[..pos+1] });
      m_delete(state, "remains");
      pos+=2;
      break;
    }
    state->in_token = 0;
  }

  while(1)
  {
    int start=pos;

    switch(data[pos])
    {
      case '\0':
	return ret;

      case '#':
      {
	pos=search(data,"\n",pos);
	if(pos==-1)
	  error("Failed to find end of preprocessor statement.\n");

	while(data[pos-1]=='\\' || (data[pos-1]=='\r' && data[pos-2]=='\\'))
	  pos=search(data,"\n",pos+1);
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
	    if(data[pos]=='-') pos++;
	    while(data[pos]>='0' && data[pos]<='9') pos++;
	  }
	  break;
	}
	if(data[pos]=='e' || data[pos]=='E')
	{
	  pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	}
	break;

      default:
	error("Unknown token %O\n",data[pos..pos+20]);

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
	    if(pos==-1) {
	      if(state) {
		state->remains = data[start..sizeof(data)-3];
		state->in_token = 1;
		return ret;
	      }
	      error("Failed to find end of comment.\n");
	    }
	    pos+=2;
	    break;

	  case "<<": case ">>":
	    if(data[pos+2]=='=') pos++;
	  case "==": case "!=": case "<=": case ">=":
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
	  if(data[pos]=='\\') pos+=2;
          int end=search(data, "'", pos)+1;
          if(!end)
            throw( ({sprintf("Unknown token: %O\n",data[pos-1..pos+19]) }) );
          pos=end;
          break;

	case '"':
	{
	  int q,s;
	  while(1)
	  {
	    q=search(data,"\"",pos+1);
	    s=search(data,"\\",pos+1);
	    if(q==-1) q=sizeof(data)-1;
	    if(s==-1) s=sizeof(data)-1;

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

//! Represents a C token, along with a selection of associated data and
//! operations.
class Token
{
  //! The line where the token was found.
  int line;

  //! The actual token.
  string text;

  //! The file in which the token was found.
  string file;

  //! Trailing whitespaces.
  string trailing_whitespaces="";

  //! @decl void create(string text, void|int line, void|string file,@
  //!   void|string trailing_whitespace)
  void create(string t, void|int l, void|string f, void|string space)
    {
      text=t;
      line=l;
      file=f;
      if(space) trailing_whitespaces=space;
    }

  //! If the object is printed as %s it will only output its text contents.
  string _sprintf(int how)
    {
      switch(how)
      {
	case 's':
	  return text;
	case 'O':
	  return sprintf("%O(%O,%O,%d)",this_program,text,file,line);
      }
    }

  //! Tokens are considered equal if the text contents are equal. It
  //! is also possible to compare the Token object with a text string
  //! directly.
  int `==(mixed foo)
    {
      return (objectp(foo) ? foo->text : foo) == text;
    }

  //! A string can be added to the Token, which will be added to the
  //! text contents.
  string `+(string ... s)
    {
      return predef::`+(text,@s);
    }

  //! A string can be added to the Token, which will be added to the
  //! text contents.
  string ``+(string ... s)
    {
      return predef::`+(@s,text);
    }

  //! It is possible to case a Token object to a string. The text content
  //! will be returned.
  mixed cast(string to)
    {
      if(to=="string") return text;
    }

  //! Characters and ranges may be indexed from the text contents of the token.
  int|string `[](int a, void|int b) {
    if(zero_type(b)) return text[a];
    return text[a..b];
  }
}

//! Returns an array of @[Token] objects given an array of string tokens.
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

static constant global_groupings = ([ "{":"}", "(":")", "[":"]" ]);

//! Fold sub blocks of an array of tokens into sub arrays,
//! for grouping purposes.
//! @param tokens
//!   The token array to fold.
//! @param groupings
//!   Supplies the tokens marking the boundaries of blocks to fold.
//!   The indices of the mapping mark the start of a block, the
//!   corresponding values mark where the block ends. The sub arrays
//!   will start and end in these tokens. If no groupings mapping is
//!   provided, {}, () and [] are used as block boundaries.
array(Token|array) group(array(string|Token) tokens,
			 void|mapping(string:string) groupings)
{
  ADT.Stack stack=ADT.Stack();
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
	  werror ("%s:%d: Expected %O, got %O\n",
		  token->file, token->line,
		  groupings[(string)ret[0]], (string) token);
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

//! Strips off all (preprocessor) line statements from a token array.
array(Token|array) strip_line_statements(array(Token|array) tokens)
{
  array(Token|array) ret=({});
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

//! Folds all whitespace tokens into the previous token's trailing_whitespaces.
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

//! Reconstitutes the token array into a plain string again; essentially
//! reversing @[split()] and whichever of the @[tokenize], @[group] and
//! @[hide_whitespaces] methods may have been invoked.
string simple_reconstitute(array(string|object(Token)|array) tokens)
{
  string ret="";
  foreach(Array.flatten(tokens), mixed tok)
    {
      if(objectp(tok))
	tok=tok->text + tok->trailing_whitespaces;
      ret+=tok;
    }

  return ret;
}

//! Like @[simple_reconstitute], but adding additional @tt{#line n "file"@}
//! preprocessor statements in the output whereever a new line or
//! file starts.
string reconstitute_with_line_numbers(array(string|object(Token)|array) tokens)
{
  int line=1;
  string file;
  string ret="";
  foreach(Array.flatten(tokens), mixed tok)
    {
      if(objectp(tok))
      {
	if((tok->line && tok->line != line) ||
	   (tok->file && tok->file != file))
	{
	  if(sizeof(ret) && ret[-1]!='\n') ret+="\n";
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
