string *quote_from;
string *quote_to;
string *unquote_from;
string *unquote_to;

void create()
{
  quote_from=quote_to=unquote_from=unquote_to=({});
  for(int e=0;e<256;e++)
  {
    switch(e)
    {
    case 'a'..'z':
    case 'A'..'Z':
    case '0'..'9':
    case 'å': case 'ä': case 'ö':
    case 'Å': case 'Ä': case 'Ö':
    case 'ü': case 'Ü':
    case '!':
    case '#':
    case '$':
    case '&':
    case '/':
    case '(':
    case ')':
    case '=':
    case '-':
    case '_':
    case '+':
    case '?':
    case '~':
    case '*':
    case ',':
    case '.':
    case ';':
    case ':':
      break;

    default:
      quote_from+=({sprintf("%c",e)});
      quote_to+=({sprintf("%%%02x",e)});
    }
    unquote_from+=({sprintf("%%%02x",e)});
    unquote_to+=({sprintf("%c",e)});
  }
}

string quote_param(string s) { return replace(s,quote_from,quote_to); }
string unquote_param(string s) { return replace(s,unquote_from,unquote_to); }

string mktag(string tag, mapping params)
{
  string ret="<"+tag;
  foreach(indices(params),string i)
  {
    ret+=" "+quote_param(i);

    if(stringp(params[i]))
      ret+="='"+quote_param(params[i])+"'";
  }
  return ret+">";
}
