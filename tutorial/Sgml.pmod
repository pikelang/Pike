string *from=({"&nbsp;","&amp;","&lt;","&gt;"});
string *to=({" ","&","<",">"});

string unquote(string x) { return replace(x,from,to); }
string quote(string x) { return replace(x,to,from); }

class Tag
{
  string tag;
  int pos;
  mapping(string:mixed) params=([]);
//  array(Tag) data;
  array(object) data;
  string file;

  string location()
  {
    return "pos "+pos+" in file "+file;
  }

  void create(string t, void|mapping p, void|int po, 
	      void|array(object) d, void|string f)
  {
    tag=t;
    pos=po;
    params=p||([]);
    data=d;
    file=f;
  }
};

#define TAG object(Tag)|string
#define SGML array(TAG)

SGML lex(string data, string file)
{
  mixed foo=data/"<";
  SGML ret=({ unquote(foo[0]) });
  int pos=strlen(foo[0]);
  for(int e=1;e<sizeof(foo);e++)
  {
    string tag;
    string s=foo[e];
    pos++;

    if(s[0..2]=="!--")
    {
      pos+=strlen(foo[e]);
      while(sscanf(s,"%*s-->%s",s)!=2)
      {
	e++;
	s+="<"+foo[e];
	pos+=strlen(foo[e])+1;
      }
      ret[-1]+=unquote(s);
      continue;
    }
    
    if(sscanf(s,"%[^ \t\n\r>]%s",tag,s)!=2)
      werror(sprintf("Missing end > (around pos %d in %s)\n",pos,file));

    tag=lower_case(tag);
    mapping params=([]);

    while(1)
    {
      sscanf(s,"%*[ \t\r\n]%s",s);
      if(!strlen(s))
      {
	write(sprintf("Missing end > (around pos %d in %s)\n",pos,file));
	break;
      }
      if(s[0]=='>')
      {
	s=s[1..];
	break;
      }

      if(sscanf(s,"%[^ \t\r\n>=]%s",string key,s) && strlen(key))
      {
	key=lower_case(key);
	if(s[0]=='=')
	{
	  string val;
	  switch(s[1])
	  {
	  case '\'':
	    while(sscanf(s,"='%s'%s",val,s)!=2)
	    {
	      e++;
	      s+="<"+foo[e];
	      pos+=strlen(foo[e])+1;
	    }
	    break;
	  case '\"':
	    while(sscanf(s,"=\"%s\"%s",val,s)!=2)
	    {
	      e++;
	      s+="<"+foo[e];
	      pos+=strlen(foo[e])+1;
	    }
	    break;
	  default:
	    sscanf(s,"=%[^ \t\r\n>]%s",val,s);
	    break;
	  }
	  if(!val)
	  {
	    werror("Missing end quote parameter\n"); 
	  }
	  params[key]=val;
	}else{
	  params[key]=1;
	}
      }
    }

//    werror("Fount tag "+tag+" at pos "+pos+".\n");
    ret+=({ Tag(tag,params,pos,0,file), unquote(s) });
    pos+=sizeof(foo[e]);
  }

  return ret;
}


SGML group(SGML data)
{
  SGML ret=({});
  foreach(data,TAG foo)
  {
    if(objectp(foo))
    {
      if(strlen(foo->tag) && foo->tag[0]=='/')
      {
	string tag=foo->tag[1..];
	string t;
	int d;
	if (sscanf(tag,"%[^ \t\r\n>]%*s",t)==2) foo->tag=tag=t;
	for(d=sizeof(ret)-1;d>=0;d--)
	{
	  if(objectp(ret[d]) && !ret[d]->data && ret[d]->tag==tag)
	  {
	    ret[d]->data=ret[d+1..];
	    ret=ret[..d];
	    break;
	  }
	}
	if(d>=0) continue;
      }
    }
    ret+=({foo});
  }
  return ret;
}



string mktag(string tag, mapping params)
{
  string ret="<"+tag;
  foreach(indices(params),string i)
  {
    ret+=" "+i;

    if(stringp(params[i]))ret+="="+params[i];
  }
  return ret+">";
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
      }else{
	ret+=mkt(foo->tag,foo->params);
	if(foo->data)
	{
	  ret+=generate(foo->data,mkt);
	  ret+=mkt("/"+foo->tag,([]));
	}
      }
    }

  return ret;
}

SGML copy(SGML data)
{
  if(!data) return 0;
  SGML ret=({});
  foreach(data,TAG t)
    {
      if(stringp(t))
      {
	ret+=({t});
      }else{
	ret+=({Tag(t->tag,t->params+([]),t->pos,copy(t->data),t->file)});
      }
    }
  return ret;
}

string get_text(SGML data)
{
  string ret="";
  foreach(data,TAG t)
    {
      if(stringp(t))
      {
	ret+=t;
      }else{
	ret+="<"+t->tag;
	foreach(indices(t->params), string name)
	  ret+=" "+name+"="+t->params[name];

	ret+=">";
	if(t->data)
	{
	  ret+=get_text(t->data);
	  ret+="</"+t->tag+">";
	}
      }
    }
  return ret;
}


#ifdef TEST
int main()
{
  write(sprintf("%O\n",group(lex(Stdio.read_file("tutorial.wmml")))));
}
#endif
