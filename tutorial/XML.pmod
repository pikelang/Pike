import ".";

string *from=({"&#a0;","&amp;","&lt;","&gt;"});
string *to=({" ","&","<",">"});

string unquote(string x) { return replace(x,from,to); }
string quote(string x) { return replace(x,to,from); }

string *from_data=({"&#a0;","&amp;","&lt;","&gt;","&apos;","&quot;"});
string *to_data=({" ","&","<",">","'","\""});

string unquote_data(string x) { return replace(x,from_data,to_data); }
string quote_data(string x) { return replace(x,to_data,from_data); }

#define TAG object(Sgml.Tag)|string
#define SGML array(TAG)

string mktag(string tag, mapping params, int end)
{
  string ret="<"+tag;
  foreach(indices(params),string i)
    if(params[i])
      ret+=" "+i+"='"+quote_data(stringp(params[i])?params[i]:i)+"'";

  return ret + (end?"/>":">");
}

string generate(SGML data, void|function mkt)
{
  string ret="";
  if(!mkt)
  {
    mkt=mktag;
  }
  foreach(data, TAG foo)
    {
      if(stringp(foo))
      {
	ret+=quote(foo);
      }
      else if (objectp(foo))
      {
	ret+=mkt(foo->tag,foo->params,!foo->data);
	if(foo->data)
	{
#if 0
	  if(foo->tag=="script")
	  {
	    // Magic for javascript!
	    ret+="\n<!--\n"+
	      foo->data*""+
	      "// -->\n";
	  }else
#endif
	    ret+=generate(foo->data,mkt);

	  ret+=mkt("/"+foo->tag,([]),0);
	}
      }
      else error("got an illegal tag or string : %O\n"
		 "in: %O\n",foo,data);
    }

  return ret;
}


#ifdef TEST
int main()
{
  write(sprintf("%O\n",group(lex(Stdio.read_file("tutorial.wmml")))));
}
#endif
