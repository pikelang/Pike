//
// This file needs to support old pikes that e.g. don't understand
// "#pike". Some of them fail when they see an unknown cpp directive.
//
// #pike __REAL_VERSION__
//
// $Id: Pike.pmod,v 1.37 2004/11/26 04:27:25 nilsson Exp $

//! This module parses and tokenizes Pike source code.

inherit "C.pmod";

#define UNKNOWN_TOKEN \
  error("Unknown pike token: %O\n", data[pos..pos+20])

static mapping(string : int) backquoteops =
([ "/":1, "/=":2,
   "%":1, "%=":2,
   "*":1, "*=":2,
   "&":1, "&=":2,
   "|":1, "|=":2,
   "^":1, "^=":2,
   "~":1,
   "+":1, "+=":2,
   "-":1, "-=":2,
   "<<=":3, "<<":2, "<=":2, "<":1,
   ">>=":3, ">>":2, ">=":2, ">":1,
   "!=":2, "!":1,
   "==":2, "=":1,
   "()":2,
   "->=":3, "->":2,
   "[]=":3, "[]":2, "[..]":4 ]);

//! Splits the @[data] string into an array of tokens. An additional
//! element with a newline will be added to the resulting array of
//! tokens. If the optional argument @[state] is provided the split
//! function is able to pause and resume splitting inside #"" and
//! /**/ tokens. The @[state] argument should be an initially empty
//! mapping, in which split will store its state between successive
//! calls.
array(string) split(string data, void|mapping state)
{
  int line=1;
  array(string) ret=({});
  int pos;
  if(data=="") return ({"\n"});
  data += "\n\0"; // End sentinel.

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

    case "#\"":
      int q,s;
      pos=-1;
      while(1) {
	q = search(data,"\"",pos+1);
	s = search(data,"\\",pos+1);

	if( q==-1 ||
	    (s==sizeof(data)-2 && s<q) ) {
	  state->in_token = 1;
	  state->remains += data[..sizeof(data)-3];
	  return ret;
	}

	if(s==-1 || s>q) {
	  pos = q+1;
	  break;
	}

	pos=s+1;
      }
      ret += ({ state->remains + data[..pos-1] });
      m_delete(state, "remains");
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
	pos+=1;

	if(data[pos]=='\"') {
	  int q,s;
	  while(1) {
	    q = search(data,"\"",pos+1);
	    s = search(data,"\\",pos+1);

	    if( q==-1 ||
		(s==sizeof(data)-2 && s<q) ) {
	      if(state) {
		state->in_token = 1;
		state->remains = data[pos-1..sizeof(data)-3];
		return ret;
	      }
	      error("Failed to find end of multiline string.\n");
	    }

	    if(s==-1 || s>q) {
	      pos = q+1;
	      break;
	    }

	    pos=s+1;
	  }
	  break;
	}

	if(data[pos..pos+5]=="string") {
	  pos=((search(data,"\"",pos)+1) || pos);
	  pos=((search(data,"\"",pos)+1) || pos);
	  break;
	}

	pos=search(data,"\n",pos);
	if(pos==-1)
	  error("Failed to find end of preprocessor statement.\n");

	while(data[pos-1]=='\\' || (data[pos-1]=='\r' && data[pos-2]=='\\'))
	  pos=search(data,"\n",pos+1);

        sscanf(data[start..pos], 
	       "#%*[ \t]charset%*[ \t\\]%s%*[ \r\n]", string charset);
	if(charset)
	  data = (data[0..pos]+
		  master()->decode_charset(data[pos+1..sizeof(data)-3], 
					   charset)
		  +"\n\0");  // New end sentinel.
	pos++;
	break;

      case 'a'..'z':
      case 'A'..'Z':
      case 128..:     // Lets simplify things for now...
      case '_':
	while(1)
	{
	  switch(data[pos])
	  {
	    case 'a'..'z':
	    case 'A'..'Z':
	    case '0'..'9':
	    case 128..:      // Lets simplify things for now...
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
          pos+=2;
          break;
        }
        pos++;
        break;

      case '0'..'9':
	if(data[pos]=='0') {
	  if(data[pos+1]=='x' || data[pos+1]=='X') {
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
	  else if(data[pos+1]=='b' || data[pos+1]=='B') {
	    pos+=2;
	    while(1) {
	      if(data[pos]!='0' && data[pos]!='1')
		break;
	      pos++;
	    }
	  }
	}
	while(data[pos]>='0' && data[pos]<='9') pos++;
        if(data[pos]=='.' && data[pos+1]>='0' && data[pos+1]<='9')
	{
	  pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	  if(data[pos]=='e' || data[pos]=='E')
	  {
	    pos++;
	    while(data[pos]>='0' && data[pos]<='9') pos++;
	  }
	  break;
	}
	if( (data[pos]=='e' || data[pos]=='E') &&
	    data[pos+1]>='0' && data[pos+1]<='9' )
	{
	  pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	}
	break;

      default:
        UNKNOWN_TOKEN;
      
      case '`':
	{
	int bqstart = pos;
        while(data[pos]=='`')
          ++pos;
        if (pos - bqstart > 3) { // max. three ``` {
	  pos = bqstart;
          UNKNOWN_TOKEN;
	}
        int chars = backquoteops[data[pos..pos+3]]
	  || backquoteops[data[pos..pos+2]]
          || backquoteops[data[pos..pos+1]]
          || backquoteops[data[pos..pos]];
        if (chars)
          pos += chars;
        else {
	  pos = bqstart;
          UNKNOWN_TOKEN;
	}
        }
        break;

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
      case '@':
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
          case "::":
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
	  if(data[pos]=='\\') pos+=2;
          int end=search(data, "'", pos)+1;
          if (!end) {
            --pos;
            UNKNOWN_TOKEN;
          }
          pos=end;
	  break;

	case '"':
	{
	  int q,s;
	  while(1)
	  {
	    q=search(data,"\"",pos+1);
	    s=search(data,"\\",pos+1);


	    if( q==-1 ||
		(s==sizeof(data)-2 && s<q) )
	      error("Unterminated string.\n");

	    if(s==-1 || s>q) {
	      pos = q+1;
	      break;
	    }

	    pos=s+1;
	  }
	  if(has_value(data[start..pos-1], "\n"))
	    error("Newline in string.\n");
	  break;
	}
      }
    }

    ret+=({ data[start..pos-1] });
  }
}
 
