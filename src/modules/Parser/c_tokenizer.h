#define NEWLINE() do { while( pos<len && data[pos]!='\n' )pos++; } while(0)

unsigned int TOKENIZE( struct array **res, CHAR *data, unsigned int len )
{
  unsigned int start=0;
  unsigned int pos;
  for( pos=0;pos<len; pos++ )
  {
    switch(data[pos])
    {
      case '.':
	if( data[pos+1]=='.' )
	{
	  pos++;
	  if( data[pos+1] == '.')
	    pos++;
	  break;
	}
	/* fallthrough. */
      case '0':
	if( data[pos+1]=='x' || data[pos+1]=='X')
	{
	  pos+=2;
	  while( (pos < len) &&
		 ((data[pos]>='0' && data[pos] <='9') ||
		  (data[pos]>='a' && data[pos] <='f') ||
		  (data[pos]>='A' && data[pos] <='F')))
	    pos++;
	  if( pos != len )
	    pos--;
	  break;
	}
	
	/* fallthrough. */
      case '1':  case '2':    case '3':  case '4':
      case '5':  case '6':    case '7':  case '8':
      case '9':
	while(pos<len && data[pos]>='0' && data[pos]<='9') pos++;
	if( pos == len ) break;
	if(data[pos]=='.')
	{
	  pos++;
	  while(pos<len && data[pos]>='0' && data[pos]<='9') pos++;
	  if(data[pos]=='e' || data[pos]=='E')
	  {
	    pos++;
	    if(data[pos]=='-') pos++;
	    while(data[pos]>='0' && data[pos]<='9') pos++;
	  }
	}
	if(data[pos]=='e' || data[pos]=='E')
	{
	  pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	}
	if(data[pos]=='U' || data[pos]=='u') pos++; // 10UL
	if(data[pos]=='L' || data[pos]=='l') pos++; // 10L
	if( pos != len )
	  pos--;
	break;

      case  '`':
      case '\\': continue; /* IGNORED */

      case '/':
	if( pos == len-1 )
	  break;
	switch( data[pos+1] )
	{
	  case '/':
	    NEWLINE();
	    // line comment
	    break;
	  case '*':
	    pos += 2;
	    while( pos < len && !(data[pos] == '/' && data[pos-1] == '*') )
	      pos++;
	    if( pos == len )
	      goto failed_to_find_end;
	    break;
	  default: break;
	}
      case '{': case '}':
      case '[': case ']':
      case '(': case ')':
      case ';': case ':':
      case ',': case '?':
      case '@': // Hm. Pike specific if I ever saw one.
	break; // all done, one character token

      case '<':
	if( data[pos+1] == '<' ) pos++;
	if( data[pos+1] == '=' ) pos++;
	break;

      case '-':
	if( data[pos+1] == '>' ) pos++;
	else
	{
	  if( data[pos+1] == '-' ) pos++;
	  if( data[pos+1] == '=' ) pos++;
	}
	break;

      case '>':
	if( data[pos+1] == '>' ) pos++;
	if( data[pos+1] == '=' ) pos++;
	break;

      case '+': case '&':  case '|':
	if( data[pos+1] == data[pos] ) pos++;

      case '*': case '%':  
      case '^': case '!':  case '~':  case '=':
	if( data[pos+1] == '=' )
	  pos++;
	break;
	
      case ' ':
      case '\n':
      case '\r':
      case '\t':
      case '\14':
	pos++;
	while( pos < len )
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
	if( pos != len ) pos--;
	break;
      case '\'':
	pos++;
	while( pos < len )
	{
	  if( data[pos] == '\\' )
	    pos++;
	  else if( data[pos] == '\'' )
	    break;
	  pos++;
	}
	if( pos >= len ) goto failed_to_find_end;
	break;

      case '"':
	pos++;
	while( pos < len )
	{
	  if( data[pos] == '\\' )
	    pos++;
	  else if( data[pos] == '"' )
	    break;
	  pos++;
	}
	if( pos >= len )
	  goto failed_to_find_end;
	break;
	
      case '#':
	NEWLINE();
	while( data[pos-1]=='\\' || (pos>1 && data[pos-1]=='\r' && data[pos-2]=='\\') )
	{
	  pos++;
	  NEWLINE();
	}
	break;

      default:
	if( m_isidchar( data[pos] ) )
	{
	  pos++;
	  while( m_isidchar2( data[pos] ) && pos < len) pos++;
	  if( pos != len )
	    pos--;
	}
	else
	  Pike_error("Unexpected character %x (%c) at position %d.\n",
		     data[pos], (isprint(data[pos])?data[pos]:'?'), pos );
    }
    PUSH_TOKEN( res, data+start, ( pos == len )?pos-start:pos-start+1 );
    start = pos+1;
  }
failed_to_find_end:
  return MINIMUM(start,len);
}
