// $Id$
// Not yet finished -- Fredrik Hubinette

  //inherit Stdio.UDP : udp;
//! RFC 1034, RFC 1035 and RFC 2308
  protected void send_reply(mapping r, mapping q, mapping m, Stdio.UDP udp);

#pike __REAL_VERSION__

final constant NOERROR=0;
final constant FORMERR=1;
final constant SERVFAIL=2;
final constant NXDOMAIN=3;
final constant NOTIMPL=4;
final constant REFUSED=5;
final constant NXRRSET=8;

final constant QUERY=0;

//! Resource classes
enum ResourceClass
{
  //! Class Internet
  C_IN=1,

  //! Class CSNET (Obsolete)
  C_CS=2,

  //! Class CHAOS
  C_CH=3,

  //! Class Hesiod
  C_HS=4,

  //! Class ANY
  C_ANY=255,
};

//! Entry types
enum EntryType
{
  //! Type - host address
  T_A=1,

  //! Type - authoritative name server
  T_NS=2,

  //! Type - mail destination (Obsolete - use MX)
  T_MD=3,

  //! Type - mail forwarder (Obsolete - use MX)
  T_MF=4,

  //! Type - canonical name for an alias
  T_CNAME=5,

  //! Type - start of a zone of authority
  T_SOA=6,

  //! Type - mailbox domain name (EXPERIMENTAL)
  T_MB=7,

  //! Type - mail group member (EXPERIMENTAL)
  T_MG=8,

  //! Type - mail rename domain name (EXPERIMENTAL)
  T_MR=9,

  //! Type - null RR (EXPERIMENTAL)
  T_NULL=10,

  //! Type - well known service description
  T_WKS=11,

  //! Type - domain name pointer
  T_PTR=12,

  //! Type - host information
  T_HINFO=13,

  //! Type - mailbox or mail list information
  T_MINFO=14,

  //! Type - mail exchange
  T_MX=15,

  //! Type - text strings
  T_TXT=16,

  //! Type - IPv6 address record (RFC 1886, deprecated) 
  T_AAAA=28,

  //! Type - Location Record (RFC 1876)
  T_LOC=29,

  //! Type - Service location record (RFC 2782)
  T_SRV=33,

  //! Type - NAPTR (RFC 3403)
  T_NAPTR=35,

  //! Type - IPv6 address record (RFC 2874, incomplete support)
  T_A6=38,

  //! Type - SPF - Sender Policy Framework (RFC 4408)
  T_SPF=99,
};

//! Low level DNS protocol
class protocol
{
  string mklabel(string s)
  {
    if(sizeof(s)>63)
      error("Too long component in domain name.\n");
    return sprintf("%c%s",sizeof(s),s);
  }

  protected private string mkname(string|array(string) labels, int pos,
			       mapping(string:int) comp)
  {
    if(stringp(labels))
      labels = labels/"."-({""});
    if(!labels || !sizeof(labels))
      return "\0";
    string n = labels*".";
    if(comp[n])
      return sprintf("%2c", comp[n]|0xc000);
    else {
      if(pos<0x4000)
	comp[n]=pos;
      string l = mklabel(labels[0]);
      return l + mkname(labels[1..], pos+sizeof(l), comp);
    }
  }

  protected string make_raw_addr6(string addr6)
  {
    if(!addr6) return "\0"*16;
    if(has_value(addr6, "::")) {
      int parts = sizeof((addr6/":")-({""}));
      if(has_value(addr6, ".")) parts++;
      addr6 = replace(addr6, "::", ":"+"0:"*(8-parts));
      sscanf(addr6, ":%s", addr6);
    }
    if(has_value(addr6, "."))
      return sprintf("%2c%2c%2c%2c%2c%2c%1c%1c%1c%1c",
		     @array_sscanf(addr6, "%x:%x:%x:%x:%x:%x:%x.%x.%x.%x"));
    else
      return sprintf("%@2c",
		     array_sscanf(addr6, "%x:%x:%x:%x:%x:%x:%x:%x"));
  }

