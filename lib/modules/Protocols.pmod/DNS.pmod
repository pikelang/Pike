// Not yet finished -- Fredrik Hubinette

  //inherit Stdio.UDP : udp;

//! Support for the Domain Name System protocol.
//!
//! Implements @rfc{1034@}, @rfc{1035@} and @rfc{2308@}.

protected void send_reply(mapping r, mapping q, mapping m, Stdio.UDP udp);

#pike __REAL_VERSION__

// documentation taken from RFC 2136

//! No error condition.
final constant NOERROR=0;

//! The name server was unable to interpret the request due to a
//! format error.
final constant FORMERR=1;

//! The name server encountered an internal failure while processing
//! this request, for example an operating system error or a
//! forwarding timeout.
final constant SERVFAIL=2;

//! Some name that ought to exist, does not exist.
final constant NXDOMAIN=3;

//! The name server does not support the specified Opcode.
final constant NOTIMP=4;
final constant NOTIMPL=4;		// Deprecated

//! The name server refuses to perform the specified operation for
//! policy or security reasons.
final constant REFUSED=5;

//! Name that should not exist, does exist.
final constant YXDOMAIN=6;

//! RRset that should not exist, does exist.
final constant YXRRSET=7;

//! Some RRset that ought to exist, does not exist.
final constant NXRRSET=8;

//! Server not authoritative for zone.
final constant NOTAUTH=9;

//! Name not contained in zone.
final constant NOTZONE=10;

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

  //! Type - mailbox domain name (Obsolete)
  T_MB=7,

  //! Type - mail group member (Obsolete)
  T_MG=8,

  //! Type - mail rename domain name (Obsolete)
  T_MR=9,

  //! Type - null RR (Obsolete @rfc{1035@})
  T_NULL=10,

  //! Type - well known service description (Obsolete @rfc{1123@} and
  //! @rfc{1127@})
  T_WKS=11,

  //! Type - domain name pointer
  T_PTR=12,

  //! Type - host information
  T_HINFO=13,

  //! Type - mailbox or mail list information (Obsolete)
  T_MINFO=14,

  //! Type - mail exchange
  T_MX=15,

  //! Type - text strings
  T_TXT=16,

  //! Type - Responsible Person
  T_RP=17,

  //! Type - AFC database record (@rfc{1183@})
  T_AFSDB=18,

  //! Type - X25 PSDN address (@rfc{1183@})
  T_X25=19,

  //! Type - ISDN address (@rfc{1183@})
  T_ISDN=20,

  //! Type - Route Through (@rfc{1183@})
  T_RT=21,

  //! Type - OSI Network Service Access Protocol (@rfc{1348@},
  //! @rfc{1637@} and @rfc{1706@})
  T_NSAP=22,

  //! Type - OSI NSAP Pointer (@rfc{1348@} and Obsolete @rfc{1637@})
  T_NSAP_PTR=23,

  //! Type - Signature (@rfc{2535@})
  T_SIG=24,

  //! Type - Key record (@rfc{2535@} and @rfc{2930@})
  T_KEY=25,

  //! Type - Pointer to X.400 mapping information (@rfc{1664@})
  T_PX=26,

  //! Type - Global Position (@rfc{1712@} Obsolete use LOC).
  T_GPOS=27,

  //! Type - IPv6 address record (@rfc{1886@})
  T_AAAA=28,

  //! Type - Location Record (@rfc{1876@})
  T_LOC=29,

  //! Type - Next (@rfc{2065@} and Obsolete @rfc{3755@})
  T_NXT=30,

  //! Type - Nimrod Endpoint IDentifier (draft)
  T_EID=31,

  //! Type - Nimrod Locator (draft)
  T_NIMLOC=32,

  //! Type - Service location record (@rfc{2782@})
  T_SRV=33,

  //! Type - ATM End System Address (af-saa-0069.000)
  T_ATMA=34,

  //! Type - NAPTR (@rfc{3403@})
  T_NAPTR=35,

  //! Type - Key eXchanger record (@rfc{2230@})
  T_KX=36,

  //! Type - Certificate Record (@rfc{4398@})
  T_CERT=37,

  //! Type - IPv6 address record (@rfc{2874@} and Obsolete @rfc{6563@})
  T_A6=38,

  //! Type - Delegation Name (@rfc{2672@})
  T_DNAME=39,

  //! Type - Kitchen Sink (draft)
  T_SINK=40,

  //! Type - Option (@rfc{2671@})
  T_OPT=41,

  //! Type - Address Prefix List (@rfc{3123@})
  T_APL=42,

  //! Type - Delegation Signer (@rfc{4034@})
  T_DS=43,

  //! Type - SSH Public Key Fingerprint (@rfc{4255@})
  T_SSHFP=44,

  //! Type - IPsec Key (@rfc{4025@})
  T_IPSECKEY=45,

  //! Type - DNSSEC signature (@rfc{4034@})
  T_RRSIG=46,

  //! Type - Next-Secure record (@rfc{4034@})
  T_NSEC=47,

  //! Type - DNS Key record (@rfc{4034@})
  T_DNSKEY=48,

  //! Type - DHCP identifier (@rfc{4701@})
  T_DHCID=49,

  //! Type - NSEC record version 3 (@rfc{5155@})
  T_NSEC3=50,

  //! Type - NSEC3 parameters (@rfc{5155@})
  T_NSEC3PARAM=51,

  //! Type - TLSA certificate association (@rfc{6698@})
  T_TLSA=52,

  //! Type - Host Identity Protocol (@rfc{5205@})
  T_HIP=55,

  //! Type - SPF - Sender Policy Framework (@rfc{4408@})
  T_SPF=99,

  // UserDB via DNS?
  T_UINFO=100,
  T_UID=101,
  T_GID=102,
  T_UNSPEC=103,

  //! Type - Secret key record (@rfc{2930@})
  T_TKEY=249,

  //! Type - Transaction Signature (@rfc{2845@})
  T_TSIG=250,

  //! Type - Incremental Zone Transfer (@rfc{1996@})
  T_IXFR=251,

  //! Type - Authoritative Zone Transfer (@rfc{1035@})
  T_AXFR=252,

  //! Type - Mail Box (MB, MG or MR) (Obsolete - use MX)
  T_MAILB=253,

  //! Type - Mail Agent (both MD and MF) (Obsolete - use MX)
  T_MAILA=254,

  //! Type - ANY - A request for all records
  T_ANY=255,

  //! Type - Certificate Authority Authorization (@rfc{6844@})
  T_CAA=257,

  //! Type - DNSSEC Trust Authorities (draft)
  T_TA=32768,

  //! Type - DNSSEC Lookaside Validation Record (@rfc{4431@})
  T_DLV=32769,
};

//! Flag bits used in @[T_DNSKEY] RRs.
enum DNSKEY_Flags {
  F_ZONEKEY		= 0x0100,	//! Zone Key.
  F_SECUREENTRYPOINT	= 0x0001,	//! Secure Entry Point.
};

//! DNSSEC Protocol types.
//!
//! @note
//!   @rfc{4034@} obsoleted all but @[DNSSEC_DNSSEC].
enum DNSSEC_Protocol {
  DNSSEC_TLS		= 1,	//! Reserved for use by TLS.
  DNSSEC_EMAIL		= 2,	//! Reserved for use by SMTP et al.
  DNSSEC_DNSSEC		= 3,	//! Key for use by DNSSEC. @rfc{4034:2.1.2@}.
  DNSSEC_IPSEC		= 4,	//! Reserved for use by IPSEC.
  DNSSEC_ALL		= 255,	//! Any use. Discouraged.
};

