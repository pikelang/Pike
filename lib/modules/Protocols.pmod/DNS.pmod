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

  string low_mkquery(int id,
		    string dname,
		    int cl,
		    int type)
  {
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

  // This will have to be generalized for
  // the server part...
  string mkquery(string dname,
		 int cl,
		 int type)
  {
    return low_mkquery(random(65536),dname,cl,type);
  }

  string decode_domain(string msg, int *n)
  {
    string *domains=({});
    
    int pos=n[0];
    int next=-1;
    string *ret=({});
    while(pos < sizeof(msg))
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
    for(int e=0;e<num && next[0]<strlen(s);e++)
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
    m->aa=(m->c1>>5)&1;
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

  static private int is_ip(string ip)
  {
    return(replace(ip,
		   ({ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "." }),
		   ({  "", "", "", "", "", "", "", "", "", "", ""  })) == "");
  }

  static private mapping etc_hosts;

#ifdef __NT__
  string get_tcpip_param(string val)
    {
      foreach(({
	"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
	  "SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP"
	  }),string key)
	{
	  catch {
	    return RegGetValue(HKEY_LOCAL_MACHINE, key, value);
	  };
	}
    }
#endif
  
  static private string match_etc_hosts(string host)
    {
      if (!etc_hosts) {
	string raw;
#ifdef __NT__
	raw=get_tcpip_param("DataBasePath")+"\\hosts";
#else
	raw="/etc/hosts";
#endif
	raw = Stdio.read_file(raw);
	
	etc_hosts = ([ "localhost":"127.0.0.1" ]);
	
	if (raw && sizeof(raw)) {
	  foreach(raw/"\n"-({""}), string line) {
	    // Handle comments, and split the line on white-space
		 line = lower_case(replace((line/"#")[0], "\t", " "));
	    array arr = (line/" ") - ({ "" });
	    
	    if (sizeof(arr) > 1) {
	      if (is_ip(arr[0])) {
		foreach(arr[1..], string name) {
		  etc_hosts[name] = arr[0];
		}
	      } else {
		// Bad /etc/hosts entry ignored.
		     }
	    }
	  }
	} else {
	  // Couldn't read /etc/hosts.
	}
      }
      return(etc_hosts[lower_case(host)]);
    }

  array(string) nameservers = ({});
  array domains = ({});
  void create(void|string|array(string) server, void|int|array(string) domain)
  {
    if(!server)
    {
      string domain;
#if __NT__

      
      domain=get_tcpip_param("Domain");
      nameservers = ({ get_tcpip_param("NameServer") });
      domains=get_tcpip_param("SearchList") / " "- ({""});
#else
      string resolv_conf = Stdio.read_file("/etc/resolv.conf");

      if (!resolv_conf) {
	throw(({ "Protocols.DNS.client(): No /etc/resolv.conf!\n",
		 backtrace() }));
      }

      foreach(resolv_conf/"\n", string line)
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
	  // Save domain for later.
	  domain = rest;
	  break;
	case "search":
	  rest = replace(rest, "\t", " ");
	  foreach(rest / " " - ({""}), string dom)
	    domains += ({dom});
	  break;
	      
	case "nameserver":
	  if (!is_ip(rest)) {
	    // Not an IP-number!
	    string host = rest;
	    if (!(rest = match_etc_hosts(host))) {
	      werror(sprintf("Protocols.DNS.client(): "
			     "Can't resolv nameserver \"%s\"\n", host));
	      break;
	    }
	  }
	  if (sizeof(rest)) {
	    nameservers = `+(nameservers, ({ rest }));
	  }
	  break;
	}
      }
#endif
      if (!sizeof(nameservers)) {
	nameservers = ({ "127.0.0.1" });
      }
      if(domain)
	domains = ({ domain }) + domains;
      domains = Array.map(domains, lambda(string d) {
				     if (d[-1] == '.') {
				       return d[..sizeof(d)-2];
				     }
				     return d;
				   });
    } else {
      if(arrayp(server))	
	nameservers = server;
      else
	nameservers = ({ server });

      if(arrayp(domain))	
	domains = domain;
      else
	if(stringp(domain))
	  domains = ({ domain });
	
    }
  }


  // Warning: NO TIMEOUT
  mapping do_sync_query(string s)
  {
    object udp=spider.dumUDP();
    udp->bind(0);
    udp->send(nameservers[0],53,s);
    mapping m;
    do {
      m=udp->read();
    } while (m->port != 53 ||
	     m->ip != nameservers[0] ||
	     m->data[0..1]!=s[0..1]);
    return decode_res(m->data);
  }
  

  // Warning: NO TIMEOUT
  mixed *gethostbyname(string s)
  {
    mapping m;
    if(sizeof(domains) && s[-1] != '.' && sizeof(s/".") < 3) {
      m=do_sync_query(mkquery(s, C_IN, T_A));
      if(!m || !m->an || !sizeof(m->an))
	foreach(domains, string domain)
	{
	  m=do_sync_query(mkquery(s+"."+domain, C_IN, T_A));
	  if(m && m->an && sizeof(m->an))
	    break;
	}
    } else {
      m=do_sync_query(mkquery(s, C_IN, T_A));
    }
      
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

  string arpa_from_ip(string ip)
  {
    return reverse(ip/".")*"."+".IN-ADDR.ARPA";
  }

  string ip_from_arpa(string arpa)
  {
    return reverse(arpa/".")[2..]*".";
  }
  
  // Warning: NO TIMEOUT
  mixed *gethostbyaddr(string s)
  {
    mapping m=do_sync_query(mkquery(arpa_from_ip(s), C_IN, T_PTR));
    string *names=({});
    string *ips=({});
    foreach(m->an, mapping x)
    {
      if(x->ptr)
	names+=({x->ptr});
      if(x->name)
	  
      {
	ips+=({ip_from_arpa(x->name)});
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
    mapping m;
    if(sizeof(domains) && host[-1] != '.' && sizeof(host/".") < 3) {
      m=do_sync_query(mkquery(host, C_IN, T_MX));
      if(!m || !m->an || !sizeof(m->an))
	foreach(domains, string domain)
	{
	  m=do_sync_query(mkquery(host+"."+domain, C_IN, T_MX));
	  if(m && m->an && sizeof(m->an))
	    break;
	}
    } else {
      m=do_sync_query(mkquery(host, C_IN, T_MX));
    }
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

#define RETRIES 12
#define RETRY_DELAY 5
#define REMOVE_DELAY 120
#define GIVE_UP_DELAY (RETRIES * RETRY_DELAY + REMOVE_DELAY)*2

class async_client
{
  inherit client;
  inherit spider.dumUDP : udp;
  async_client next_client;

  class Request
  {
    string req;
    string domain;
    function callback;
    int retries;
    int timestamp;
    mixed *args;
  };

  mapping requests=([]);

  static private void remove(object(Request) r)
  {
    if(!r) return;
    sscanf(r->req,"%2c",int id);
    m_delete(requests,id);
    r->callback(r->domain,0,@r->args);
    destruct(r);
  }

  void retry(object(Request) r, void|int nsno)
  {
    if(!r) return;
    if (nsno >= sizeof(nameservers)) {
      if(r->retries++ > RETRIES)
      {
	call_out(remove,REMOVE_DELAY,r);
	return;
      } else {
	nsno = 0;
      }
    }
    
    send(nameservers[nsno],53,r->req);
    call_out(retry,RETRY_DELAY,r,nsno+1);
  }

  void do_query(string domain, int cl, int type,
		function(string,mapping,mixed...:void) callback,
		mixed ... args)
  {
    for(int e=next_client ? 5 : 256;e>=0;e--)
    {
      int lid = random(65536);
      if(!catch { requests[lid]++; })
      {
	string req=low_mkquery(lid,domain,cl,type);
	
	object r=Request();
	r->req=req;
	r->domain=domain;
	r->callback=callback;
	r->args=args;
	r->timestamp=time();
	requests[lid]=r;
	udp::send(nameservers[0],53,r->req);
	call_out(retry,RETRY_DELAY,r,1);
	return;
      }
    }
    
    /* We failed miserably to find a request id to use,
     * so we create a second UDP port to be able to have more 
     * requests 'in the air'. /Hubbe
     */
    if(!next_client)
      next_client=async_client(nameservers,domains);
    
    next_client->do_query(domain, cl, type, callback, @args);
  }

  static private void rec_data()
  {
    mixed err;
    if (err = catch {
      mapping m=udp::read();
      if(m->port != 53 || search(nameservers, m->ip) == -1) return;
      sscanf(m->data,"%2c",int id);
      object r=requests[id];
      if(!r) return;
      m_delete(requests,id);
      r->callback(r->domain,decode_res(m->data),@r->args);
      destruct(r);
    }) {
      werror(sprintf("DNS: Failed to read UDP packet. Connection refused?\n"
		     "%s\n",
		     describe_backtrace(err)));
    }
  }

  static private void generic_get(string d,
				  mapping answer,
				  int multi, 
				  int all,
				  int type, 
				  string field,
				  string domain,
				  function callback,
				  mixed ... args)
  {
    if(!answer || !answer->an || !sizeof(answer->an))
    {
      if(multi == -1 || multi >= sizeof(domains)) {
	// Either a request without multi (ip, or FQDN) or we have tried all
	// domains.
	callback(domain,0,@args);
      } else {
	// Multiple domain request. Try the next one...
	do_query(domain+"."+domains[multi], C_IN, type,
		 generic_get, ++multi, all, type, field, domain,
		 callback, @args);
      }
    } else {
      if (all) {
	callback(domain, answer->an, @args);
      } else {
	foreach(answer->an, array an)
	  if(an[field])
	  {
	    callback(domain, an[field], @args);
	    return;
	  }
	callback(domain,0,@args);
	return;
      }
    }
  }

  void host_to_ip(string host, function callback, mixed ... args)
  {
    if(sizeof(domains) && host[-1] != '.' && sizeof(host/".") < 3) {
      do_query(host, C_IN, T_A,
	       generic_get, 0, 0, T_A, "a", host, callback, @args );
    } else {
      do_query(host, C_IN, T_A,
	       generic_get, -1, 0, "a",
	       host, callback, @args);
    }
  }

  void ip_to_host(string ip, function callback, mixed ... args)
  {
    do_query(arpa_from_ip(ip), C_IN, T_PTR,
	     generic_get, -1, 0, T_PTR, "ptr",
	     ip, callback,
	     @args);
  }

  void get_mx_all(string host, function callback, mixed ... args)
  {
    mapping m;
    if(sizeof(domains) && host[-1] != '.' && sizeof(host/".") < 3) {
      do_query(host, C_IN, T_MX,
	       generic_get, 0, 1, T_MX, "mx", host, callback, @args);
    } else {
      do_query(host, C_IN, T_MX,
	       generic_get, -1, 1, T_MX, "mx", host, callback, @args);
    }
  }

  void get_mx(string host, function callback, mixed ... args)
  {
    get_mx_all(host,
	       lambda(string domain, array(mapping) mx,
		      function callback, mixed ... args) {
		 array a;
		 if (mx) {
		   a = column(mx, "mx");
		   sort(column(mx, "preference"), a);
		 }
		 callback(a, @args);
	       }, callback, args);
  }

  void create(void|string|array(string) server, void|string|array(string) domain)
  {
    if(!udp::bind(0))
      throw(({"DNS: failed to bind a port.\n",backtrace()}));

    udp::set_read_callback(rec_data);
    ::create(server,domain);
  }
};