  protected private string mkrdata(mapping entry, int pos, mapping(string:int) c)
  {
    switch(entry->type) {
     case T_CNAME:
       return mkname(entry->cname, pos, c);
     case T_PTR:
       return mkname(entry->ptr, pos, c);
     case T_NS:
       return mkname(entry->ns, pos, c);
     case T_MD:
       return mkname(entry->md, pos, c);
     case T_MF:
       return mkname(entry->mf, pos, c);
     case T_MB:
       return mkname(entry->mb, pos, c);
     case T_MG:
       return mkname(entry->mg, pos, c);
     case T_MR:
       return mkname(entry->mr, pos, c);
     case T_MX:
       return sprintf("%2c", entry->preference)+mkname(entry->mx, pos+2, c);
     case T_HINFO:
       return sprintf("%1c%s%1c%s", sizeof(entry->cpu||""), entry->cpu||"",
		      sizeof(entry->os||""), entry->os||"");
     case T_MINFO:
       string rmailbx = mkname(entry->rmailbx, pos, c);
       return rmailbx + mkname(entry->emailbx, pos+sizeof(rmailbx), c);
     case T_SRV:
        return sprintf("%2c%2c%2c", entry->priority, entry->weight, entry->port) +
                      mkname(entry->target||"", pos+6, c);
     case T_A:
       return sprintf("%@1c", (array(int))((entry->a||"0.0.0.0")/".")[0..3]);
     case T_AAAA:
       return make_raw_addr6(entry->aaaa);
     case T_SOA:
       string mname = mkname(entry->mname, pos, c);
       return mname + mkname(entry->rname, pos+sizeof(mname), c) +
	 sprintf("%4c%4c%4c%4c%4c", entry->serial, entry->refresh,
		 entry->retry, entry->expire, entry->minimum);
     case T_NAPTR:
       string rnaptr = sprintf("%2c%2c", entry->order, entry->preference);
       rnaptr += sprintf("%1c%s%1c%s%1c%s%s",
			 sizeof(entry->flags || ""), entry->flags || "",
			 sizeof(entry->service || ""), entry->service || "",
			 sizeof(entry->regexp || ""), entry->regexp || "",
			 mkname(entry->replacement, pos, c));
       return rnaptr;

     case T_TXT:
       return Array.map(stringp(entry->txt)? ({entry->txt}):(entry->txt||({})),
			lambda(string t) {
			  return sprintf("%1c%s", sizeof(t), t);
			})*"";
     case T_SPF:
       return Array.map(stringp(entry->spf)? ({entry->spf}):(entry->spf||({})),
			lambda(string t) {
			  return sprintf("%1c%s", sizeof(t), t);
			})*"";
     case T_LOC:
       // FIXME: Not implemented yet.
     default:
       return "";
    }
  }

  protected private string encode_entries(array(mapping) entries, int pos,
				       mapping(string:int) comp)
  {
    string res="";
    foreach(entries, mapping entry) {
      string e = mkname(entry->name, pos, comp)+
	sprintf("%2c%2c%4c", entry->type, entry->cl, entry->ttl);
      pos += sizeof(e)+2;
      string rd = entry->rdata || mkrdata(entry, pos, comp);
      res += e + sprintf("%2c", sizeof(rd)) + rd;
      pos += sizeof(rd);
    }
    return res;
  }

  string low_low_mkquery(mapping q)
  {
    array qd = q->qd && (arrayp(q->qd)? q->qd : ({q->qd}));
    array an = q->an && (arrayp(q->an)? q->an : ({q->an}));
    array ns = q->ns && (arrayp(q->ns)? q->ns : ({q->ns}));
    array ar = q->ar && (arrayp(q->ar)? q->ar : ({q->ar}));
    string r = sprintf("%2c%c%c%2c%2c%2c%2c", q->id,
		       ((q->rd)&1)|(((q->tc)&1)<<1)|(((q->aa)&1)<<2)|
		       (((q->opcode)&15)<<3)|(((q->qr)&1)<<7),
		       ((q->rcode)&15)|(((q->cd)&1)<<4)|(((q->ad)&1)<<5)|
		       (((q->ra)&1)<<7),
		       qd && sizeof(qd), an && sizeof(an),
		       ns && sizeof(ns), ar && sizeof(ar));
    mapping(string:int) c = ([]);
    if(qd)
      foreach(qd, mapping _qd)
	r += mkname(_qd->name, sizeof(r), c) +
	sprintf("%2c%2c", _qd->type, _qd->cl);
    if(an)
      r+=encode_entries(an, sizeof(r), c);
    if(ns)
      r+=encode_entries(ns, sizeof(r), c);
    if(ar)
      r+=encode_entries(ar, sizeof(r), c);
    return r;
  }

  string low_mkquery(int id,
		    string dname,
		    int cl,
		    int type)
  {
    return low_low_mkquery((["id":id, "rd":1,
			     "qd":(["name":dname, "cl":cl, "type":type])]));
  }

//! create a DNS query PDU
//! 
//! @param dnameorquery
//! @param cl
//!   record class such as Protocols.DNS.C_IN
//! @param type
//!  query type such Protocols.DNS.T_A
//!
//! @returns
//!  data suitable for use with 
//!  @[Protocols.DNS.client.do_sync_query]
//!
//! @example
//! // generate a query PDU for a address lookup on the hostname pike.ida.liu.se
//! string q=Protocols.DNS.protocol()->mkquery("pike.ida.liu.se", Protocols.DNS.C_IN, Protocols.DNS.T_A);
  string mkquery(string|mapping dnameorquery, int|void cl, int|void type)
  {
    if(mappingp(dnameorquery))
      return low_low_mkquery(dnameorquery);
    else
      return low_mkquery(random(65536),dnameorquery,cl,type);
  }

