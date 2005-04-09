//  $Id: DNS_SD.pmod,v 1.1 2005/04/09 21:07:21 jonasw Exp $
//  Interface to DNS Service Discovery. Written by Jonas Walldén.


#pike __REAL_VERSION__

#if constant(_Protocols_DNS_SD.Service)


//!  This class provides an interface to DNS Service Discovery. The
//!  functionality of DNS-SD is described at <http://www.dns-sd.org/>.
//!
//!  Using the Proctocols.DNS_SD.Service class a Pike program can
//!  announce services, for example a web site or a database server, to
//!  computers on the local network.
//!
//!  When registering a service you need to provide the service name.
//!  service type, domain and port number. You can also optionally
//!  specify a TXT record. The contents of the TXT record varies between
//!  different services; for example, a web server can announce a path
//!  to a web page, and a printer spooler is able to list printer
//!  features such as color support or two-sided printing.
//!
//!  The service is registered on the network for as long as the instance
//!  of the Service class is valid.
class Service {
  inherit _Protocols_DNS_SD.Service;

  private static string clip_utf8_str(string s, int maxlen)
  {
    //  Clip before UTF-8 encoding to limit loop to a few iterations at most
    s = s[..maxlen - 1];
    string s_utf8 = string_to_utf8(s);
    while (strlen(s_utf8) > maxlen) {
      //  Shorten one (possibly wide) character
      s = s[..strlen(s) - 2];
      s_utf8 = string_to_utf8(s);
    }
    return s_utf8;
  }


  private static string get_flat_txt_record(void|string|array(string) txt)
  {
    string txt_flat;
    
    if (txt && stringp(txt))
      txt = ({ txt });
    if (arrayp(txt) && sizeof(txt)) {
      txt_flat = "";
      foreach(txt, string entry) {
	entry = clip_utf8_str(entry, 255);
	txt_flat += sprintf("%c%s", (int) strlen(entry), entry);
      }
    }
    return txt_flat;
  }
  
  //!  Updates the TXT record for the service.
  //!
  //!  @param txt
  //!    A TXT record with service-specific information. It can be given as a
  //!    plain string or an array of property assignment strings. To remove
  //!    an existing TXT record you give an empty string as the argument.
  void update_txt(string|array(string) txt)
  {
    //  Get flattened string representation of text record
    string txt_flat = get_flat_txt_record(txt);

    //  Invoke C interface
    ::update_txt(txt_flat || "");
  }
  

  //!  Registers a service on the local network.
  //!
  //!  @param name
  //!    User-presentable name of the service.
  //!  @param service
  //!    Type of service on the form @tt{_type._protocol@}. Type is an
  //!    identifier related to the service type. A list of registered
  //!    service types can be found at
  //!    @url{http://http://www.dns-sd.org/ServiceTypes.html/@}. Protocol
  //!    is normally @tt{tcp@} but @tt{udp@} is also a valid choice. For
  //!    example, a web server would get a @[service] of @tt{_http._tcp@}.
  //!  @param domain
  //!    Domain name. Normally an empty string which the DNS-SD library
  //!    will translate into @tt{local.@}.
  //!  @param port
  //!    Port number for the service (e.g. 80 for a web site).
  //!  @param txt
  //!    An optional TXT record with service-specific information. It can
  //!    be given as a plain string or an array of property assignment
  //!    strings. The TXT record can be changed later by calling @[update_txt]
  //!    in the object returned when you register the service.
  //!
  //!  @example
  //!  object svc = Protocols.DNS_SD.Service(
  //!                 "Company Intranet Forum",  //  name
  //!                 "_http._tcp",              //  service type
  //!                 "",                        //  domain (default)
  //!                 80,                        //  port
  //!                 ({ "path=/forum/" })       //  TXT record
  //!               );
  void create(string name, string service, string domain, int port,
	      void|string|array(string) txt)
  {
    if (name)
      name = clip_utf8_str(name, 63);
    
    //  Pack array of text message records
    string txt_flat = get_flat_txt_record(txt);
    
    //  Invoke C code
    ::create(name || "", service, domain || "", port, txt_flat || "");
  }

};
#endif
