// Not yet finished -- Fredrik Hubinette

constant NOERROR=0;
constant FORMERR=1;
constant SERVFAIL=2;
constant NXDOMAIN=3;
constant NOTIMPL=4;
constant NXRRSET=8;

constant QUERY=0;

constant C_IN=1;
constant C_ANY=255;

constant T_A=1;
constant T_NS=2;
constant T_MD=3;
constant T_MF=4;
constant T_CNAME=5;
constant T_SOA=6;
constant T_MB=7;
constant T_PTR=12;
constant T_HINFO=13;
constant T_MINFO=14;
constant T_MX=15;
constant T_TXT=16;
constant T_AAAA=28;


class protocol
{
  string mklabel(string s)
  {
    if(strlen(s)>63)
      throw(({"Too long component in domain name",backtrace()}));
    return sprintf("%c%s",strlen(s),s);
  }

  // This will have to be generalized for
  // the server part...
  string mkquery(string dname,
		 int cl,
		 int type)
  {
    int id=random(65536); // make spoofing harder
    
    return sprintf("%2c%c%c%2c%2c%2c%2c%s\000%2c%2c",
		   id,
		   1,0,
		   1,
		   0,
		   0,
		   0,
		   Array.map(dname/".",mklabel)*"",
		   type,cl);

  }

  string decode_domain(string msg, int *n)
  {
    string *domains=({});
    
    int pos=n[0];
    int next=-1;
    string *ret=({});
    while(1)
    {
      switch(int len=msg[pos])
      {
	case 0:
	  if(next==-1) next=pos+1;
	  n[0]=next;
	  return ret*".";
	  
	case 1..63:
	  pos+=len+1;
	  ret+=({msg[pos-len..pos-1]});
	  continue;
	  
	default:
	  if(next==-1) next=pos+2;
	  pos=((len&63)<<8) + msg[pos+1];
	  continue;
      }
      break;
    }
  }

  string decode_string(string s, int *next)
  {
    int len=s[next[0]];
    next[0]+=len+1;
    return s[next[0]-len..next[0]-1];
  }

  int decode_short(string s, int *next)
  {
    sscanf(s[next[0]..next[0]+1],"%2c",int ret);
    next[0]+=2;
    return ret;
  }

  int decode_int(string s, int *next)
  {
    sscanf(s[next[0]..next[0]+1],"%2c",int ret);
    next[0]+=2;
    return ret;
  }
  
  mixed *decode_entries(string s,int num, int *next)
  {
    string *ret=({});
    for(int e=0;e<num;e++)
    {
      mapping m=([]);
      m->name=decode_domain(s,next);
      sscanf(s[next[0]..next[0]+10],
	     "%2c%2c%4c%2c",
	     m->type,m->cl,m->ttl,m->len);
      
    next[0]+=10;
    int tmp=next[0];
    switch(m->type)
    {
      case T_CNAME:
	m->cname=decode_domain(s,next);
	break;
      case T_PTR:
	m->ptr=decode_domain(s,next);
	break;
      case T_NS:
	m->ns=decode_domain(s,next);
	break;
      case T_MX:
	m->preference=decode_short(s,next);
	m->mx=decode_domain(s,next);
	break;
      case T_HINFO:
	m->cpu=decode_string(s,next);
	m->os=decode_string(s,next);
	break;
      case T_A:
      case T_AAAA:
	m->a=sprintf("%{.%d%}",values(s[next[0]..next[0]+m->len-1]))[1..];
	break;
      case T_SOA:
	m->mname=decode_domain(s,next);
	m->rname=decode_domain(s,next);
	m->serial=decode_int(s,next);
	m->refresh=decode_int(s,next);
	m->retry=decode_int(s,next);
	m->expire=decode_int(s,next);
	m->minimum=decode_int(s,next);
	break;
    }
    
    next[0]=tmp+m->len;
    ret+=({m});
    }
    return ret;
  }