  string decode_domain(string msg, array(int) n)
  {
    array(string) domains=({});
    
    int pos=n[0];
    int next=-1;
    array(string) ret=({});
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
	  if((~len)&0xc0) 
	    error("Invalid message compression mode.\n");
	  if(next==-1) next=pos+2;
	  pos=((len&63)<<8) + msg[pos+1];
	  continue;
      }
      break;
    }
  }

  string decode_string(string s, array(int) next)
  {
    int len=s[next[0]];
    next[0]+=len+1;
    return s[next[0]-len..next[0]-1];
  }

  int decode_byte(string s, array(int) next)
  {
    return s[next[0]++];
  }

  int decode_short(string s, array(int) next)
  {
    sscanf(s[next[0]..next[0]+1],"%2c",int ret);
    next[0]+=2;
    return ret;
  }

  int decode_int(string s, array(int) next)
  {
    sscanf(s[next[0]..next[0]+3],"%4c",int ret);
    next[0]+=4;
    return ret;
  }

  //! Decode a set of entries from an answer.
  //!
  //! @param s
  //!   Encoded entries.
  //!
  //! @param num
  //!   Number of entires in @[s].
  //!
  //! @param next
  //!   Array with a single element containing the start position in @[s]
  //!   on entry and the continuation position on return.
  //!
  //! @returns
  //!   Returns an array of mappings describing the decoded entires:
  //!   @array
  //!     @elem mapping 0..
  //!       Mapping describing a single entry:
  //!       @mapping
  //!         @member string "name"
  //!           Name the entry concerns.
  //!         @member EntryType "type"
  //!           Type of entry.
  //!         @member ResourceClass "cl"
  //!           Resource class. Typically @[C_IN].
  //!         @member int "ttl"
  //!           Time to live for the entry in seconds.
  //!         @member int "len"
  //!           Length in bytes of the encoded data section.
  //!       @endmapping
  //!       Depending on the type of entry the mapping may contain
  //!       different additional fields:
  //!       @int
  //!         @value T_CNAME
  //!           @mapping
  //!             @member string "cname"
  //!           @endmapping
  //!         @value T_PTR
  //!           @mapping
  //!             @member string "ptr"
  //!           @endmapping
  //!         @value T_NS
  //!           @mapping
  //!             @member string "ns"
  //!           @endmapping
  //!         @value T_MX
  //!           @mapping
  //!             @member int "preference"
  //!             @member string "mx"
  //!           @endmapping
  //!         @value T_HINFO
  //!           @mapping
  //!             @member string "cpu"
  //!             @member string "os"
  //!           @endmapping
  //!         @value T_SRV
  //!           RFC 2052 and RFC 2782.
  //!           @mapping
  //!             @member int "priority"
  //!             @member int "weight"
  //!             @member int "port"
  //!             @member string "target"
  //!             @member string "service"
  //!             @member string "proto"
  //!             @member string "name"
  //!           @endmapping
  //!         @value T_A
  //!           @mapping
  //!             @member string "a"
  //!               IPv4-address in dotted-decimal format.
  //!           @endmapping
  //!         @value T_AAAA
  //!           @mapping
  //!             @member string "aaaa"
  //!               IPv6-address in colon-separated hexadecimal format.
  //!           @endmapping
  //!         @value T_LOC
  //!           @mapping
  //!             @member int "version"
  //!               Version, currently only version @expr{0@} (zero) is
  //!               supported.
  //!             @member int "size"
  //!             @member int "h_perc"
  //!             @member int "v_perc"
  //!             @member int "lat"
  //!             @member int "long"
  //!             @member int "alt"
  //!           @endmapping
  //!         @value T_SOA
  //!           @mapping
  //!             @member string "mname"
  //!             @member string "rname"
  //!             @member int "serial"
  //!             @member int "refresh"
  //!             @member int "retry"
  //!             @member int "expire"
  //!
  //!             @member int "minimum"
  //!               Note: For historical reasons this entry is named
  //!               @expr{"minimum"@}, but it contains the TTL for
  //!               negative answers (RFC 2308).
  //!           @endmapping
  //!         @value T_NAPTR
  //!           @mapping
  //!             @member int "order"
  //!             @member int "preference"
  //!             @member string "flags"
  //!             @member string "service"
  //!             @member string "regexp"
  //!             @member string "replacement"
  //!           @endmapping
  //!         @value T_TXT
  //!           @mapping
  //!             @member string "txt"
  //!           @endmapping
  //!         @value T_SPF
  //!           @mapping
  //!             @member string "spf"
  //!           @endmapping
  //!       @endint
  //!   @endarray
  array decode_entries(string s,int num, array(int) next)
  {
    array(string) ret=({});
    for(int e=0;e<num && next[0]<sizeof(s);e++)
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
      case T_SRV:
	// RFC 2052 and RFC 2782.
        m->priority=decode_short(s,next);
        m->weight=decode_short(s,next);
        m->port=decode_short(s,next);
        m->target=decode_domain(s,next);
        array x=m->name/".";

	if(x[0][0..0]=="_")
	  m->service=x[0][1..];
	else
	  m->service=x[0];

        if(sizeof(x) > 1)
        {
          if(x[1][0..0]=="_")
            m->proto=x[1][1..];
          else
            m->proto=x[1];
        }

	m->name=x[2..]*".";
        break;
      case T_A:
	m->a=sprintf("%{.%d%}",values(s[next[0]..next[0]+m->len-1]))[1..];
	break;
      case T_AAAA:
	m->aaaa=sprintf("%{:%02X%02X%}",
		     values(s[next[0]..next[0]+m->len-1])/2)[1..];
	break;
      case T_LOC:
	m->version = decode_byte(s,next);
	if (m->version == 0)
	{
	  int aByte;
	  aByte  = decode_byte(s,next);
	  m->size = pow((aByte>>4)&0xf , aByte&0xf)/100.0;
	  aByte = decode_byte(s,next);
	  m->h_perc = pow((aByte>>4)&0xf , aByte&0xf)/100.0;
	  aByte = decode_byte(s,next);
	  m->v_perc = pow((aByte>>4)&0xf , aByte&0xf)/100.0;
	  m->lat = ((decode_int(s,next)-(2<<30))/3600000.0);
	  m->long = ((decode_int(s,next)-(2<<30))/3600000.0);
	  m->alt = ((decode_int(s,next)/100.0)-100000.0);
	}
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
      case T_NAPTR:
	m->order = decode_short (s, next);
        m->preference = decode_short (s, next);
        m->flags = decode_string (s, next);
        m->service = decode_string (s, next);
        m->regexp = decode_string (s, next);
        m->replacement = decode_domain (s, next);
        break;
      case T_TXT:
	m->txt = decode_string(s, next);
	break;
      case T_SPF:
	m->spf = decode_string(s, next);
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
    m->rd=m->c1&1;
    m->tc=(m->c1>>1)&1;
    m->aa=(m->c1>>2)&1;
    m->opcode=(m->c1>>3)&15;
    m->qr=(m->c1>>7)&1;
    
    m->rcode=m->c2&15;
    m->cd=(m->c2>>4)&1;
    m->ad=(m->c2>>5)&1;
    m->ra=(m->c2>>7)&1;
    
    m->length=sizeof(s);
    
    array(string) tmp=({});
    
    array(int) next=({12});
    m->qd = allocate(m->qdcount);
    for(int i=0; i<m->qdcount; i++) {
      m->qd[i]=(["name":decode_domain(s,next)]);
      sscanf(s[next[0]..next[0]+3],"%2c%2c",m->qd[i]->type, m->qd[i]->cl);
      next[0]+=4;
    }
    m->an=decode_entries(s,m->ancount,next);
    m->ns=decode_entries(s,m->nscount,next);
    m->ar=decode_entries(s,m->arcount,next);
    return m;
  }
};