//! DNSSEC Algorithm types.
enum DNSSES_Algorithm {
  DNSSEC_RSAMD5		= 1,	//! RSA/MD5 @rfc{2537@}.
  DNSSEC_DH		= 2,	//! Diffie-Hellman @rfc{2539@}.
  DNSSEC_DSA		= 3,	//! DSA/SHA1 @rfc{2536@}.
  DNSSEC_ECC		= 4,
  DNSSEC_RSASHA1	= 5,	//! RSA/SHA1 @rfc{3110@}.

  DNSSEC_INDIRECT	= 252,
  DNSSEC_PRIVATEDNS	= 253,	//! Private algorithm DNS-based @rfc{4035:A.1.1@}.
  DNSSEC_PRIVATEOID	= 254,	//! Private algorithm OID-based @rfc{4035:A.1.1@}.
};

//! DNSSEC Digest types.
enum DNSSEC_Digests {
  DNSSEC_SHA1		= 1,	//! SHA1 digest @rfc{4035:A.2@}.
};

int safe_bind(Stdio.UDP udp, string|int port, string|void device)
{
  mixed err = catch {
      udp->bind(port, device, 1);
      return 1;
    };
#if constant(System.EADDRINUSE)
  if (errno() == System.EADDRINUSE) return 0;
#endif
#if constant(System.EACCES)
  if (errno() == System.EACCES) return 0;	// Privileged port.
#endif
#if constant(System.WSAEACCES)
  if (errno() == System.WSAEACCES) return 0;
#endif
  werror("Protocols.DNS: Binding of UDP port failed with errno %d: %s.\n",
         errno(), strerror(errno()));
  master()->handle_error(err);
  return 0;
}

