#pike __REAL_VERSION__

//! ECMAScript/JavaScript token parser based on ECMAScript 2017
//! (ECMA-262), chapter 11: Lexical Grammar.

protected constant keywords = (<
  "await", "break", "case", "catch", "class", "const", "continue",
  "debugger", "default", "delete", "do", "else", "export", "extends",
  "finally", "for", "function", "if", "import", "in", "instanceof",
  "new", "return", "super", "switch", "this", "throw", "try", "typeof",
  "var", "void", "while", "with", "yield",
>);

// Division can't follow these, so any slash starts a regexp.
protected constant punctuators = (<
  "{", /*"}",*/ "(", /*")",*/ "[", /*"]",*/ ".", "...", ";", ",", "<", ">",
  "<=", ">=", "==", "!=", "===", "!==", "+", "-", "*", "%", "**",
  "++", "--", "<<", ">>", ">>>", "&", "|", "^", "!", "~", "&&", "||",
  "?", ":", "=", "+=", "-=", "**=", "<<=", ">>=", ">>>=", "&=", "|=",
  "^=", "=>", "/", "/=",
>);

int(0..1) is_whitespace(int c) {
  return (< 0x9, 0xb, 0xc, 0x20, 0xa0, 0x1680, 0x2000, 0x2001, 0x2002,
            0x2003, 0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009,
            0x200a, 0x202f, 0x205f, 0x3000, 0xfeff >)[c];
}

//! Splits the ECMAScript source @[data] in tokens.
array(string) split(string data)
{
  if( has_prefix(data, "\357\273\277") )
    data = utf8_to_string(data);

  int pos;
  array(string) ret = ({});
  data += "\0\0\0";

  while(1)
  {
    int start = pos;

    switch(data[pos])
    {
    case 0:
      return ret;

    case '\r':
    case '\n':
    case 0xfeff:
    case 0x2028:
    case 0x2029:
      pos++;
      break;

    case '.':
      pos++;
      if( data[pos]<'0' || data[pos]>'9' ) break;
      // fallthrough
    case '0'..'9':
      // Hexdigit
      if( data[pos]=='0' && (< 'x', 'X' >)[data[pos+1]] ) {
	pos += 2;
	while( (< '0','1','2','3','4','5','6','7','8','9',
		  'a','b','c','d','e','f','A','B','C','D',
                  'E','F' >)[data[++pos]] );
        break;
      }
      else if( data[pos]=='0' && (< 'b', 'B' >)[data[pos+1]] ) {
        pos += 2;
        while( (< '0', '1' >)[data[++pos]] );
        break;
      } else {

        // Octal marker
        if( data[pos]=='0' && (< 'o', 'O' >)[data[pos+1]] )
          pos += 2;

	// Octal, decimal and floats except exponent.
	while( (< '0','1','2','3','4','5','6','7','8','9',
		  '.' >)[data[++pos]] );
      }

      if( (< 'e', 'E' >)[data[pos]] ) {
	pos++;
	if( (< '+', '-' >)[data[pos]] ) pos++;
	while( (< '0','1','2','3','4','5','6','7','8','9' >)[data[++pos]] );
      }
      break;

    case '\'':
      // We don't need to parse \x12 \x1234 or \x{0} for splitting
      if( data[pos+1]=='\'' && data[pos+2]=='\'' ) {
	pos += 2;
	while( !(data[++pos]=='\'' && data[pos+1]=='\'' && data[pos+2]=='\'') )
	  if( data[pos]=='\\' ) pos++;
	pos += 2;
      }
      else
      {
        while( data[++pos]!='\'' )
          if( data[pos]=='\\' ) pos++;
      }

      pos++;
      break;

    case '"':
      // We don't need to parse \x12 \x1234 or \x{0} for splitting
      if( data[pos+1]=='\"' && data[pos+2]=='\"' ) {
	pos += 2;
	while( !(data[++pos]=='\"' && data[pos+1]=='\"' && data[pos+2]=='\"') )
	  if( data[pos]=='\\' ) pos++;
	pos += 2;
      }
      else
	while( data[++pos]!='\"' )
	  if( data[pos]=='\\' ) pos++;
      pos++;
      break;

    case '/':
      pos++;
      switch(data[pos])
      {
      case '/':
        while( !((< '\n', '\r', 0x2028, 0x2029, 0 >)[data[pos++]]) );
        pos--;
        break;

      default:

        int regexp_context() {
          int p = 0;
          string t;
          while( ++p ) {
            if( sizeof(ret)<p ) return 1;
            t = ret[-p];
            if( is_whitespace(t[0]) ) continue;
            if( has_prefix(t, "//") || has_prefix(t, "/*") ) continue;
            break;
          };

          return keywords[t] || punctuators[t];
        };
        if( regexp_context() )
        {
          int in_range = 0;

        loop:while( 1 ) {
            switch( data[pos] ) {
            case '\\':
              pos++;
              break;
            case '/':
              if(!in_range) {
                pos++;
                break loop;
              }
              break;
            case '[':
              in_range=1;
              break;
            case ']':
              in_range=0;
              break;
            }
            pos++;
          }

          // Really all of ID_Continue
          while( (< 'g', 'i', 'm', 'u', 'y' >)[data[pos]] ) pos++;

          //          werror("%O\n", data[start..pos-1]);
          break;
        }
        else
        {
          if( data[pos]=='=' ) pos++;
          break;
        }
        break;

      case '*':
        while( !(data[pos++]=='*' && data[pos]=='/') );
        pos++;
        break;
      }
      break;

      // FIXME: Leading character can be UnicodeEscapeSequence ( \u%x
      // or \u{%x} )
      // FIXME: Should include all UnicodeIDStart
    case 'a'..'z':
    case 'A'..'Z':
    case '_':
    case '$':
      while(1) {
	switch(data[pos]) {
          // FIXME: Character can be UnicodeEscapeSequence ( \u%x or
          // \u{%x} )
          // FIXME: Should include all UnicodeIDContinue
        case 0x200c: case 0x200d: // ZWNJ, ZWJ
	case 'a'..'z':
	case 'A'..'Z':
	case '0'..'9':
        case '_':
        case '$':
	  pos++;
	  continue;
	}
	break;
      }
      break;

      // FIXME: < > <= >= << >> >>> <<= >>= >>>=
    case '<': case '>':
      pos++;
      if( (< '<', '=', '>' >)[data[pos]]) pos++;
      if( data[pos]=='=' ) pos++;
      break;

    case '*':
      if(data[pos+1]=='*') pos++;
      // fallthrough
    case '+': case '-':
    case '%':
    case '&': case '|': case '^':
      pos ++;
      if( (< '+', '-', '&', '|' >)[data[pos]] && data[pos]==data[pos-1])
        pos++;
      if( data[pos]=='=' ) pos++;
      break;

    case '=':
      if( data[pos+1]=='>' ) { pos++; break; }
      // Fallthrough
    case '!':
      pos++;
      if(data[pos]=='=') pos++;
      if(data[pos]=='=') pos++;
      break;

    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':': case ';':
    case '~':
    case ',':
    case '?':
      pos++;
      break;

    default:

      // White spaces
      if( is_whitespace(data[pos]) )
      {
        while( is_whitespace(data[++pos]) );
        break;
      }

      error("%O\nParser error at %d (%O).\n", ret[<10..], pos, data[pos..pos+20]);
    }

    ret += ({ data[start..pos-1] });
  }
}