//! Implements a Domain Name Service (DNS) server.
class server
{
  //!
  inherit protocol;

  //inherit Stdio.UDP : udp;

  array(Stdio.UDP) ports = ({});

  protected void send_reply(mapping r, mapping q, mapping m, Stdio.UDP udp)
  {
    // FIXME: Needs to handle truncation somehow.
    if(!r)
      r = (["rcode":4]);
    r->id = q->id;
    r->qr = 1;
    r->opcode = q->opcode;
    r->rd = q->rd;
    r->qd = r->qd || q->qd;
    string s = mkquery(r);
    udp->send(m->ip, m->port, s);
  }

  //! Reply to a query (stub).
  //!
  //! @param query
  //!   Parsed query.
  //!
  //! @param udp_data
  //!   Raw UDP data.
  //!
  //! @param cb
  //!   Callback you can call with the result instead of returning it.
  //!   In that case, return @expr{0@} (zero).
  //!  
  //!
  //! Overload this function to implement the proper lookup.
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) on failure, or a result mapping on success:
  //!   @mapping
  //!     @member int "rcode"
  //!     @member array(mapping(string:string|int))|void "an"
  //!       @array
  //!         @elem mapping(string:string|int) entry
  //!           @mapping
  //!             @member string|array(string) "name"
  //!             @member int "type"
  //!             @member int "cl"
  //!           @endmapping
  //!       @endarray
  //!     @member array|void "qd"
  //!     @member array|void "ns"
  //!     @member array|void "ar"
  //!   @endmapping
  protected mapping reply_query(mapping query, mapping udp_data,
				 function(mapping:void) cb)
  {
    // Override this function.
    //
    // Return mapping may contain:
    // aa, ra, {ad, cd,} rcode, an, ns, ar

    return 0;
  }

  protected void handle_query(mapping q, mapping m, Stdio.UDP udp)
  {
    int(0..1) result_available = 0;
    void _cb(mapping r) {
      if(result_available)
       error("DNS: you can either use the callback OR return the reply, not both.\n");
      result_available = 1;
      send_reply(r, q, m, udp);
    };
    mapping r = reply_query(q, m, _cb);
    if(r && result_available)
      error("DNS: you can either use the callback OR return the reply, not both.\n");
    if(r) {
      result_available = 1;
      send_reply(r, q, m, udp);
    }
  }

  protected void handle_response(mapping r, mapping m, Stdio.UDP udp)
  {
    // This is a stub intended to simplify servers which allow recursion
  }

  protected private void rec_data(mapping m, Stdio.UDP udp)
  {
    mixed err;
    mapping q;
    if (err = catch {
      q=decode_res(m->data);
    }) {
      werror("DNS: Failed to read UDP packet.\n%s\n",
	     describe_backtrace(err));
      if(m && m->data && sizeof(m->data)>=2)
	send_reply((["rcode":1]),
		   mkmapping(({"id"}), array_sscanf(m->data, "%2c")), m, udp);
    }
    else if(q->qr)
      handle_response(q, m, udp);
    else
      handle_query(q, m, udp);
  }

  void create(int|string|void arg1, string|int ... args)
  {
    if(!arg1 && !sizeof(args))
      arg1 = 53;
    if(!sizeof(args))
    {
      if(stringp(arg1))
       args = ({ arg1, 53 });
      else
       args = ({ 0, arg1 });
    }
    else
      args = ({ arg1 }) + args;
    if(sizeof(args)&1)
      error("DNS: if you specify more than one argument, the number of "
           "arguments needs to be even (server(ip1, port1, ip2, port2, "
           "...)).\n");
    for(int i;i<sizeof(args);i+=2) {
      Stdio.UDP udp = Stdio.UDP();
      if(args[i]) {
       if (!udp->bind(args[i+1],args[i]))
         error("DNS: failed to bind host:port %s:%d.\n", args[i],args[i+1]);
      } else {
       if(!udp->bind(args[i+1]))
         error("DNS: failed to bind port %d.\n", args[i+1]);
      }
      udp->set_read_callback(rec_data, udp);
      // port objects are stored for destruction when the server object is destroyed.
      ports += ({udp});
    }

  }

  static void destory()
  {
    if(sizeof(ports))
    {
      foreach(ports;; Stdio.UDP port)
        destruct(port);
    }
  }

}