//! Low level DNS protocol
class protocol
{
  string mklabel(string s)
  {
    if(sizeof(s)>63)
      error("Too long component in domain name.\n");
    return sprintf("%1H",s);
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
    return sprintf ("%@2c", Protocols.IPv6.parse_addr (addr6));
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
       return sprintf("%1H%1H", entry->cpu||"", entry->os||"");
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
     case T_A6:
       if( stringp( entry->a6 ) || !entry->a6 )
         return "\0"+make_raw_addr6(entry->a6);
       return sprintf( "%c%s%s",
                       entry->a6->prefixlen,
                       make_raw_addr6(entry->a6->address)[entry->a6->prefixlen/8..],
                       entry->a6->prefixname||"");
     case T_SOA:
       string mname = mkname(entry->mname, pos, c);
       return mname + mkname(entry->rname, pos+sizeof(mname), c) +
	 sprintf("%4c%4c%4c%4c%4c", entry->serial, entry->refresh,
		 entry->retry, entry->expire, entry->minimum);
     case T_NAPTR:
       string rnaptr = sprintf("%2c%2c", entry->order, entry->preference);
       rnaptr += sprintf("%1H%1H%1H%s",
			 entry->flags || "",
			 entry->service || "",
			 entry->regexp || "",
			 mkname(entry->replacement, pos, c));
       return rnaptr;

     case T_TXT:
       return map(stringp(entry->txt)? ({entry->txt}):(entry->txt||({})),
                  lambda(string t) {
                    return sprintf("%1H", t);
                  })*"";
     case T_SPF:
       return map(stringp(entry->spf)? ({entry->spf}):(entry->spf||({})),
                  lambda(string t) {
                    return sprintf("%1H", t);
                  })*"";
     case T_LOC:
       int encode_T_LOC_tinyfloat(float|int subject)
       {
	 int power = min((int)(log(subject*100.0)/log(10.0)), 9);
	 int base =  min((int)(subject*100.0/pow(10.0,power)), 9);
	 return ((base&0xf)<<4)|(power&0xf);
       };
       if ((entry->version? entry->version:1) != 1)
	 error("Only T_LOC version 1 is supported");
       return sprintf("%1c%1c%1c%1c%4c%4c%4c",
		      0, // Only version that currently exists
		      encode_T_LOC_tinyfloat(entry->size? entry->size:100.0), //Default is 1M
		      encode_T_LOC_tinyfloat(entry->h_prec? entry->h_prec:1000*100.0), // Default is 10KM
		      encode_T_LOC_tinyfloat(entry->v_prec? entry->v_prec:10*100.0), // Default is 10M
		      entry->lat?(int)round(entry->lat*3600000.0)+(2<<30):2<<30, // Default is 2<<30 which is 0.0
		      entry->long?(int)round(entry->long*3600000.0)+(2<<30):2<<30, // Default is 2<<30 which is 0.0
		      entry->alt?(int)round((entry->alt+100000)*100):100000, // Default to 0 WGS84 (which is 100000)
		      );
     case T_CAA:
       if (entry->tag == "" || !entry->tag)
         error("An empty tag is not permitted.\n");
       return sprintf("%c%H%s", entry->flags | (!!entry->critical << 7),
                      entry->tag, entry->value || "");
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
      res += e + sprintf("%2H", rd);
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
  //!   query type such Protocols.DNS.T_A
  //!
  //! @returns
  //!  data suitable for use with
  //!  @[Protocols.DNS.client.do_sync_query]
  //!
  //! @example
  //! // generate a query PDU for a address lookup on the hostname pike.lysator.liu.se
  //! string q=Protocols.DNS.protocol()->mkquery("pike.lysator.liu.se", Protocols.DNS.C_IN, Protocols.DNS.T_A);
  string mkquery(string|mapping dnameorquery, int|void cl, int|void type)
  {
    if(mappingp(dnameorquery))
      return low_low_mkquery(dnameorquery);
    else
      return low_mkquery(random(65536),dnameorquery,cl,type);
  }

  string decode_domain(string msg, array(int) n)
  {
    int pos=n[0];
    int next=-1;
    array(string) ret=({});
    int labels = 0;
    while(pos < sizeof(msg))
    {
      labels++;
      if (labels > 255)
        error("Bad domain name. Too many labels.\n");
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
    int pos = next[0];
    int len=s[pos];
    next[0]+=len+1;
    return s[pos+1..pos+len];
  }

  int decode_byte(string s, array(int) next)
  {
    return s[next[0]++];
  }

  int decode_short(string s, array(int) next)
  {
    return s[next[0]++]<<8 | s[next[0]++];
  }

  int decode_int(string s, array(int) next)
  {
    int pos = next[0];
    next[0] += 4;
    return s[pos++]<<24 | s[pos++]<<16 | s[pos++]<<8 | s[pos++];
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
  //!           @rfc{2052@} and @rfc{2782@}.
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
  //!             @member float "size"
  //!             @member float "h_perc"
  //!             @member float "v_perc"
  //!             @member float "lat"
  //!             @member float "long"
  //!             @member float "alt"
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
  //!               negative answers (@rfc{2308@}).
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
  //!               Note: For historical reasons, when receiving decoded
  //!               DNS entries from a client, this will be the first string
  //!               in the TXT record only.
  //!             @member string "txta"
  //!               When receiving decoded DNS data from a client, txta is
  //!               the array of all strings in the record. When sending
  //!               multiple strings in a TXT record in a server, please
  //!               supply an array as "txt" containing the strings, txta
  //!               will be ignored.
  //!           @endmapping
  //!         @value T_SPF
  //!           @mapping
  //!             @member string "spf"
  //!           @endmapping
  //!         @value T_CAA
  //!           @mapping
  //!             @member int "critical"
  //!               Sets the critical bit of the flag field.
  //!             @member int "flags"
  //!
  //!             @member string "tag"
  //!               Cannot be empty.
  //!             @member string "value"
  //!         @endmapping
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
	m->a=(array(string))values(s[next[0]..next[0]+m->len-1])*".";
	break;
      case T_AAAA:
	m->aaaa=sprintf("%{%02X%}",
                        (values(s[next[0]..next[0]+m->len-1])/2)[*])*":";
	break;
      case T_LOC:
	m->version = decode_byte(s,next);
	if (m->version == 0)
	{
	  int aByte;
	  aByte  = decode_byte(s,next);
	  m->size = (((aByte>>4)&0xf)%10)*(pow(10,(aByte&0xf)%10)/100.0);
	  aByte = decode_byte(s,next);
	  m->h_perc = (((aByte>>4)&0xf)%10)*(pow(10,(aByte&0xf)%10)/100.0);
	  aByte = decode_byte(s,next);
	  m->v_perc = (((aByte>>4)&0xf)%10)*(pow(10,(aByte&0xf)%10)/100.0);
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
	{
	    int tlen;

	    m->txta = ({ });
	    while (tlen < m->len) {
		m->txta += ({ decode_string(s, next) });
		tlen += sizeof(m->txta[-1]) + 1;
	    }
	    m->txt = m->txta[0];
	}
	break;
      case T_SPF:
	m->spf = decode_string(s, next);
	break;
      case T_CAA:
        {
          string tag;

          m->critical = !!((m->flags = decode_byte(s, next)) & 0x80);
          tag = m->tag = decode_string(s, next);
          m->value = s[next[0]..next[0] + m->len - 3 - sizeof(tag)];
        }
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
    if (sscanf(s,"%2c%c%c%2c%2c%2c%2c",
               m->id,
               m->c1,
               m->c2,
               m->qdcount,
               m->ancount,
               m->nscount,
               m->arcount) != 7)
      error("Bad DNS request, failed to parse header\n");
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

    array(int) next=({12});
    m->qd = allocate(m->qdcount);
    for(int i=0; i<m->qdcount; i++) {
      if (next[0] > sizeof(s))
        error("Bad DNS request, not enough data\n");
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

protected string ANY;

protected void create()
{
  // Check if IPv6 support is available, and that mapped IPv4 is enabled.
  catch {
    // Note: Attempt to bind on the IPv6 loopback (::1)
    //       rather than on IPv6 any (::), to make sure some
    //       IPv6 support is actually configured. This is needed
    //       since eg Solaris happily binds on :: even
    //       if no IPv6 interfaces are configured.
    //       Try IPv6 any (::) too for paranoia reasons.
    //       For even more paranoia, try sending some data
    //       from :: to the drain services on 127.0.0.1 and ::1.
    //
    // If the tests fail, we regard the IPv6 support as broken,
    // and use just IPv4.
    Stdio.UDP udp = Stdio.UDP();
    if (udp->bind(0, "::1") && udp->bind(0, "::") &&
	(udp->send("127.0.0.1", 9, "/dev/null") == 9) &&
	(udp->send("::1", 9, "/dev/null") == 9)) {
      // Note: The above tests are apparently not sufficient, since
      //       WIN32 happily pretends to send stuff to ::ffff:0:0/96...
      Stdio.UDP udp2 = Stdio.UDP();
      if (udp2->bind(0, "127.0.0.1")) {
	// IPv4 is present.
	array(string) a = udp2->query_address()/" ";
	int port = (int)a[1];
	string key = random_string(16);
	udp2->set_nonblocking();

	// We shouldn't get any lost packets, since we're on the loop-back,
	// but for paranoia reasons we perform a couple of retries.
      retry:
	for (int i = 0; i < 16; i++) {
	  if (!udp->send("127.0.0.1", port, key)) continue;
	  // udp2->wait() throws errors on WIN32.
	  catch(udp2->wait(1));
	  mapping res;
	  while ((res = udp2->read())) {
	    if (res->data == key) {
	      // Mapped IPv4 seems to work.
	      ANY = "::";
	      break retry;
	    }
	  }
	}
	destruct(udp2);
      }
    }
    destruct(udp);
  };
}

//! Base class for @[server], @[tcp_server].
class server_base
{
  inherit protocol;

  array(Stdio.UDP|object) ports = ({ });

  protected string low_send_reply(mapping r, mapping q, mapping m)
  {
    if(!r)
      r = (["rcode":4]);
    r->id = q->id;
    r->qr = 1;
    r->opcode = q->opcode;
    r->rd = q->rd;
    r->qd = r->qd || q->qd;
    string s = mkquery(r);
    return s;
  }

  //! Reply to a query (stub).
  //!
  //! @param query
  //!   Parsed query.
  //!
  //! @param udp_data
  //!   Raw UDP data. If the server operates in TCP mode (@[tcp_server]),
  //!   it will contain an additional tcp_con entry. In that case,
  //!   @expr{udp_data->tcp_con->con@} will contain the TCP connection the
  //!   request was received on as @[Stdio.File] object.
  //!
  //! @param cb
  //!   Callback you can call with the result instead of returning it.
  //!   In that case, return @expr{0@} (zero).
  //!
  //!
  //! Overload this function to implement the proper lookup.
  //!
  //! @note
  //!   To indicate the default failure @[cb] must be called with an
  //!   argument of @expr{0@} (zero), and @expr{0@} (zero) be returned.
  //!
  //! @returns
  //!   Returns @expr{0@} (zero) when the @[cb] callback will be used,
  //!   or a result mapping if not:
  //!   @mapping
  //!     @member int "rcode"
  //!       0 (or omit) for success, otherwise one of the Protocols.DNS.* constants
  //!     @member array(mapping(string:string|int))|void "an"
  //!       Answer section:
  //!       @array
  //!         @elem mapping(string:string|int) entry
  //!           @mapping
  //!             @member string|array(string) "name"
  //!             @member int "type"
  //!             @member int "cl"
  //!           @endmapping
  //!       @endarray
  //!     @member array|void "qd"
  //!       Question section, same format as @[an]; omit to return the original question
  //!     @member array|void "ns"
  //!       Authority section (usually NS records), same format as @[an]
  //!     @member array|void "ar"
  //!       Additional section, same format as @[an]
  //!     @member int "aa"
  //!       Set to 1 to include the Authoritative Answer bit in the response
  //!     @member int "tc"
  //!       Set to 1 to include the TrunCated bit in the response
  //!     @member int "rd"
  //!       Set to 1 to include the Recursion Desired bit in the response
  //!     @member int "ra"
  //!       Set to 1 to include the Recursion Available bit in the response
  //!     @member int "cd"
  //!       Set to 1 to include the Checking Disabled bit in the response
  //!     @member int "ad"
  //!       Set to 1 to include the Authenticated Data bit in the response
  //!   @endmapping
  protected mapping|zero reply_query(mapping query, mapping udp_data,
                                     function(mapping:void) cb)
  {
    // Override this function.
    //
    // Return mapping may contain:
    // aa, ra, {ad, cd,} rcode, an, ns, ar

    return 0;
  }

  //! Handle a query.
  //!
  //! This function calls @[reply_query()],
  //! and dispatches the result to @[send_reply()].
  protected void handle_query(mapping q, mapping m, Stdio.UDP|object udp)
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

  //! Handle a query response (stub).
  //!
  //! Overload this function to handle responses to possible recursive queries.
  protected void handle_response(mapping r, mapping m, Stdio.UDP|object udp)
  {
    // This is a stub intended to simplify servers which allow recursion
  }

  //! Report a failure to decode a DNS request.
  //!
  //! The default implementation writes a backtrace to stderr.  This
  //! method exists so that derived servers can replace it with more
  //! appropriate error handling for their environment.
  protected void report_decode_error(mixed err, mapping m, Stdio.UDP|object udp)
  {
    werror("DNS: Failed to read %s packet.\n%s\n",
	   udp->tcp_connection ? "TCP" : "UDP",
	   describe_backtrace(err));
  }

  //! Respond to a query that cannot be decoded.
  //!
  //! This method exists so that servers can override the default behaviour.
  protected void handle_decode_error(mapping err, mapping m,
				     Stdio.UDP|object udp)
  {
    if(m && m->data && sizeof(m->data)>=2)
      send_reply((["rcode":1]),
                 mkmapping(({"id"}), array_sscanf(m->data, "%2c")), m, udp);
  }

  //! Low-level DNS-data receiver.
  //!
  //! This function receives the raw DNS-data from the @[Stdio.UDP] socket
  //! or TCP connection object @[udp], decodes it, and dispatches the decoded
  //! DNS request to @[handle_query()] and @[handle_response()].
  protected void rec_data(mapping m, Stdio.UDP|object udp)
  {
    mixed err;
    mapping q;
    if (err = catch {
      q=decode_res(m->data);
    }) {
      report_decode_error(err, m, udp);
      handle_decode_error(err, m, udp);
    }
    else if(q->qr)
      handle_response(q, m, udp);
    else
      handle_query(q, m, udp);
  }

  protected void send_reply(mapping r, mapping q, mapping m,
			    Stdio.UDP|object con);

  protected void _destruct()
  {
    if(sizeof(ports))
    {
      foreach(ports;; object port)
        destruct(port);
    }
  }
}

//! Base class for implementing a Domain Name Service (DNS) server operating
//! over UDP.
//!
//! This class is typically used by inheriting it,
//! and overloading @[reply_query()] and @[handle_response()].
//!
//! @seealso
//!   @[dual_server]
class server
{
  //!
  inherit server_base;

  //inherit Stdio.UDP : udp;

  protected void send_reply(mapping r, mapping q, mapping m, Stdio.UDP udp) {
    udp->send(m->ip, m->port, low_send_reply(r, q, m));
  }

  //! @decl void create()
  //! @decl void create(int port)
  //! @decl void create(string ip)
  //! @decl void create(string ip, int port)
  //! @decl void create(string ip, int port, string|int ... more)
  //!
  //! Open one or more new DNS server ports.
  //!
  //! @param ip
  //!   The IP to bind to. Defaults to @expr{"::"@} or @expr{0@} (ie ANY)
  //!   depending on whether IPv6 support is present or not.
  //!
  //! @param port
  //!   The port number to bind to. Defaults to @expr{53@}.
  //!
  //! @param more
  //!   Optional further DNS server ports to open.
  //!   Must be a set of @[ip], @[port] argument pairs.
  protected void create(int|string|void arg1, string|int ... args)
  {
    if(!arg1 && !sizeof(args))
      arg1 = 53;
    if(!sizeof(args))
    {
      if(stringp(arg1))
       args = ({ arg1, 53 });
      else
       args = ({ ANY, arg1 });
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
	if (!safe_bind(udp, args[i+1], args[i]))
         error("DNS: failed to bind host:port %s:%d.\n", args[i],args[i+1]);
      } else {
	if(!safe_bind(udp, args[i+1]))
         error("DNS: failed to bind port %d.\n", args[i+1]);
      }
      udp->set_read_callback(rec_data, udp);
      // port objects are stored for destruction when the server object is destroyed.
      ports += ({udp});
    }

  }
}


//! Base class for implementing a Domain Name Service (DNS) server operating
//! over TCP.
//!
//! This class is typically used by inheriting it,
//! and overloading @[reply_query()] and @[handle_response()].
class tcp_server
{
  inherit server_base;

  mapping(Connection:int(1..1)) connections = ([ ]);

  protected class Connection {
    constant tcp_connection = 1;

    protected int(0..1) write_ready;
    protected string read_buffer = "", out_buffer = "";
    protected array c_id;
    Stdio.File con;

    protected void create(Stdio.File con) {
      this::con = con;
      con->set_nonblocking(rcb, wcb, ccb);
      c_id = call_out(destruct, 120, this);
    }

    protected void ccb(mixed id) {
      destruct(con);
      m_delete(connections, this);
    }

    protected void wcb(mixed id) {
      if (sizeof(out_buffer)) {
	int written = con->write(out_buffer);
	out_buffer = out_buffer[written..];
      } else
	write_ready = 1;
    }

    protected void rcb(mixed id, string data) {
      int len;

      read_buffer += data;
      if (sscanf(read_buffer, "%2c", len)) {
	if (sizeof(read_buffer) > len - 2) {
	  string data = read_buffer[2..len+1];
	  string ip, port;
	  mapping m;

	  read_buffer = read_buffer[len+2..];

	  remove_call_out(c_id);
	  c_id = call_out(destruct, 120, this);

	  [ip, port] = con->query_address() / " ";
	  m = ([ "data" : data,
	         "ip" : ip,
		 "port" : (int)port,
		 "tcp_con" : this ]);


	  rec_data(m, this);
	}
      }
    }

    void send(string s) {
      if (sizeof(s) > 65535)
	error("DNS: Cannot send packets > 65535 bytes (%d here).\n", sizeof(s));
      out_buffer += sprintf("%2H", s);

      if (write_ready) {
	int written = con->write(out_buffer);
	out_buffer = out_buffer[written..];
	write_ready = 0;
      }

      remove_call_out(c_id);
      c_id = call_out(destruct, 120, this);
    }

    protected void _destruct() {
      if (con) con->close();
      destruct(con);
      m_delete(connections, this);
    }
  }

  protected int accept(Stdio.Port port) {
    connections[Connection(port->accept())] = 1;
  }

  protected void send_reply(mapping r, mapping q, mapping m, Connection con) {
    con->send(low_send_reply(r, q, m));
  }

  //! @decl void create()
  //! @decl void create(int port)
  //! @decl void create(string ip)
  //! @decl void create(string ip, int port)
  //! @decl void create(string ip, int port, string|int ... more)
  //!
  //! Open one or more new DNS server ports.
  //!
  //! @param ip
  //!   The IP to bind to. Defaults to @expr{"::"@} or @expr{0@} (ie ANY)
  //!   depending on whether IPv6 support is present or not.
  //!
  //! @param port
  //!   The port number to bind to. Defaults to @expr{53@}.
  //!
  //! @param more
  //!   Optional further DNS server ports to open.
  //!   Must be a set of @[ip], @[port] argument pairs.
  protected void create(int|string|void arg1, string|int ... args)
  {
    if(!arg1 && !sizeof(args))
      arg1 = 53;
    if(!sizeof(args))
    {
      if(stringp(arg1))
       args = ({ arg1, 53 });
      else
       args = ({ ANY, arg1 });
    }
    else
      args = ({ arg1 }) + args;
    if(sizeof(args)&1)
      error("DNS: if you specify more than one argument, the number of "
           "arguments needs to be even (server(ip1, port1, ip2, port2, "
           "...)).\n");
    for(int i;i<sizeof(args);i+=2) {
      Stdio.Port port;

      if(args[i]) {
	port = Stdio.Port(args[i+1], accept, args[i]);
      } else {
	port = Stdio.Port(args[i+1], accept);
      }

      port->set_id(port);
      // Port objects are stored for destruction when the server
      // object is destroyed.
      ports += ({ port });
    }
  }

  protected void _destruct()
  {
    foreach (connections; Connection con;) {
      destruct(con);
    }

    ::_destruct();
  }
}

//! This is both a @[server] and @[tcp_server].
class dual_server {
  inherit server : UDP;
  inherit tcp_server : TCP;

  protected void send_reply(mapping r, mapping q, mapping m,
			    Connection|Stdio.UDP con) {
    string rpl = low_send_reply(r, q, m);

    if (!con->tcp_connection) {
      if (sizeof(rpl) > 512) {
	rpl = sprintf("%s%8c", rpl[..3], 0); // Truncate after header and
					     // send empty response
					     // ("dnscache strategy")
	rpl[2] |= 2; // Set TC bit
      }
      con->send(m->ip, m->port, rpl);
    } else
      con->send(rpl);
  }

  protected void create(int|string|void arg1, string|int ... args)
  {
    ::create(arg1, @args);
  }

  protected void _destruct()
  {
    ::_destruct();
  }
}


#define RETRIES 12
#define RETRY_DELAY 5

//! Synchronous DNS client.
class client
{
  inherit protocol;

#ifdef __NT__
  array(string) get_tcpip_param(string val, void|string fallbackvalue)
  {
    array(string) res = ({});
    foreach(({
      "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
      "SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters",
      "SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP"
    }),string key)
    {
      catch {
	res += ({ System.RegGetValue(HKEY_LOCAL_MACHINE, key, val) });
      };
    }

#if constant(System.RegGetKeyNames)
    /* Catch if System.RegGetKeyNames() doesn't find the directory. */
    catch {
      foreach(System.RegGetKeyNames(HKEY_LOCAL_MACHINE,
			     "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\"
			     "Parameters\\Interfaces"), string key)
      {
	catch {
	  res += ({ System.RegGetValue(HKEY_LOCAL_MACHINE,
				"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\"
				"Parameters\\Interfaces\\" + key, val) });
	};
      }
      foreach(System.RegGetKeyNames(HKEY_LOCAL_MACHINE,
			     "SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\"
			     "Parameters\\Interfaces"), string key)
      {
	catch {
	  res += ({ System.RegGetValue(HKEY_LOCAL_MACHINE,
                                "SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\"
				"Parameters\\Interfaces\\" + key, val) });
	};
      }
    };
#endif
    res -= ({ UNDEFINED });
    return sizeof(res) ? res : ({ fallbackvalue });
  }

#endif /* !__NT__ */

  protected private mapping(string:array(string)) etc_hosts;

  private Regexp is_ip_regex
   = Regexp("^([0-9a-fA-F:]+:[0-9a-fA-F:]*|[0-9]+(\\.[0-9]+)+)$");

  protected private int is_ip(string(8bit) ip)
  {
    return is_ip_regex->match(ip);
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

  //! Return /etc/hosts records
  array(string)|zero match_etc_hosts(string host)
  {
    if (!etc_hosts) {
      etc_hosts = ([ "localhost":({ "127.0.0.1" }) ,
                     "127.0.0.1":({ "localhost" }) ]);

      string raw = read_etc_file("hosts");

      if (raw && sizeof(raw)) {
	foreach(raw/"\n"-({""}), string line) {
	  // Handle comments, and split the line on white-space
	  line = lower_case(replace((line/"#")[0], "\t", " "));
	  array arr = (line/" ") - ({ "" });

	  if (sizeof(arr) > 1) {
	    if (is_ip(arr[0])) {
              // Store reverse lookup
              if (sizeof(arr) > 1)
                etc_hosts[arr[0]] += arr[1..];
              // Store forward lookups
	      foreach (arr[1..], string name)
                etc_hosts[name] += ({ arr[0] });
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

  // FIXME: Read hosts entry in /etc/nswitch.conf?

  //! @decl void create()
  //! @decl void create(void|string|array server, void|int|array domain)

  array(string) nameservers = ({});
  array domains = ({});
  protected void create(void|string|array(string) server,
			void|int|array(string) domain)
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
		array(string)|zero hostip = match_etc_hosts(host);
		if (!hostip) {
		  werror("Protocols.DNS.client(): "
			 "Can't resolv nameserver \"%s\"\n", host);
		  break;
		}
                nameservers += hostip;
	      } else if (sizeof(rest)) {
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
      domains = map(domains, lambda(string d) {
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

  //! Perform a synchronous DNS query.
  //!
  //! @param s
  //!   Result of @[Protocols.DNS.protocol.mkquery]
  //! @returns
  //!   mapping containing query result or 0 on failure/timeout
  //!
  //! @example
  //!   @code
  //!     // Perform a hostname lookup, results stored in r->an
  //!     object d=Protocols.DNS.client();
  //!     mapping r=d->do_sync_query(d->mkquery("pike.lysator.liu.se", C_IN, T_A));
  //!   @endcode
  mapping|zero do_sync_query(string s)
  {
    int i;
    object udp = Stdio.UDP();
    // Attempt to randomize the source port.
    for (i = 0; i < RETRIES; i++) {
      if (!safe_bind(udp, 1024 + random(65536-1024), ANY)) continue;
    }
    if (i >= RETRIES) safe_bind(udp, 0, ANY) || udp->bind(0);
#if 0
    werror("Protocols.DNS.client()->do_sync_query(%O)\n"
	   "UDP Address: %s\n"
	   "%s\n", s, udp->query_address(), describe_backtrace(backtrace()));
#endif /* 0 */
    mapping m;
    for (i=0; i < RETRIES; i++) {
      udp->send(nameservers[i % sizeof(nameservers)], 53, s);

      // upd->wait() can throw an error sometimes.
      //
      // One common case is on WIN32 where select complains
      // about it not being a socket.
      catch {
	udp->wait(RETRY_DELAY);
      };
      // Make sure that udp->read() doesn't block.
      udp->set_nonblocking();
      // udp->read() can throw an error on connection refused.
      catch {
	m = udp->read();
	if (m && (m->port == 53) &&
	    (m->data[0..1] == s[0..1]) &&
	    has_value(nameservers, m->ip)) {
	  // Success.
	  return decode_res(m->data);
	}
      };
      // Restore blocking state for udp->send() on retry.
      udp->set_blocking();
    }
    // Failure.
    return 0;
  }

  protected mapping low_gethostbyname(string s, int type)
  {
    mapping m;
    if(sizeof(domains) && sizeof(s) && s[-1] != '.' && sizeof(s/".") < 3) {
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
    mapping aaaa_records = low_gethostbyname(s, T_AAAA);

#if 0
    werror("a_records: %O\n"
	   "aaaa_records: %O\n",
	   a_records, aaaa_records);
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

  //!	Queries the service record (@rfc{2782@}) from the default or
  //!	given DNS server. The result is an array of arrays with the
  //!	following six elements for each record. The array is sorted
  //!	according to the priority of each record.
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
  string|zero get_primary_mx(string host)
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

  //!
  array(string)|zero get_mx(string host)
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

  //!
  class Request(string domain, string req,
		function(string,mapping|zero,__unknown__...:void)|zero callback,
		array(mixed) args)
  {
    int retries;
    int timestamp = time();

    //! Cancel the current request.
    void cancel()
    {
      remove(this);
    }
    mixed retry_co;
  };

  mapping requests=([]);

  protected void remove(object(Request) r)
  {
    if(!r) return;
    sscanf(r->req,"%2c",int id);
    m_delete(requests,id);
    if (r->retry_co) remove_call_out(r->retry_co);
    r->retry_co = UNDEFINED;
    r->callback && r->callback(r->domain,0,@r->args);
    destruct(r);
  }
}

#define REMOVE_DELAY 120
#define GIVE_UP_DELAY (RETRIES * RETRY_DELAY + REMOVE_DELAY)*2

private mapping dnstypetonum = ([
  "A":    Protocols.DNS.T_A,
  "MX":   Protocols.DNS.T_MX,
  "MXIP": Protocols.DNS.T_MX,
  "TXT":  Protocols.DNS.T_TXT,
  "AAAA": Protocols.DNS.T_AAAA,
  "PTR":  Protocols.DNS.T_PTR,
]);

// FIXME: Randomized source port!
//! Asynchronous DNS client.
class async_client
{
  inherit client;
  inherit Stdio.UDP : udp;
  async_client next_client;

  void retry(object(Request) r, void|int nsno)
  {
    if(!r) return;
    if (nsno >= sizeof(nameservers)) {
      if(r->retries++ > RETRIES)
      {
	r->retry_co = call_out(remove, REMOVE_DELAY, r);
	return;
      } else {
	nsno = 0;
      }
    }

    r->retry_co = call_out(retry, RETRY_DELAY, r, nsno+1);
    udp::send(nameservers[nsno], 53, r->req);
  }

  //! Enqueue a new raw DNS request.
  //!
  //! @returns
  //!   Returns a @[Request] object.
  //!
  //! @note
  //!   Pike versions prior to 8.0 did not return the @[Request] object.
  Request do_query(string domain, int cl, int type,
		   function(string,mapping,__unknown__...:void) callback,
		   mixed ... args)
  {
    if (!callback) return UNDEFINED;
    for(int e=next_client ? 100 : 256;e>=0;e--)
    {
      int lid = random(65536);
      if(!catch { requests[lid]++; })
      {
	string req=low_mkquery(lid,domain,cl,type);

        object r = Request(domain, req, callback, args);
	r->retry_co = call_out(retry, RETRY_DELAY, r, 1);
	requests[lid] = r;
	udp::send(nameservers[0], 53, req);
	return r;
      }
    }

    /* We failed miserably to find a request id to use,
     * so we create a second UDP port to be able to have more
     * requests 'in the air'. /Hubbe
     */
    if(!next_client)
      next_client=this_program(nameservers,domains);

    return next_client->do_query(domain, cl, type, callback, @args);
  }

  protected private void rec_data(mapping m)
  {
    mixed err;
    object r;
    mapping res;
    if (err = catch {
      if(m->port != 53 || !has_value(nameservers, m->ip)) return;
      sscanf(m->data,"%2c",int id);
      r = requests[id];
      if(!r) {
	// Invalid request id. Spoofed answer?
	// FIXME: Consider black- or greylisting the answer.
	return;
      }
      m_delete(requests,id);
      res = decode_res(m->data);
    }) {
      werror("DNS: Failed to read UDP packet. Connection refused?\n%s\n",
	     describe_backtrace(err));
    }
    // NB: The callback may have gone away during our processing.
    //     Don't complain if that is the case.
    if (r->callback && (err = catch {
	r->callback(r->domain, res, @r->args);
      })) {
      werror("DNS: Callback failed:\n"
	     "%s\n",
	     describe_backtrace(err));
    }
    destruct(r);
  }

  private void collect_return(string domain, mapping res,
                              function(array|zero, __unknown__ ...:void)|zero callback,
                              mixed ... restargs) {
    array an = res->an;
    switch (res->rcode) {
      case Protocols.DNS.SERVFAIL:
      case Protocols.DNS.NOTIMP:
      case Protocols.DNS.REFUSED:
      case Protocols.DNS.YXDOMAIN:
      case Protocols.DNS.YXRRSET:
      case Protocols.DNS.NXRRSET:
      case Protocols.DNS.NOTZONE:
        if (!sizeof(an)) {
          if (callback)				// Callback might have vanished
            callback(0, @restargs);
          return;
        }
    }
    sort(an->preference, an);
    if (callback) {				// Callback might have vanished
      callback(an->aaaa + an->a + an->mx + an->txt + an->ptr - ({ 0 }),
               @restargs);
    }
  }

  private array prepslots(int numslots,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    array slots = allocate(numslots + 1);
    slots[0] = numslots;
    return ({ slots, callback }) + restargs;
  }

   // FIXME This actually is a hand-rolled Promise/Future construction.
  //        Would it be more elegant (but more overhead) to use a real Promise?
  private void collectslots(array|zero results, int slot, array slots,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    // All queries are racing; collect all results in order
    slots[slot] = results;
    if (!--slots[0]) {				// Last slot filled
      array|zero allresults;
      for (slot = 0; ++slot < sizeof(slots);) {
        if (results = slots[slot]) {
          if (allresults)
            results -= allresults;		// Eliminate duplicates
          else
            allresults = ({});
          allresults += results;
        }
      }
      if (callback)				// Callback might have vanished
        callback(allresults, @restargs);	// Report back
    }
  }

  private void multicallback(array|zero results,
      string type, string domain, array(string) multiq,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    // If we have results, return early
    if (results && sizeof(results)) {
      if (callback)			// Callback might have vanished
        callback(results, @restargs);
    } else {
      // No results, so try the next domain extension
      domain += "." + multiq[0];
      if (sizeof(multiq) == 1)		// Last try, short to final callback
        low_generic_query(2, type, domain, callback, @restargs);
      else {
        low_generic_query(2, type, domain,	// Shift remaining domain list
          multicallback, type, domain, multiq[1..], callback, @restargs);
      }
    }
  }

  private void mxfallback(array|zero results, string domain,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    // We just want the names, not the IP addresses
    if (results && sizeof(results))
      results = ({ domain });
    if (callback)			// Callback might have vanished
      callback(results, @restargs);
  }

  private void collectmx(array|zero results, string domain,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    if (results && sizeof(results)) {
      if (callback)			// Callback might have vanished
        callback(results, @restargs);	// Got actual MX records
    } else
      // Promote any A records to MX 0 records
      generic_query("AAAA", domain, mxfallback, domain, callback, @restargs);
  }

  private void collectmxip(array|zero results, string domain,
      function(array(string)|zero, __unknown__ ...:void) callback,
      mixed ... restargs) {
    if (results && sizeof(results)) {
      restargs = prepslots(sizeof(results), callback, @restargs);
      foreach (results; int i; string mx)	// MX -> IP
        low_generic_query(1, "AAAA", mx, collectslots, i + 1, @restargs);
    } else
      // Promote A any records to MX 0 records
      generic_query("AAAA", domain, callback, @restargs);
  }

  //! @param restrictsearch
  //!   @int
  //!     @value 0
  //!       Try @expr{/etc/hosts@} first, then try all configured
  //!       domain-postfixes when querying the DNS servers (default).
  //!     @value 1
  //!       Try @expr{/etc/hosts@} first, then try an unaltered query
  //!       on the DNS servers.
  //!     @value 2
  //!       Just try an unaltered query on the DNS servers.
  //!   @endint
  //!
  private void low_generic_query(int restrictsearch, string type,
       string domain, function(array(string)|zero, __unknown__ ...:void) callback,
       mixed ... restargs) {
    int itype = dnstypetonum[type];
    if (!itype) {
      callback(0, @restargs);
      return;
    }
    // Check /etc/hosts only in particular circumstances
    if (restrictsearch <= 1) {
      switch (type) {
        case "PTR":
        case "AAAA":
        case "A":
          array(string)|zero etchosts = match_etc_hosts(domain);
          if (etchosts) {
            callback(etchosts, @restargs);
            return;
          }
      }
    }
    switch (type) {
      case "MXIP":
      case "MX":
        restargs = ({ domain, callback }) + restargs;
        callback = collectmx;
        break;
      case "PTR":
        domain = arpa_from_ip(domain);
        break;
      case "AAAA":
      case "A":
        if (!restrictsearch) {
          if (domain[-1] != '.' && sizeof(domains)) {
            array(string) multiq;
            if (has_value(domain, "."))
              multiq = domains;
            else {
              multiq += domains[1..] + ({ "" });
              domain = domain + "." + domains[0];
            }
            restargs = ({ domain, type, multiq, callback }) + restargs;
            callback = multicallback;
          }
        }
        break;
    }
    switch (type) {
      case "MXIP":
        callback = collectmxip;
        break;
      case "AAAA":
        restargs = prepslots(2, callback, @restargs);
        do_query(domain, Protocols.DNS.C_IN, dnstypetonum["A"],
                 collect_return, collectslots, 2, @restargs);
        restargs = ({ 1 }) + restargs;
        callback = collectslots;
        break;
    }
    do_query(domain, Protocols.DNS.C_IN, itype,
             collect_return, callback, @restargs);
  }

  //! Asynchronous DNS query with multiple results and a distinction
  //! between failure and empty results.
  //!
  //! @param type
  //!  DNS query type. Currenlty supported:
  //!   @string
  //!     @value "A"
  //!       Return just IPv4 records.
  //!     @value "AAAA"
  //!       Return both IPv6 and IPv4 records.
  //!     @value "PTR"
  //!       Reverse lookup for IP addresses, it expects normal
  //!       IP addresses for @expr{domain@}.
  //!     @value "TXT"
  //!       Return TXT records.
  //!     @value "MX"
  //!       Return MX records sorted by @expr{preference@}, lowest
  //!       numbers first.
  //!     @value "MXIP"
  //!       Like querying for @expr{MX@}, except it returns IP
  //!       addresses instead of the MX records themselves.
  //!   @endstring
  //!
  //! @param domain
  //!  The domain name we are querying.  Add a trailing dot to prohibit
  //!  domain-postfix searching.
  //!
  //! @param callback
  //!  The callback function that receives the result of the DNS query.
  //!  It should be declared as follows:
  //!     @expr{void callback(array(string)|zero results, mixed ... restargs);@}
  //!  If the request fails it will return @expr{zero@} for @expr{results@}.
  //!
  //! @param restargs
  //!  They are passed unaltered to the @expr{callback@} function.
  //!
  //! @note
  //!   There is a notable difference between @expr{results@} equal
  //!   to @expr{zero@} (= request failed and can be retried) and
  //!   @expr{({})@} (= request definitively answered the record
  //!   does not exist; retries are pointless).
  //!
  //! @note
  //!   This method uses the exact same heuristics as the standard DNS
  //!   resolver library (regarding the use of /etc/hosts, and when to
  //!   perform a domain-postfix search, and when not to (i.e. trailing
  //!   dot)).
  //!
  //! @note
  //!   All queries sort automatically by preference (lowest numbers first).
  void generic_query(string type, string domain,
       function(array(string)|zero, __unknown__ ...:void) callback,
       mixed ... restargs) {
    low_generic_query(0, upper_case(type), domain, callback, @restargs);
  }

  private void single_result(array|zero results,
      string domain, function(string, string, __unknown__ ...:void) callback,
      mixed ... restargs) {
    if (callback)
      callback(domain, results && sizeof(results) ? results[0] : "",
               @restargs);
  }

  private void multiple_results(array|zero results,
      string domain, function(string, array, __unknown__ ...:void) callback,
      mixed ... restargs) {
    if (callback)
      callback(domain, results, @restargs);
  }

  protected private Request generic_get(string d,
					mapping answer,
					int multi,
					int all,
					int type,
					string field,
					string domain,
					function callback,
					mixed ... args)
  {
    if (!callback) return UNDEFINED;
    if(!answer || !answer->an || !sizeof(answer->an))
    {
      if(multi == -1 || multi >= sizeof(domains)) {
	// Either a request without multi (ip, or FQDN) or we have tried all
	// domains.
	callback(domain,0,@args);
      } else {
	// Multiple domain request. Try the next one...
	return do_query(domain+"."+domains[multi], C_IN, type,
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
	    return UNDEFINED;
	  }
	callback(domain,0,@args);
      }
    }
    return UNDEFINED;
  }

  //! Looks up the IPv4 address for a host, and when done calls the
  //! function callback with the host name and IP number as arguments.
  //!
  //! @returns
  //!   Returns a @[Request] object where progress can be observed
  //!   from the retries variable and the request can be cancelled
  //!   using the @[cancel] method.
  //!
  //! @seealso
  //!   @[host_to_ips]
  Request host_to_ip(string host, function(string,string,__unknown__...:void) callback, mixed ... args)
  {
    generic_query("A", host, single_result, host, callback, @args);
  }

  //! Looks up the IPv4 address for a host. Returns a
  //! @[Concurrent.Future] object that resolves into the IP number as
  //! a string, or 0 if it is missing.
  //!
  //! @seealso
  //!   @[host_to_ips]
  variant Concurrent.Future host_to_ip(string host) {
    Concurrent.Promise p = Concurrent.Promise();
    void success(string host, string ip) {
      p->success(ip);
    }
    host_to_ip(host, success);
    return p->future();
  }

  //! Looks up the IP number(s) for a host, and when done calls the
  //! function callback with the host name and array of IP addresses
  //! as arguments. If IPv6 and IPv4 addresses are both available,
  //! IPv6 addresses will be earlier in the array.
  //!
  //! @returns
  //!   Returns a @[Request] object where progress can be observed
  //!   from the retries variable and the request can be cancelled
  //!   using the @[cancel] method.
  Request host_to_ips(string host,
		      function(string, array, __unknown__...:void) callback,
		      mixed ... args)
  {
    generic_query("AAAA", host, multiple_results, host, callback, @args);
  }

  //! Looks up the IP number for a host. Returns a
  //! @[Concurrent.Future] object that resolves into an array of
  //! IP addresses as strings, or an empty array if it is missing.
  variant Concurrent.Future host_to_ips(string host) {
    Concurrent.Promise p = Concurrent.Promise();
    host_to_ips(host) {p->success(__ARGS__[1]);};
    return p->future();
  }

  //! Looks up the host name for an IP number, and when done calls the
  //! function callback with the IP number adn host name as arguments.
  //!
  //! @returns
  //!   Returns a @[Request] object where progress can be observed
  //!   from the retries variable and the request can be cancelled
  //!   using the @[cancel] method.
  Request ip_to_host(string ip, function(string,string,__unknown__...:void) callback, mixed ... args)
  {
    generic_query("PTR", ip, single_result, ip, callback, @args);
  }

  //! Looks up the host name for an IP number. Returns a
  //! @[Concurrent.Future] object that resolves into the host name, or
  //! 0 if it is missing.
  variant Concurrent.Future ip_to_host(string ip) {
    Concurrent.Promise p = Concurrent.Promise();
    void success(string ip, string host) {
      p->success(host);
    }
    ip_to_host(ip, success);
    return p->future();
  }

  //! Looks up the mx pointers for a host, and when done calls the
  //! function callback with the results as an array of mappings.
  //!
  //! @returns
  //!   Returns a @[Request] object where progress can be observed
  //!   from the retries variable and the request can be cancelled
  //!   using the @[cancel] method.
  Request get_mx_all(string host, function(string,array(mapping(string:string|int)),__unknown__...:void) callback, mixed ... args)
  {
    if(sizeof(domains) && host[-1] != '.' && sizeof(host/".") < 3) {
      return do_query(host, C_IN, T_MX,
		      generic_get, 0, 1, T_MX, "mx", host, callback, @args);
    } else {
      return do_query(host, C_IN, T_MX,
		      generic_get, -1, 1, T_MX, "mx", host, callback, @args);
    }
  }

  //! Looks up the mx pointers for a host. Returns a
  //! @[Concurrent.Future] object that resolves into an array of
  //! mappings.
  variant Concurrent.Future get_mx_all(string host) {
    Concurrent.Promise p = Concurrent.Promise();
    void success(string host, array(mapping(string:string|int)) results) {
      p->success(results);
    }
    get_mx_all(host, success);
    return p->future();
  }

  //! Looks up the mx pointers for a host, and when done calls the
  //! function callback with the results as an array of strings. These
  //! can be host names, IP numbers, or a mix.
  //!
  //! @returns
  //!   Returns a @[Request] object where progress can be observed
  //!   from the retries variable and the request can be cancelled
  //!   using the @[cancel] method.
  Request get_mx(string host, function(array(string),__unknown__...:void) callback, mixed ... args)
  {
    return get_mx_all(host,
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

  //! Looks up the mx pointers for a host. Returns a
  //! @[Concurrent.Future] object that resolves into an array of
  //! strings.
  variant Concurrent.Future get_mx(string host) {
    Concurrent.Promise p = Concurrent.Promise();
    void success(array(string) results) {
      p->success(results);
    }
    get_mx(host, success);
    return p->future();
  }

  //! Close the client.
  //!
  //! @note
  //!   All active requests are aborted.
  void close()
  {
    foreach(requests; ; Request r) {
      remove(r);
    }
    udp::close();
    udp::set_read_callback(0);
  }

  //!
  protected void create(void|string|array(string) server,
			void|string|array(string) domain)
  {
    int i;
    // Attempt to randomize the source port.
    for (i = 0; i < RETRIES; i++) {
      if (safe_bind(udp::this, 1024 + random(65536-1024), ANY)) break;
    }
    if((i >= RETRIES) &&
       !safe_bind(udp::this, 0, ANY) &&
       !safe_bind(udp::this, 0))
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


// FIXME: Reuse sockets where possible?
//! Synchronous DNS client using TCP
//! Can handle larger responses than @[client] can.
class tcp_client
{
  inherit client;

  //! Perform a synchronous DNS query.
  //!
  //! @param s
  //!   Result of @[Protocols.DNS.protocol.mkquery]
  //! @returns
  //!  mapping containing query result or 0 on failure/timeout
  //!
  //! @example
  //!   @code
  //!     // Perform a hostname lookup, results stored in r->an
  //!     object d=Protocols.DNS.tcp_client();
  //!     mapping r=d->do_sync_query(d->mkquery("pike.lysator.liu.se", C_IN, T_A));
  //!   @endcode
  mapping|zero do_sync_query(string s)
  {
    for (int i=0; i < RETRIES; i++) {
      object tcp = Stdio.File();
      if (tcp->connect(nameservers[i % sizeof(nameservers)], 53))
      {
        tcp->write("%2H",s);
	sscanf(tcp->read(2),"%2c",int len);
	return decode_res(tcp->read(len));
      }
    }
    // Failure.
    return 0;
  }
}

// FIXME: Reuse sockets? Acknowledge RETRIES?
//! Asynchronous DNS client using TCP
class async_tcp_client
{
  inherit async_client;

  //!
  class Request
  {
    inherit ::this_program;

    protected Stdio.File sock;
    protected string writebuf="",readbuf="";

    protected void create(string domain, string req,
			  function(string,mapping,__unknown__...:void) callback,
			  array(mixed) args)
    {
      ::create(domain, req, callback, args);
      sock=Stdio.File();
      sock->async_connect(nameservers[0], 53, connectedcb);
    }

    protected void close()
    {
      sock && sock->close();
      sock = UNDEFINED;
    }

    protected void connectedcb(int ok)
    {
      if (!callback) return;
      if (!ok) {callback(domain, 0, @args); return;}
      sock->set_nonblocking(readcb, writecb, closecb);
      writebuf=sprintf("%2H",req);
      writecb();
    }

    protected void readcb(mixed id,string data)
    {
      readbuf+=data;
      if (sscanf(readbuf,"%2H",string ret))
      {
        if (callback) callback(domain, decode_res(ret), @args);
        callback=0;
        close();
      }
    }

    protected void writecb()
    {
      if (writebuf!="") writebuf=writebuf[sock->write(writebuf)..];
    }

    protected void closecb()
    {
      cancel();
    }

    void cancel()
    {
      close();
      ::cancel();
    }
  }

  //!
  Request do_query(string domain, int cl, int type,
		   function(string,mapping,__unknown__...:void) callback,
		   mixed ... args)
  {
    string req=low_mkquery(random(65536),domain,cl,type);
    return Request(domain, req, callback, args);
  }
}

//! Both a @[client] and a @[tcp_client].
class dual_client
{
  inherit client : UDP;
  inherit tcp_client : TCP;

  //!
  mapping do_sync_query(string s)
  {
    mapping ret = UDP::do_sync_query(s);
    if (!ret->tc) return ret;
    return TCP::do_sync_query(s);
  }

  protected void create(mixed ... args) {::create(@args);}
}

//! Both an @[async_client] and an @[async_tcp_client].
class async_dual_client
{
  inherit async_client : UDP;
  inherit async_tcp_client : TCP;

  void check_truncation(string domain, mapping result, int cl, int type,
			function(string,mapping,__unknown__...:void) callback,
			mixed ... args)
  {
    if (!result || !result->tc) callback(domain,result,@args);
    else TCP::do_query(domain,cl,type,callback,@args);
  }

  //!
  Request do_query(string domain, int cl, int type,
		   function(string,mapping,__unknown__...:void) callback,
		   mixed ... args)
  {
    return UDP::do_query(domain,cl,type,check_truncation,
			 cl,type,callback,@args);
  }

  protected void create(mixed ... args) {::create(@args);}
}


async_client global_async_client;

#define GAC(X)								\
async_client.Request async_##X( string host, function callback, mixed ... args ) 	\
{									\
  if( !global_async_client )						\
    global_async_client = async_client();				\
  return global_async_client->X(host,callback,@args);			\
}                                                                       \
variant Concurrent.Future async_##X( string host ) {                    \
  if( !global_async_client )						\
    global_async_client = async_client();				\
  return global_async_client->X(host);                                  \
}

//! @ignore
GAC(ip_to_host);
//! @endignore
//! @decl client.Request async_ip_to_host(string ip, function cb, mixed ... cba)
//! @decl Concurrent.Future async_ip_to_host(string ip)
//! Calls ip_to_host in a global async_client created on demand.
//! @seealso
//!   @[async_client.ip_to_host()]

//! @ignore
GAC(host_to_ip);
//! @endignore
//! @decl client.Request async_host_to_ip(string host, function cb, mixed ... cba)
//! @decl Concurrent.Future async_host_to_ip(string host)
//! Calls host_to_ip in a global async_client created on demand.
//! @seealso
//!   @[async_client.host_to_ip()]

//! @ignore
GAC(host_to_ips);
//! @endignore
//! @decl client.Request async_host_to_ips(string host, function cb, mixed ... cba)
//! @decl Concurrent.Future async_host_to_ips(string host)
//! Calls host_to_ips in a global async_client created on demand.
//! @seealso
//!   @[async_client.host_to_ips()]

//! @ignore
GAC(get_mx_all);
//! @endignore
//! @decl client.Request async_get_mx_all(string host, function cb, mixed ... cba)
//! @decl Concurrent.Future async_get_mx_all(string host)
//! Calls get_mx_all in a global async_client created on demand.
//! @seealso
//!   @[async_client.get_mx_all()]

//! @ignore
GAC(get_mx);
//! @endignore
//! @decl client.Request async_get_mx(string host, function cb, mixed ... cba)
//! @decl Concurrent.Future async_get_mx(string host)
//! Calls get_mx in a global async_client created on demand.
//! @seealso
//!   @[async_client.get_mx()]


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
