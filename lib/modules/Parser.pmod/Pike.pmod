#pike __REAL_VERSION__

inherit "C.pmod";

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
	pos+=1;
	if(data[pos]=='\"')
	  break;
	pos=search(data,"\n",pos);
	if(pos==-1)
	  error("Failed to find end of preprocessor statement.\n");
	
	while(data[pos-1]=='\\') pos=search(data,"\n",pos+1);
        sscanf(data[start..pos], 
	       "#%*[ \t]charset%*[ \t\\]%s%*[ \n]", string charset);
	if(charset)
	  data = (data[0..pos]+
		  master()->decode_charset(data[pos+1..sizeof(data)-3], 
					   charset)
		  +"\n\0");  // New end sentinel.
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
	werror("%O\n",ret);
	werror("Unknown token %O\n",data[pos..pos+20]);
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
      case '@':
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
 