#define RETRIES 12
#define RETRY_DELAY 5

//! 	Synchronous DNS client.
class client 
{
  inherit protocol;

#ifdef __NT__
  array(string) get_tcpip_param(string val, void|string fallbackvalue)
  {
    array(string) res = ({});
    foreach(({
      "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
      "SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP"
    }),string key)
    {
      catch {
	res += ({ RegGetValue(HKEY_LOCAL_MACHINE, key, val) });
      };
    }

#if constant(RegGetKeyNames)
    /* Catch if RegGetKeyNames() doesn't find the directory. */
    catch {
      foreach(RegGetKeyNames(HKEY_LOCAL_MACHINE,
			     "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\"
			     "Parameters\\Interfaces"), string key)
      {
	catch {
	  res += ({ RegGetValue(HKEY_LOCAL_MACHINE,
				"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\"
				"Parameters\\Interfaces\\" + key, val) });
	};
      }
    };
#endif
    res -= ({ UNDEFINED });
    return sizeof(res) ? res : ({ fallbackvalue });
  }

#else /* !__NT__ */

  protected private mapping(string:string) etc_hosts;

  protected private int is_ip(string ip)
  {
    // FIXME: Doesn't work with IPv6
    return (replace(ip, "0123456789."/1, allocate(11,"")) == "");
  }

  protected private string read_etc_file(string fname)
  {
    array(string) paths;
    string res;
#ifdef __NT__
    paths = get_tcpip_param("DataBasePath");
#else
    paths = ({ "/etc", "/amitcp/db" });
#endif
    foreach(paths, string path) {
      string raw = Stdio.read_file(path + "/" + fname);
      if (raw && sizeof(raw = replace(raw, "\r", "\n"))) {
	if (res) {
	  if (sizeof(res) && (res[-1] != '\n')) {
	    // Avoid confusion...
	    res += "\n";
	  }
	  res += raw;
	} else {
	  res = raw;
	}
      }
    }
    return res;
  }

  protected private string match_etc_hosts(string host)
  {
    if (!etc_hosts) {
      etc_hosts = ([ "localhost":"127.0.0.1" ]);
	
      string raw = read_etc_file("hosts");
	
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
	// Couldn't read or no  /etc/hosts.
      }
    }
    return etc_hosts[lower_case(host)];
  }