  mapping decode_res(string s)
  {
    mapping m=([]);
    sscanf(s,"%2c%c%c%2c%2c%2c%2c",
	   m->id,
	   m->c1,
	   m->c2,
	   m->qdcount,
	   m->ancount,
	   m->nscount,
	   m->arcount);
    m->rd=(m->c1>>7)&1;
    m->tc=(m->c1>>6)&1;
    m->tc=(m->c1>>5)&1;
    m->opcode=(m->c1>>1)&15;
    m->qr=m->c1&1;
    
    m->rcode=(m->c2>>4)&15;
    m->cd=(m->c2>>3)&1;
    m->ad=(m->c2>>2)&1;
    m->ra=(m->c2)&1;
    
    m->length=strlen(s);
    
    string *tmp=({});
    int e;
    
    if(m->qdcount!=1)
      return m;
    
    int *next=({12});
    m->qd=decode_domain(s,next);
    sscanf(s[next[0]..next[0]+3],"%2c%2c",m->type, m->cl);
    next[0]+=4;
    m->an=decode_entries(s,m->ancount,next);
    m->ns=decode_entries(s,m->nscount,next);
    m->ar=decode_entries(s,m->arcount,next);
    return m;
  }
};


class client {
  inherit protocol;

  string nameserver;

  void create(void|string server)
  {
    if(!server)
    {
      foreach(Stdio.read_file("/etc/resolv.conf")/"\n", string line)
	{
	  string rest;
	  sscanf(line,"%s#",line);
	  sscanf(line,"%*[\r \t]%s",line);
	  line=reverse(line);
	  sscanf(line,"%*[\r \t]%s",line);
	  line=reverse(line);
	  sscanf(line,"%s%*[ \t]%s",line,rest);
	  switch(line)
	  {
	    case "domain":
	    case "search":
	      break; // Not yet implemented
	      
	    case "nameserver":
	      nameserver=rest;
	      break;
	  }
	}
    }else{
      nameserver=server;
    }
  }


  // Warning: NO TIMEOUT
  mapping do_sync_query(string s)
  {
    object udp=spider.dumUDP();
    udp->bind(0);
    udp->send(nameserver,53,s);
    mapping m;
    do {
      m=udp->read();
    } while (m->port != 53 ||
	     m->ip != nameserver ||
	     m->data[0..1]!=s[0..1]);
    return decode_res(m->data);
  }
  

  // Warning: NO TIMEOUT
  mixed *gethostbyname(string s)
  {
    mapping m=do_sync_query(mkquery(s, C_IN, T_A));
    string *names=({});
    string *ips=({});
    foreach(m->an, mapping x)
      {
	if(x->name)
	  names+=({x->name});
	if(x->a)
	  ips+=({x->a});
      }
    return ({
      sizeof(names)?names[0]:0,
	ips,
	names,
	});
  }
  
  // Warning: NO TIMEOUT
  mixed *gethostbyaddr(string s)
  {
    mapping m=do_sync_query(mkquery(reverse(s/".")*"."+".IN-ADDR.ARPA",
				    C_IN,
				    T_PTR));
    string *names=({});
    string *ips=({});
    foreach(m->an, mapping x)
      {
	if(x->ptr)
	  names+=({x->ptr});
	if(x->name)
	  
	{
	  ips+=({reverse(x->name/".")[2..]*"."});
	}
      }
    return ({
      sizeof(names)?names[0]:0,
	ips,
	names,
	});
  }

  string get_primary_mx(string host)
  {
    mapping m=do_sync_query(mkquery(host,C_IN,T_MX));
    int minpref=29372974;
    string ret;
    foreach(m->an, mapping m2)
      {
	if(m2->preference<minpref)
	{
	  ret=m2->mx;
	  minpref=m2->preference;
	}
      }
    return ret;
  }
}


class async_client
{
  inherit client;

  mapping requests=([]);
  class request
  {
  };
};
