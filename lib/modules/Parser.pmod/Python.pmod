#pike __REAL_VERSION__

// $Id$

//! Returns the provided string with Python code as
//! an array with tokens.
array(string) split(string data) {

  int pos;
  array(string) ret = ({});
  data += "\0\0\0"; // end sentinel

  while(1) {
    int start = pos;

    switch(data[pos]) {

    case 0:
      return ret;

    case '\r':
      // End of line
      pos++;
      if(data[pos]=='\n') pos++;
      break;

    case '\n':
      // End of line
      pos++;
      if(data[pos]=='\r') pos++;
      break;

    case ' ':
    case '\t':
    case '\f':
      // Spaces and tabs
      while( (< ' ', '\t' >)[data[++pos]] );
      break;

    case '\\':
      // Line joiner
      pos++;
      break;

    case '#':
      // Comment. Ends at end of physical line.
      while(!(< '\r', '\n' >)[data[++pos]]);
      break;

    case 'a'..'z':
    case 'A'..'Z':
    case '_':
      while(1) {
	switch(data[pos]) {
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
      }
      else {
	// Octal, decimal and floats except exponent.
	while( (< '0','1','2','3','4','5','6','7','8','9',
		  '.' >)[data[++pos]] );
      }

      // Long integer
      if( (< 'l', 'L' >)[data[pos]] ) {
	pos++;
	break;
      }

      if( (< 'e', 'E' >)[data[pos]] ) {
	pos++;
	if( (< '+', '-' >)[data[pos]] ) pos++;
	while( (< '0','1','2','3','4','5','6','7','8','9' >)[data[++pos]] );
      }

      // Imaginary number
      if( (< 'j', 'J' >)[data[pos]] ) pos++;

      break;

    case '\'':
      if( data[pos+1]=='\'' && data[pos+2]=='\'' ) {
	pos += 2;
	while( !(data[++pos]=='\'' && data[pos+1]=='\'' && data[pos+2]=='\'') )
	  if( data[pos]=='\\' ) pos++;
	pos += 2;
      }
      else
	while( data[++pos]!='\'' )
	  if( data[pos]=='\\' ) pos++;

      pos++;
      break;

    case '"':
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

    case '<': case '>':
      pos++;
      if( (< '<', '=', '>' >)[data[pos]]) pos++;
      if( data[pos]=='=' ) pos++;
      break;

    case '*':
      if(data[pos+1]=='*') pos++;
      // fallthrough
    case '+': case '-':
    case '/': case '%':
    case '&': case '|': case '^':
    case '=':
      pos ++;
      if( data[pos]=='=' ) pos++;
      break;

    case '!':
      pos++;
      if(data[pos]=='=') pos++;
      break;

    case '(': case ')':
    case '[': case ']':
    case '{': case '}':
    case ':': case ';':
    case '~':
    case ',':
      pos++;
      break;

    default:
      error("Parser error at %d (%O).\n", pos, data[pos..pos+20]);
    }

    ret += ({ data[start..pos-1] });
  }
}