#endif /* !__NT__ */

  // FIXME: Read hosts entry in /etc/nswitch.conf?

  //! @decl void create()
  //! @decl void create(void|string|array server, void|int|array domain)

  array(string) nameservers = ({});
  array domains = ({});
  void create(void|string|array(string) server, void|int|array(string) domain)
  {
    if(!server)
    {
#if __NT__
      domains = get_tcpip_param("Domain", "") +
	get_tcpip_param("DhcpDomain", "") +
	map(get_tcpip_param("SearchList", ""),
	    lambda(string s) {
	      return replace(s, " ", ",")/",";
	    }) * ({});

      nameservers = map(get_tcpip_param("NameServer", "") +
			get_tcpip_param("DhcpNameServer", ""),
			lambda(string s) {
			  return replace(s, " ", ",")/",";
			}) * ({});
#else
      string domain;
      string resolv_conf;
      foreach(({"/etc/resolv.conf", "/amitcp/db/resolv.conf"}),
	      string resolv_loc)
        if ((resolv_conf = Stdio.read_file(resolv_loc)))
	  break;

      if (!resolv_conf) {
	if (System->get_netinfo_property) {
	  //  Mac OS X / Darwin (and possibly other systems) that use
	  //  NetInfo may have these values in the database.
	  if (nameservers =
	      System->get_netinfo_property(".",
					   "/locations/resolver",
					   "nameserver")) {
	    nameservers = map(nameservers, `-, "\n");
	  } else {
	    nameservers = ({});
	  }
	  
	  if (domains = System->get_netinfo_property(".",
						    "/locations/resolver",
						    "domain")) {
	    domains = map(domains, `-, "\n");
	  } else {
	    domains = ({});
	  }
	} else {
	  /* FIXME: Is this a good idea?
	   * Why not just try the fallback?
	   * /grubba 1999-04-14
	   *
	   * Now uses 127.0.0.1 as fallback.
	   * /grubba 2000-10-17
	   */
	  resolv_conf = "nameserver 127.0.0.1";
	}
#if 0
	error( "Protocols.DNS.client(): No /etc/resolv.conf!\n" );
#endif /* 0 */
      }

      if (resolv_conf)
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
	      domain = sizeof(rest) && rest;
	      break;
	    case "search":
	      rest = replace(rest, "\t", " ");
	      domains += ((rest/" ") - ({""}));
	      break;
	      
	    case "nameserver":
	      if (!is_ip(rest)) {
		// Not an IP-number!
		string host = rest;
		if (!(rest = match_etc_hosts(host))) {
		  werror("Protocols.DNS.client(): "
			 "Can't resolv nameserver \"%s\"\n", host);
		  break;
		}
	      }
	      if (sizeof(rest)) {
		nameservers += ({ rest });
	      }
	      break;
	  }
	}

      if(domain)
	domains = ({ domain }) + domains;
