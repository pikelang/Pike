#define NEWLINE() do { while( pos<len && data[pos]!='\n' )pos++; } while(0)
#define SKIPWHT() do {while(data[pos]==' '||data[pos]=='\t')pos++; } while(0)

static unsigned int TOKENIZE(struct array **res, CHAR *data, unsigned int len)
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
        else if ( data[pos+1] >= '0' && data[pos+1] <= '9' )
        {
          pos++;
          while(pos < len && data[pos] >= '0' && data[pos] <= '9')
            pos++;
          if (pos != len)
            pos--;
        }

	break;

      case '0':
	if( data[pos+1]=='x' || data[pos+1]=='X' )
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
	else if( data[pos+1]=='b' || data[pos+1]=='B' )
	{
	  pos+=2;
	  while( pos<len && (data[pos]=='0' || data[pos]=='1') )
	    pos++;
	  if( pos != len )
	    pos--;
	  break;
	}

	/* FALL_THROUGH */
      case '1':  case '2':    case '3':  case '4':
      case '5':  case '6':    case '7':  case '8':
      case '9':
	while(pos<len && data[pos]>='0' && data[pos]<='9') pos++;
	if( pos == len ) break;
	if(data[pos]=='.' && data[pos+1]>='0' && data[pos+1]<='9')
	{
	  pos++;
	  while(pos<len && data[pos]>='0' && data[pos]<='9') pos++;
	  if(data[pos]=='e' || data[pos]=='E')
	  {
	    pos++;
	    if(data[pos]=='-' || data[pos]=='+') pos++;
	    while(data[pos]>='0' && data[pos]<='9') pos++;
	  }
	}
	if(data[pos]=='e' || data[pos]=='E')
	{
	  pos++;
	  if(data[pos]=='-' || data[pos]=='+') pos++;
	  while(data[pos]>='0' && data[pos]<='9') pos++;
	}
	if(data[pos]=='b')
	{
	  pos++;
          if(data[pos]=='i')
              pos++;
          if(data[pos]=='t')
              pos++;
	}
	if( pos != len )
	  pos--;
	break;

      case  '`':
	if(data[pos+1]=='`') pos++;
        if(m_isidchar(data[pos+1]))
        {
            do {
                pos++;
            } while(m_isidchar2(data[pos+1]));
            if(data[pos+1] == '=')
                pos++;
            break;
            // NOTE: Depends on string having null at end.
        }
	switch(data[pos+1]) {
	case '<':
	case '>':
	  pos++;
	  if(data[pos+1]==data[pos]) pos++;
	  break;
	case '-':
	  pos++;
	  if(data[pos+1]=='>') pos++;
	  break;
	case '(':
	  pos++;
	  if(data[pos+1]==')') pos++;
	  break;
	case '[':
	  pos++;
	  if(data[pos+1]=='.') pos++;
	  if(data[pos+1]=='.') pos++;
	  if(data[pos+1]==']') pos++;
	  break;
	case '/': case '%': case '*': case '&': case '|':
	case '^': case '+': case '!': case '=': case '~':
	  pos++;
          if(data[pos+1] == '*')
              pos++;
	  break;
	}
	if(data[pos+1]=='=') pos++;
	break;


      case '\\': continue; /* IGNORED */

      case '/':
	if( pos == len-1 )
	  break;
	switch( data[pos+1] )
	{
	  case '/':
	    NEWLINE();
	    /* line comment */
	    break;
	  case '*':
	    pos += 2;
	    while( pos < len && !(data[pos] == '/' && data[pos-1] == '*') )
	      pos++;
	    if( pos == len )
	      goto failed_to_find_end;
	    break;
	  case '=':
	    pos++;
	    break;
	  default: break;
	}

      case '{': case '}':
      case '[': case ']':
      case '(': case ')':
      case ';':
      case ',': case '?':
      case '@': /* Hm. Pike specific if I ever saw one. */
	break;  /* all done, one character token */

      case ':':
	if( data[pos+1] == ':' ) pos++;
	break;

      case '<':
	if( data[pos+1] == '<' ) pos++;
	if( data[pos+1] == '=' ) pos++;
	break;

      case '-':
	if( data[pos+1] == '-' ) pos++;
	else
	{
	  if( data[pos+1] == '>' ) pos++;
	  if( data[pos+1] == '=' ) pos++;
	}
	break;

      case '>':
	if( data[pos+1] == '>' ) pos++;
	if( data[pos+1] == '=' ) pos++;
	break;

      case '+': case '&':  case '|':
	if( data[pos+1] == data[pos] ) pos++;
	else if( data[pos+1] == '=' ) pos++;
	/* FALLTHRU */

      case '*': case '%':
      case '^': case '!':  case '~':  case '=':
	if( data[pos+1] == '=' )
	  pos++;
	break;

      SPACECASE
        pos++;
	while( pos < len )
	{
	  switch(data[pos])
          {
            SPACECASE
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
	  /* FIXME: Newline in string detection! */
	  pos++;
	}
	if( pos >= len )
	  goto failed_to_find_end;
	break;

      case '#':
	pos++;
	SKIPWHT();
	if (data[pos] == '\"') {
	  /* Support for #"" */
	  for (pos++; pos < len; pos++) {
	    if (data[pos] == '\"') break;
	    if (data[pos] == '\\') pos++;
	  }
	  if (pos >= len)
	    goto failed_to_find_end;
	  break;
        }
        {
          char end = 0;
          switch( data[pos] )
          {
          case '(': end=')'; break;
          case '[': end=']'; break;
          case '{': end='}'; break;
          }
          if(end)
          {
            for (pos++; pos<len-1; pos++)
              if (data[pos] == '#' && data[pos+1] == end)
              {
                pos++;
                end=0;
                break;
              }
            if (end)
              goto failed_to_find_end;
            break;
          }
        }
	if( data[pos] == 's' &&
	    data[pos+1] == 't' &&
	    data[pos+2] == 'r' &&
	    data[pos+3] == 'i' &&
	    data[pos+4] == 'n' &&
	    data[pos+5] == 'g' ) {
	  pos += 6;
	  SKIPWHT();
	  if(data[pos]=='\"') {
	    for (pos++; pos < len; pos++) {
	      if (data[pos] == '\"') break;
	      if (data[pos] == '\\') pos++;
	    }
	  }
	  else if(data[pos]=='<') {
	    for(pos++; pos<len; pos++)
	      if(data[pos]=='>') break;
	  } else if (!data[pos]) {
	    pos--;
	    break;
	  }
	  else
	    Pike_error("Illegal character after #string\n");
	  if (pos >= len)
	    goto failed_to_find_end;
	  break;
	}
	NEWLINE();
	while( data[pos-1]=='\\' ||
	       (pos>2 && data[pos-1]=='\r' && data[pos-2]=='\\') )
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
