//
// This file needs to support old pikes that e.g. don't understand
// "#pike". Some of them fail when they see an unknown cpp directive.
//
// #pike __REAL_VERSION__
//

protected constant splitter = Parser._parser._Pike.tokenize;

class UnterminatedStringError
//! Error thrown when an unterminated string token is encountered.
{
  inherit Error.Generic;
  constant error_type = "unterminated_string";
  constant is_unterminated_string_error = 1;

  string err_str;
  //! The string that failed to be tokenized

  protected void create(string pre, string post)
  {
    int line = String.count(pre, "\n")+1;
    err_str = pre+post;
    if( sizeof(post) > 100 )
      ::create(sprintf("Unterminated string %O[%d] at line %d\n",
                       post[..100], sizeof(post)-100, line));
    else
      ::create(sprintf("Unterminated string %O at line %d\n",
                       post, line));
  }
}

/* accessed from testsuite */
/*private*/ array(string) low_split(string data, void|mapping(string:string) state)
{
  if(state && state->remains)
    data = (string)m_delete(state, "remains") + data;
  // Cast to string above to work around old Pike 7.0 bug.

  array(string) ret;
  string rem;
  [ret, rem] = splitter(data);
  if(sizeof(rem)) {
    if(rem[0]=='"')
      throw(UnterminatedStringError(ret*"", rem));
    if(state) state->remains=rem;
  }
  return ret;
}

//! Splits the @[data] string into an array of tokens. An additional
//! element with a newline will be added to the resulting array of
//! tokens. If the optional argument @[state] is provided the split
//! function is able to pause and resume splitting inside #"" and
//! /**/ tokens. The @[state] argument should be an initially empty
//! mapping, in which split will store its state between successive
//! calls.
array(string) split(string data, void|mapping(string:string) state) {
  array r = low_split(data, state);

  array new = ({});
  for(int i; i<sizeof(r); i++)
    if(r[i][..1]=="//" && r[i][-1]=='\n')
      new += ({ r[i][..<1], "\n" });
    else
      new += ({ r[i] });

  if(sizeof(new) && (< "\n", " " >)[new[-1]])
    new[-1] += "\n";
  else
    new += ({ "\n" });
  return new;
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
  protected void create(string t, void|int l, void|string f, void|string space)
    {
      text=t;
      line=l;
      file=f;
      if(space) trailing_whitespaces=space;
    }

  //! If the object is printed as %s it will only output its text contents.
  protected string _sprintf(int how)
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
  protected int `==(mixed foo)
    {
      return (objectp(foo) ? foo->text : foo) == text;
    }

  //! A string can be added to the Token, which will be added to the
  //! text contents.
  protected string `+(string ... s)
    {
      return predef::`+(text,@s);
    }

  //! A string can be added to the Token, which will be added to the
  //! text contents.
  protected string ``+(string ... s)
    {
      return predef::`+(@s,text);
    }

  //! It is possible to case a Token object to a string. The text content
  //! will be returned.
  protected mixed cast(string to)
    {
      if(to=="string") return text;
    }

  //! Characters and ranges may be indexed from the text contents of the token.
  protected int|string `[](int a, void|int b) {
    if(undefinedp(b)) return text[a];
    return text[a..b];
  }
}

//! Returns an array of @[Token] objects given an array of string tokens.
array(Token) tokenize(array(string) s, void|string file)
{
  array(Token) ret=allocate(sizeof(s));
  int line=1;
  foreach(s; int e; string str)
  {
    ret[e]=Token(str,line,file);
    if(str[0]=='#')
    {
      if( (sscanf(str,"#%*[ \t\14]%d%*[ \t\14]\"%s\"", line,file) == 4) ||
          (sscanf(str,"#%*[ \t\14]line%*[ \t\14]%d%*[ \t\14]\"%s\"",line,file)==5))
        line--;
    }
    line+=sizeof(str/"\n")-1;
  }
  return ret;
}

protected constant global_groupings = ([ "{":"}", "(":")", "[":"]" ]);

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
		  token->file||"-", token->line,
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
	  case '\r':
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
	  ret+=sprintf("#line %d %O\n",line,file||"-");
	}
	tok=tok->text + tok->trailing_whitespaces;
      }
      ret+=tok;
      line+=String.count(tok,"\n");
    }

  return ret;
}