#endif
      nameservers -= ({ "" });
      if (!sizeof(nameservers)) {
	/* Try localhost... */
	nameservers = ({ "127.0.0.1" });
      }
      domains -= ({ "" });
      domains = Array.map(domains, lambda(string d) {
				     if (d[-1] == '.') {
				       return d[..<1];
				     }
				     return d;
				   });
    } 
    else 
    {
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

//! perform a syncronous query
//! 
//! @param s
//!   result of @[Protocols.DNS.protocol.mkquery]
//! @returns
//!  mapping containing query result or 0 on failure/timeout
//!
//! @example
//! // perform a hostname lookup, results stored in r->an
//! object d=Protocols.DNS.client();
//! mapping r=d->do_sync_query(d->mkquery("pike.ida.liu.se", C_IN, T_A));
  mapping do_sync_query(string s)
  {
    int i;
    object udp = Stdio.UDP();
    // Attempt to randomize the source port.
    for (i = 0; i < RETRIES; i++) {
      if (!catch { udp->bind(1024 + random(65536-1024)); }) continue;
    }
    if (i >= RETRIES) udp->bind(0);
#if 0
    werror("Protocols.DNS.client()->do_sync_query(%O)\n"
	   "UDP Address: %s\n"
	   "%s\n", s, udp->query_address(), describe_backtrace(backtrace()));
#endif /* 0 */
    mapping m;
    for (i=0; i < RETRIES; i++) {
      udp->send(nameservers[i % sizeof(nameservers)], 53, s);

      // upd->wait() can throw an error sometimes.
      catch
      {
  	while (udp->wait(RETRY_DELAY))
        {
  	  // udp->read() can throw an error on connection refused.
  	  catch {
  	    m = udp->read();
  	    if ((m->port == 53) &&
  		(m->data[0..1] == s[0..1]) &&
  		has_value(nameservers, m->ip)) {
  	      // Success.
  	      return decode_res(m->data);
  	    }
  	  };
        }
      };
    }
    // Failure.
    return 0;
  }

  protected mapping low_gethostbyname(string s, int type)
  {
    mapping m;
    if(sizeof(domains) && s[-1] != '.' && sizeof(s/".") < 3) {
      mapping m = do_sync_query(mkquery(s, C_IN, type));
      if(!m || !m->an || !sizeof(m->an))
	foreach(domains, string domain)
	{
	  m = do_sync_query(mkquery(s+"."+domain, C_IN, type));
	  if(m && m->an && sizeof(m->an))
	    break;
	}
      return m;
    } else {
      return do_sync_query(mkquery(s, C_IN, type));
    }
  }
  
  //! @decl array gethostbyname(string hostname)
  //!	Queries the host name from the default or given
  //!	DNS server. The result is an array with three elements,
  //!
  //! @returns
  //!   An array with the requested information about the specified
  //!   host.
  //!
  //!	@array
  //!     @elem string hostname
  //!       Hostname.
  //!     @elem array(string) ip
  //!       IP number(s).
  //!     @elem array(string) aliases
  //!       DNS name(s).
  //!	@endarray
  //!
  //! @note
  //!   Prior to Pike 7.7 this function only returned IPv4 addresses.
  //!
  array gethostbyname(string s)
  {
    mapping a_records    = low_gethostbyname(s, T_A);
    mapping a6_records   = low_gethostbyname(s, T_A6);
    mapping aaaa_records = low_gethostbyname(s, T_AAAA);

#if 0
    werror("a_records: %O\n"
	   "a6_records: %O\n"
	   "aaaa_records: %O\n",
	   a_records, a6_records, aaaa_records);
#endif /* 0 */

    array(string) names=({});
    array(string) ips=({});
    if (a_records) {
      foreach(a_records->an, mapping x)
      {
	if(x->name)
	  names+=({x->name});
	if(x->a)
	  ips+=({x->a});
      }
    }
    // Prefer a6 to aaaa.
    if (a6_records) {
      foreach(a6_records->an, mapping x)
      {
	if(x->name)
	  names+=({x->name});
	if(x->a6)
	  ips+=({x->a6});
      }
    }
    if (aaaa_records) {
      foreach(aaaa_records->an, mapping x)
      {
	if(x->name)
	  names+=({x->name});
	if(x->aaaa)
	  ips+=({x->aaaa});
      }
    }
    
    return ({
      sizeof(names)?names[0]:0,
      ips,
      names,
    });
  }

  //!	Queries the service record (RFC 2782) from the default or given
  //!	DNS server. The result is an array of arrays with the
  //!   following six elements for each record. The array is
  //!   sorted according to the priority of each record.
  //!
  //!   Each element of the array returned represents a service 
  //!   record. Each service record contains the following:
  //!
  //! @returns
  //!   An array with the requested information about the specified
  //!   service.
  //!
  //!	@array
  //!     @elem int priority
  //!       Priority
  //!     @elem int weight
  //!       Weight in event of multiple records with same priority.
  //!     @elem int port
  //!       port number
  //!     @elem string target
  //!       target dns name
  //!	@endarray
  //!
  array getsrvbyname(string service, string protocol, string|void name)
  {
    mapping m;
    if(!service) error("no service name specified.");
    if(!protocol) error("no service name specified.");

    if(sizeof(domains) && !name) { 
      // we haven't provided a target domain to search on.
      // we probably shouldn't do a searchdomains search, 
      // as it might return ambiguous results.
      m = do_sync_query(
        mkquery("_" + service +"._"+ protocol + "." + name, C_IN, T_SRV));
      if(!m || !m->an || !sizeof(m->an))
	foreach(domains, string domain)
	{
          m = do_sync_query(
            mkquery("_" + service +"._"+ protocol + "." + domain, C_IN, T_SRV));
	  if(m && m->an && sizeof(m->an))
	    break;
	}
    } else {
      m = do_sync_query(
        mkquery("_" + service +"._"+ protocol + "." + name, C_IN, T_SRV));
    }

    if (!m) { // no entries.
      return ({});
    }
    
    array res=({});

    foreach(m->an, mapping x)
    {
       res+=({({x->priority, x->weight, x->port, x->target})});
    }

    // now we sort the array by priority as a convenience
    array y=({});
    foreach(res, array t)
      y+=({t[0]});
    sort(y, res);

    return res;
  }

  string arpa_from_ip(string ip)
  {
    if(has_value(ip,':')) {
      string raw_ipv6 = make_raw_addr6(ip);
      return reverse(sprintf("%@02X", values(raw_ipv6)))/1*"."+".IP6.ARPA";
    } else
      return reverse(ip/".")*"."+".IN-ADDR.ARPA";
  }

  string ip_from_arpa(string arpa)
  {
    array(string) parts = reverse(arpa/".");
    if(sizeof(parts)<2) return "";
    if(lower_case(parts[1]) == "ip6")
      return map(parts[2..]/4, `*, "")*":";
    else
      return parts[2..]*".";
  }

  //! @decl array gethostbyaddr(string hostip)
  //!	Queries the host name or ip from the default or given
  //!	DNS server. The result is an array with three elements,
  //!
  //! @returns
  //!   The requested data about the specified host.
  //!
  //!	@array
  //!     @elem string hostip
  //!       The host IP.
  //!     @elem array(string) ip
  //!       IP number(s).
  //!     @elem array(string) aliases
  //!       DNS name(s).
  //!	@endarray
  //!
  array gethostbyaddr(string s)
  {
    mapping m=do_sync_query(mkquery(arpa_from_ip(s), C_IN, T_PTR));
    if (m) {
      array(string) names=({});
      array(string) ips=({});

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
    } else {
      // Lookup failed.
      return ({ 0, ({}), ({}) });
    }
  }

  //! @decl string get_primary_mx(string hostname)
  //!	Queries the primary mx for the host.
  //! @returns
  //!   Returns the hostname of the primary mail exchanger.
  //!
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
    if (!m) {
      return 0;
    }
    int minpref=29372974;
    string ret;
    foreach(m->an, mapping m2)
    {
      if(m2->mx && m2->preference<minpref)
      {
	ret=m2->mx;
	minpref=m2->preference;
      }
    }
    return ret;
  }

  array(string) get_mx(string host)
  {
    mapping m;
    if(sizeof(domains) && host[-1] != '.' && sizeof(host/".") < 3) {
      m = do_sync_query(mkquery(host, C_IN, T_MX));
      if(!m || !m->an || !sizeof(m->an)) {
	foreach(domains, string domain)
	{
	  m = do_sync_query(mkquery(host+"."+domain, C_IN, T_MX));
	  if(m && m->an && sizeof(m->an))
	    break;
	}
      }
    } else {
      m = do_sync_query(mkquery(host, C_IN, T_MX));
    }
    if (!m) {
      return 0;
    }
    array a = filter(m->an, `[], "mx");
    array(string) b = column( a, "mx");
    sort( column( a, "preference"), b);

    return b;
  }
}

#define REMOVE_DELAY 120
#define GIVE_UP_DELAY (RETRIES * RETRY_DELAY + REMOVE_DELAY)*2

// FIXME: Randomized source port!
class async_client
{
  inherit client;
  inherit Stdio.UDP : udp;
  async_client next_client;

  class Request
  {
    string req;
    string domain;
    function callback;
    int retries;
    int timestamp;
    array args;
  };

  mapping requests=([]);

  protected private void remove(object(Request) r)
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
    for(int e=next_client ? 100 : 256;e>=0;e--)
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

  protected private void rec_data(mapping m)
  {
    mixed err;
    if (err = catch {
      if(m->port != 53 || !has_value(nameservers, m->ip)) return;
      sscanf(m->data,"%2c",int id);
      object r=requests[id];
      if(!r) return;
      m_delete(requests,id);
      r->callback(r->domain,decode_res(m->data),@r->args);
      destruct(r);
    }) {
      werror("DNS: Failed to read UDP packet. Connection refused?\n%s\n",
	     describe_backtrace(err));
    }
  }

  protected private void generic_get(string d,
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
	       generic_get, -1, 0, T_A, "a",
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
	       }, callback, @args);
  }

  void close()
  {
    udp::close();
    udp::set_read_callback(0);
  }

  void create(void|string|array(string) server, void|string|array(string) domain)
  {
    if(!udp::bind(0))
      error( "DNS: failed to bind a port.\n" );
#if 0
    werror("Protocols.DNS.async_client(%O, %O)\n"
	   "UDP Address: %s\n"
	   "%s\n", server, domain, udp::query_address(),
	   describe_backtrace(backtrace()));
#endif /* 0 */

    udp::set_read_callback(rec_data);
    ::create(server,domain);
  }
};


async_client global_async_client;

#define GAC(X)								\
void async_##X( string host, function callback, mixed ... args ) 	\
{									\
  if( !global_async_client )						\
    global_async_client = async_client();				\
  global_async_client->X(host,callback,@args);				\
}

//! @ignore
GAC(ip_to_host);
//! @endignore
//! @decl void async_ip_to_host(string ip, function cb, mixed ... cba)

//! @ignore
GAC(host_to_ip);
//! @endignore
//! @decl void async_host_to_ip(string host, function cb, mixed ... cba)

//! @ignore
GAC(get_mx_all);
//! @endignore
//! @decl void async_get_mx_all(string host, function cb, mixed ... cba)

//! @ignore
GAC(get_mx);
//! @endignore
//! @decl void async_get_mx(string host, function cb, mixed ... cba)


client global_client;

#define GC(X)							\
mixed X( string host ) 	                                        \
{								\
  if( !global_client )						\
    global_client = client();				        \
  return global_client->X(host);				\
}

//! @ignore
GC(gethostbyname);
//! @endignore
//! @decl array gethostbyname(string host)

//! @ignore
GC(gethostbyaddr);
//! @endignore
//! @decl array gethostbyaddr(string host)

//! @ignore
GC(get_mx);
//! @endignore
//! @decl string get_mx(string host)

//! @ignore
GC(get_primary_mx);
//! @endignore
//! @decl string get_primary_mx(string host)
