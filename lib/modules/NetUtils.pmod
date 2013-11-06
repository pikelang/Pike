/* -*- Mode: pike; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

//! Various networking-related utility functions.

final constant IPV6_FULL_MASK = 0xffffffffffffffffffffffffffffffff;
final constant IPV4_FULL_MASK = 0xffffffff;
final constant IPV4_START     = 0x01000000;

final constant DOMAIN_NAME_MAX_LENGTH  = 253;
final constant DOMAIN_LABEL_MAX_LENGTH = 63;

//! Converts a binary representation of an IP address into the
//! IPv4 or IPv6 string representation.
//!
//! The reverse of string_to_ip.
//!
//! @param ip
//!     The binary representation of the address.
//! @param v6_only
//!     Always return IPV6 addresses. IPV4 addresses will be formatted
//!     as ::FFFF:<ipv4>
//! @returns
//!     The string representation of the address, or 0 if the IP
//!     was invalid.
string ip_to_string( int ip, bool|void v6_only )
{
    if( ip < 0 )
        return 0;

    if( ip == 1 ) // Special case for ipv6 localhost
        return "::1";

    if( /*ip > IPV4_START &&*/ ip <= IPV4_FULL_MASK )
        return sprintf("%s%d.%d.%d.%d",
                       (v6_only?"::ffff:":""),
                       (ip>>24), (ip>>16)&255, (ip>>8)&255, (ip)&255);

    array x = ({});
    while( ip )
    {
        x += ({ (ip&65535) });
        ip>>=16;
    }
    x = ({ 0 })*(8-sizeof(x)) + reverse(x);
    return Protocols.IPv6.format_addr_short(x);
}

//! Converts a string representation of an IPv4 address into the
//! binary representation.
//!
//! @param ips
//!     The string representation of the address.
//!     For convenience this function accepts the notation returned
//!     from fd->query_adddress() ("ip port")
//! @returns
//!     The binary representation of the address, or -1 if the
//!     string could not be parsed.
int string_to_ip(string ips)
{
    if( has_value( ips, " " ) )
        ips = ips[..search(ips, " ")-1];
    if( has_value( ips, ":" ) )
    {
        sscanf( ips, "%s/%*s", ips );
        int res;
        array(int) ipsplit = Protocols.IPv6.parse_addr( ips );
        if( !ipsplit ) return -1;

        foreach( ipsplit, int i )
        {
            res <<= 16;
            res |= i;
        }
        if( res < 0x1000000000000 && res > 0xffff01000000 )
        {
            // ::FFFF:<ipv4> - IPv4 mapped to ipv6.
            // Since we use the (older) ::<ipv4> format, map to that
            // instead.
            return res & 0xffffffff;
        }
        return res;
    }
    int a, b, c, d;
    if ( sscanf( ips, "%d.%d.%d.%d%s", a, b, c, d, string rest ) != 5 || sizeof(rest) )
        return -1;
    if (a<0 || a>255 || b<0 || b>255 || c<0 || c>255 || d<0 || d>255)
        return -1;
    return (((((a<<8)|b)<<8)|c)<<8)|d;
}

//! Returns the CIDR of a given netmask.  Only returns the correct
//! value for netmasks with all-zeroes at the end (eg, 255.255.255.128
//! works, while 255.255.255.3 will give the wrong return value)
//!
//! @seealso
//!   http://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing#CIDR_notation
int netmask_to_cidr( string mask )
{
    if( has_prefix( mask, "0x" ) )
        mask = mask[2..];
    int i;
    string s;
    if( sscanf( mask, "%x%s", i, s ) == 2 && s == "")
        return i->popcount();
    return string_to_ip( mask )->popcount();
}

//! Converts a string with an IP address and mask (in CIDR notation)
//! into a binary IP address and bitmask.
//!
//! @param cidr
//!     The CIDR-notation input string.
//! @returns
//!     An array containing:
//!     @array
//!         @elem int ip
//!             The binary representation of the IP address.
//!         @elem int mask
//!             The bitmask.
//!     @endarray
//!     Returns 0 if the string could not be parsed.
array(int) cidr_to_netmask(string cidr)
{
    string ips;
    int bits;

    if (!cidr)
        return 0;
    if (sscanf(cidr, "%s/%d", ips, bits) != 2)
        return 0;
    int net = string_to_ip( ips );
    if (net == -1)
        return 0;

    int mask = IPV6_FULL_MASK;
    if( has_value( ips, ":" ) )
    {
        if (bits < 0 || bits > 128)
            return 0;
        mask <<= 128-bits;
    }
    else
    {
        if (bits < 0 || bits > 32)
            return 0;
        mask <<= 32-bits;
    }
    mask &= IPV6_FULL_MASK;
    return ({ net&mask, mask });
}

//! Checks whether an IP address is in a block.
//!
//! The net and mask parameters should be as returned from @[cidr_to_netmask].
//!
//! Throws an error if the IP address could not be parsed.
//!
//! @param net
//!     The network component of the block.
//! @param mask
//!     The bitmask of the block.
//! @param ip
//!     The IP address to check, in either string or binary representation.
//! @returns
//!     true if the IP is in the given block, false otherwise.
bool ip_in_block(int net, int mask, int|string ip)
{
    if (stringp(ip))
        ip = string_to_ip(ip);
    if (ip == -1 || net == -1)
        error("ip and net must be an int or an address string.\n");
    if( mask <= IPV4_FULL_MASK )
    {
        // ipv4.
        if ( (net & IPV4_FULL_MASK) != net || (mask & IPV4_FULL_MASK) != mask )
            error("net and mask must be between 0 and 0xffffffff.\n");
    }
    if ((ip & mask) == net)
        return true;
    return false;
}

//! Perform a basic sanity check on hostname based on total and
//! subdomain label length. Does not test for invalid chars.
//!
//! @param hostname
//!     Domain name string to test.
//! @returns
//!     True if @[hostname] looks like a valid domain name
bool valid_domain_name( string hostname )
{
    return hostname
        && strlen(hostname)
        && strlen(hostname) <= DOMAIN_NAME_MAX_LENGTH
        && Array.all( map( hostname/".", strlen ), `<=,
                      DOMAIN_LABEL_MAX_LENGTH );
}

//! Persistent representation of a network + mask.
class NetMask
{
    //! The network number
    int net;

    //! The network mask
    int mask;

    //! Construct a new NetMask object from the given CIDR.
    //! @param cidr
    //!  An IP and mask in CIDR notation.
    protected void create(string cidr)
    {
        if( has_value(cidr, "/" ) )
        {
            array(int) res = cidr_to_netmask(cidr);
            if (!res)
                error("bad CIDR: %s.\n", cidr);
            [net, mask] = res;
        }
        else
        {
            net = string_to_ip(cidr);
            if( net == -1 )
                error("bad address/CIDR: %s.\n", cidr);
            mask = IPV6_FULL_MASK;
        }
    }

    //! Match an IP number against the mask.
    //! @returns
    //!  true if the @[ip] is in the network, false otherwise.
    //! @param ip
    //!    The IP address to check, in either string or binary representation.
    bool ip_in(int|string ip)
    {
        return ip_in_block(net, mask, ip);
    }

    //! Convert to either a string (back to CIDR notation) or an array
    //! (net,mask)
    protected mixed cast(string type)
    {
        switch( type )
        {
            case "string":
                string a = ip_to_string(net);
                int n = mask->popcount();

                if( net >= IPV4_START && net <= IPV4_FULL_MASK && n >= 96)
                    n = 32 - (128-n);
                else if( has_value(a,".") )
                {
                    if( a == "0.0.0.0" )
                        a = "::";
                    else
                        a = "::"+a;
                }
                return a+"/"+n;
            case "array":
                return ({ net, mask });
            default:
                error("Can not cast to %O\n", type );
        }

    }

    protected string _sprintf( int flag, mapping opt )
    {
        switch( flag )
        {
            case 't': return "NetMask";
            case 'O': return "NetMask("+(string)this+")";
            default: return (string)this;
        }
    }
}

//! Class used for checking an IP against a list of IP ranges and
//! looking up some associated information.
class IpRangeLookup
{
    private mapping(string:array(Range)) range_to_info = ([]);

    //! Represents a single range in a IpRangeLoopup.
    class Range
    {
        //!
        inherit NetMask;

        //!
        mixed info;

        protected bool `==( mixed x )
        {
            if( objectp(x) &&
                (x->net == net) &&
                (x->mask == mask) )
                return true;
            return false;
        }

        protected bool `<( mixed x )
        {
            if( objectp(x) &&
                ((x->net < net) ||
                 (x->mask < mask)) )
                return true;
            return false;
        }

        protected void create( string mask, string i )
        {
            info = i;
            ::create(mask);
        }

        protected string _sprintf(int flag)
        {
            return sprintf("Range(%O -> %O)", (string)this, info);
        }
    }

    //! Return a copy of the internal range to info mapping.
    mapping(string:array(Range)) get_ranges()
    {
        return range_to_info + ([]);
    }

    protected string _sprintf()
    {
        return sprintf("IpRangeLookup( %{%O %})",Array.flatten(values(range_to_info)));
    }

    //! Looks up an IP address and returns the associated information, if any.
    //!
    //! @param ipstr
    //!     The IP address in string or binary form.
    //! @returns
    //!     The information associated with the most-specific IP range
    //!     matching ipstr.
    mixed lookup(int|string ipstr)
    {
        Range x = lookup_range( ipstr );
        if( x ) return x->info;
    }

    //! Looks up an IP address and returns the associated @[Range], if any.
    //!
    //! @param ipstr
    //!     The IP address in string or binary form.
    //! @returns
    //!     The matching net-range
    Range lookup_range(int|string ipstr)
    {
        int ip = stringp(ipstr) ? string_to_ip( [string]ipstr ) : [int]ipstr;
        if( ip < 0 ) return false;

        string ip_str = ip->digits(256);

        for( int i=0; i<=strlen(ip_str); i++ )
            if(array(Range) ranges=range_to_info[ip_str[..<i]])
                foreach(ranges, Range x )
                    if(x->ip_in( ipstr ))
                        return x;
    }


    // Ensures that the netmask only contains 255 or 0 bytes.
    // This is used to speed up the lookup() function above.
    static string trim_net( int net, int mask )
    {
        int x = (net&(((1<<(mask->size()))-1)^((1<<(mask->size()-(mask->popcount()&~7)))-1)));

        string q = x->digits(256);
        while( strlen(q) && q[-1] == 0 )
            q = q[..<1];
        return q;
    }

    //! Creates a new IpRangeLookup object and initialises the IP range
    //! table.  Errors will be thrown if @[ranges] contains invalid
    //! data.
    //!
    //! @param ranges
    //!     A mapping from information data to arrays of IP ranges.
    //!
    //!     Each range can be a single addresses ("192.168.1.1"), a
    //!     range of addresses ("192.168.1.1-192.168.1.5") or be
    //!     written in CIDR notation ("192.168.1.0/24").
    void create(mapping(mixed:array(string)) ranges, int|void _debug)
    {
        void add_range( Range range )
        {
            string key = trim_net(range->net,range->mask);
            if( !range_to_info[key] )
                range_to_info[key] = ({ range });
            else
                range_to_info[key] |= ({ range });
        };
        foreach( ranges; string info; array(string) ips )
        {
            ips = Array.uniq( ips );
            foreach( ips, string ip )
            {
                if( has_value( ip, "-" ) )
                {
                    int a = string_to_ip( (ip/"-")[0] );
                    int b = string_to_ip( (ip/"-")[-1] );
                    if( b-a > 1000 )
                    {
                        error("FIXME: We need to optimize this. Ranges should not be very large at the moment.\n");
                    }
                    for( int ipint = a; ipint<=b; ipint++ )
                        add_range(Range( ip_to_string(ipint), info ));
                }
                else
                    add_range( Range( ip, info ) );
            }
        }
        foreach( range_to_info; string net; array(Range) x )
        {
            sort(x->mask,x);
            range_to_info[net] = reverse(x);
        }
    }
}

//! Return the CIDR notation for the single host defined by @[x].
//!
//! @param ip
//!   The host ip in either string or raw form
//! @returns
//!    either @[ip]/128 or @[ip]/32 depending on the IPV6-ness of the host IP.
string host_to_cidr( int|string ip )
{
    if( intp( ip ) )
        ip = ip_to_string( ip );
    if( !has_value( ip, ":" ) )
        return ip+"/32";
    return ip+"/128";
}

//! Returns true if the IP @[ip] is a IPV6 IP.
bool is_ipv6( int|string ip )
{
    if( stringp( ip ) )
        ip = string_to_ip( ip );

    if( ip == -1 )
        return false;

    return (ip > IPV4_FULL_MASK) || (ip < IPV4_START);
}



//! Interface for objects that can be sent to ip_of and friends.
//!
//! This matches at least Stdio.File and Stdio.Port, Stdio.UDP and
//! some other classes.
class RemoteAddressObject
{
    string query_address(int|void l);
}

//! If the argument is an object with a @[query_address] method,
//! return the IP# part of the string returned by calling that
//! function with @[local_address] as the argument.
//!
//! This means that for Stdio.File objects the remote address is
//! returned by default, but if @[local_address] is true the local
//! address is returned.
//!
//! If the argument is a string instead, the part of the string before
//! the first space is returned.
//!
//! If the argument is 0 the default @[def] value is returned,
//! UNDEFINED unless specified.
//!
//! If @[def] is supplied, it is used both when query_address() fails
//! or something that is not a file descriptor object or a string is
//! passed as the argument to this function.
string ip_of( RemoteAddressObject|string|int(0..0) inc,
              bool|void local_address,
              string|void def)
{
    array ip_and_port = ip_and_port_of(inc, local_address);
    if (!ip_and_port || sizeof(ip_and_port) == 0)
        return def;
    return ip_and_port[0];
}

//! Similar to ip_of but instead of IP returns port number.
//!
//! If the argument is an object with a @[query_address] method,
//! return the port# part of the string returned by calling that
//! function with @[local_address] as the argument.
//!
//! This means that for Stdio.File objects the remote address is
//! returned by default, but if @[local_address] is true the local
//! address is returned.
//!
//! If the argument is a string instead, the part of the string after
//! the first space is returned.
//!
//! If the argument is 0 the default @[def] value is returned,
//! UNDEFINED unless specified.
//!
//! If @[def] is supplied, it is used both when query_address() fails
//! or something that is not a file descriptor object or a string is
//! passed as the argument to this function.
string port_of( RemoteAddressObject|string|int(0..0) inc,
                bool|void local_address,
                string|void def)
{
    array ip_and_port = ip_and_port_of(inc, local_address);
    if (!ip_and_port || ip_and_port[1] == 0)
        return def;
    return ip_and_port[1];
}

//! Similar to ip_of. Returns 2-element array containing IP address
//! and port number.  Second element will be 0 if no port number can
//! be retrieved.
//!
//! This function can return 0 if @[inc] is a @[RemoteAddressObject]
//! and query_address throws an error or does not return a string.
array(string) ip_and_port_of( RemoteAddressObject|string|int(0..0) inc,
                              bool|void local_address)
{
    if( objectp(inc) && (catch(inc = inc->query_address(local_address)) || !inc) )
        return 0;

    if( !stringp(inc) )
        return 0;

    array ip_and_port = inc / " ";
    if (sizeof(ip_and_port) < 2)
        ip_and_port += ({ 0 });
    return ip_and_port;
}

private multiset(string) __lips;
private multiset(int)    __lipi;
private mapping(string:array(string)) __interfaces;
private IpRangeLookup _local_networks;
private mapping _broadcast_addresses;
private array(NetworkType) __c_n_t;



//! Clear any caches. This might be needed if the network setup on the
//! system changes.
void clear_cache()
{
    __lips = 0;
    __lipi = 0;
    __interfaces = 0;
    __c_n_t = 0;

    _local_networks = 0;
    _special_networks = 0;
}


//--- FIXME: The part below this is OS dependend, and does not really
//--- support anything except Linux.

private string _ifconfig =
    Process.locate_binary( ({ "/usr/sbin","/sbin" }), "ifconfig") ||
    Process.search_path("ifconfig");


enum System {
    Linux, NT, Other, Unsupported
};

private System find_system() {
#if defined(__NT__)
    return NT;
#else
    if( uname()->sysname == "Linux" ||
        String.count(Process.popen(_ifconfig+" -s 2>/dev/null"),"\n") > 1 )
        return Linux;
    return _ifconfig ? Other : Unsupported;
#endif
}

private System system = find_system();


string ifconfig( string command )
{
    if( system == Unsupported ) {
        error("This system is unfortunately not currentl supported\n");
    }
    switch( command )
    {
        case "list if":
            switch( system )
            {
                case Linux:
                {
                    string data = Process.popen( _ifconfig + " -s" );
                    return column(((data/"\n")[*]/" ")[1..<1],0)*"\n";
                }
                case NT:
                    error("FIXME: NT not currently supported\n");
                    return Process.popen( "ipconfig /all" );

                default:
                 {
                     string data = Process.popen(_ifconfig + " -a | grep -v '\t'");
                     array res = ({});
                     foreach( data/"\n", string x )
                     {
                         if( strlen( x ) )
                         {
                             x = (x/" ")[0];
                             res += ({ x[..<1] });
                         }
                     }
                     return res*"\n";
                 }
            }
        case "all":
            if( system == NT )
                return Process.popen( "ipconfig /all");
            return Process.popen( _ifconfig + " -a" );
        default:
            return Process.popen( _ifconfig +" "+ command );
    }
}


//! Return a mapping from interface to address/netmask (only returns
//! non link-local addresses for ipv6)
mapping(string:array(string)) local_interfaces()
{
    if( __interfaces )
        return __interfaces;

    mapping(string:array(string)) next__interfaces = ([]);

    mapping(string:array(string)) next__broadcast_addresses = ([]);
    foreach( ifconfig("list if" )/"\n", string iface )
    {
        array ips = ({});
        foreach( (ifconfig(iface) + ifconfig(iface + " inet6"))/"\n",
                 string q )
        {
            string i,m;
            q = String.trim_whites(q);
            if( (sscanf( q, "inet addr:%[^ ]%*sMask:%s", i, m )==3) ||
                (sscanf( q, "inet %[^ ] mask %[^ ]", i, m )==2) ||
                (sscanf( q, "inet %[^ ]%*snetmask %[^ ]", i, m )==3))
            {
                ips += ({i+"/"+netmask_to_cidr(m)});
                ips += ({ "::ffff:"+i+"/"+(96+netmask_to_cidr(m)) });
            }
            else if( (sscanf( q, "inet6 addr: %[^ ]", i ) ) ||
                     (sscanf( q, "inet6 %[^ ] ", i )) )
            {
                if( !has_prefix( i, "fe80::" ) )
                    ips += ({ i });
                next__broadcast_addresses[iface] += ({ "ff02::1" });
            }
            if( (sscanf( q, "%*sBcast:%[^ ]", i ) == 2) ||
                (sscanf( q, "%*sbroadcast %s", i ) == 2 ) )
            {
                next__broadcast_addresses[iface] += ({i});
            }
        }
        if( sizeof( ips ) )
            next__interfaces[iface] = ips;
    }
    next__interfaces["lo"] += ({ "::/128" });
    __interfaces = next__interfaces;
    _broadcast_addresses = next__broadcast_addresses;
    return __interfaces;
}
//--- End of FIXME.


//! Returns an IpRangeLookup that can be used to find the interface
//! for an IP address.
IpRangeLookup local_networks()
{
    if( !_local_networks )
        _local_networks = IpRangeLookup( local_interfaces() );
    return _local_networks;
}


//! Returns either 0 or "::" depending on whether or not this computer
//! has IPv6 support.
//!
//! The intent is to use this when binding addresses, like
//! port->bind(portno,callback,NetUtils.ANY) to get ipv6 support if
//! present.
//!
//! The reason for the existence of this function is that using "::"
//! on a computer that does not actually have any ipv6 addresses (and
//! thus no support for ipv6), at least on linux, causes the bind call
//! to fail entirely.
string `ANY()
{
    if( has_ipv6() )
        return "::";
    return 0;
}

//! Returns true if the local host has a public IPv6 address
bool has_ipv6()
{
    foreach( local_ips(); string ip; )
        if( is_ipv6( ip ) )
            return true;
    return false;
}

//! Returns true if the local host has a public IPv4 address
bool has_ipv4()
{
    foreach( local_ips(); string ip; )
        if( !is_ipv6( ip ) )
            return true;
    return false;
}

//! Return an array with locally configured IP-numbers, excluding the
//! ones configured on the loopback inteface, unless
//! @[include_localhost] is specified.
multiset(string) local_ips(bool|void include_localhost)
{
    if( include_localhost )
    {
        array(string) local_cidr = Array.sum( values(local_interfaces()) );
        return (multiset(string))Array.uniq(column(local_cidr[*]/"/",0));
    }
    if( __lips )
        return __lips;

    multiset(string) res = (<>);
    foreach( local_interfaces(); string iface; array(string) ip )
    {
        if( has_prefix(iface, "lo") )
            continue;
        res |= (multiset)column(ip[*]/"/",0);
    }
    // res["127.0.0.1"] = 1;
    return __lips = res;
}

//! Much like local_ips, but returns the IP:s parsed to the integer
//! raw format.
multiset(int) local_ips_raw(bool|void include_localhost)
{
    if( __lipi )
        return __lipi;
    multiset(int) res = (<>);
    foreach( local_ips( include_localhost ); string k;  )
        res[ string_to_ip( k ) ] = 1;
    return __lipi = res;
}

//! Returns true if host points to the local host.
//! @param host
//!  The host to check
//! @param only_localhost
//!  Only check if it is ipv6 or ipv4 localhost, not if it is one of
//!  the public IP# of this host.
//! @returns
//!   true if the given @[host] is the local host, false otherwise
bool is_local_host(RemoteAddressObject|string|int host,
                   bool|void only_localhost)
{
    if( host && intp( host ) )
    {
        if( (host == 1) || host == (127<<24)+1 )
            return true;
        if( !only_localhost && local_ips_raw()[host] )
            return true;
        return false;
    }
    if( !host )
        return false;

    host = ip_of(host);
    return (has_prefix( host, "127.0" ) ||
            (string)Protocols.IPv6.parse_addr(host) == "\0\0\0\0\0\0\0\1" ||
            (!only_localhost && local_ips_raw()[string_to_ip(host)])) && true;

}

//! Returns non-zero if host is on one of the local networks, and if
//! so which interface it is on.
string is_local_network(RemoteAddressObject|int|string host)
{
    if( !host ) return false;
    if( !intp(host) )
        host = ip_of(host);
    return local_networks()->lookup(host);
}

//! Returns a mapping from interface name to the broadcast addresses
//! on that interface.
mapping(string:array(string)) broadcast_addresses()
{
    if( !_broadcast_addresses )
        local_networks();
    return _broadcast_addresses;
}

//! Returns either ::1 or 127.0.0.1 depending on the availability of
//! IPV6.
string local_host()
{
    if( has_ipv6() )
        return "::1";
    return "127.0.0.1";
}

private IpRangeLookup _special_networks;

// NOTE: If you add a network type here you also need to fix
// connectable_network_types below.
//! A list of all known network type/classes
final enum NetworkType
{
    //! V4 and in non-v6-separate mode also used for V6
    LOCALHOST = "localhost",
    LOCAL = "local",
    MULTICAST = "multicast",
    GLOBAL = "global",
    PRIVATE = "private",

    //! V6 only versions
    LOCALHOSTV6 = "localhostv6",
    LOCALV6 = "localv6",
    PRIVATEV6 = "privatev6",
    MULTICASTV6 = "multicastv6",
    GLOBALV6 = "globalv6",

    //! Tunneling reserved addresses
    TEREDO  = "teredo",
    V6TO4 = "6to4",

    LINKLOCAL = "linklocal",
};

//! Mapping from @[NetworkType] to an integer between 0 and 10 that
//! describes how local that type of network is.
constant locality = ([
    LOCALHOST:10, LOCALHOSTV6:10,

    LINKLOCAL:9,

    PRIVATEV6:4, PRIVATE:4,

    MULTICASTV6:2, MULTICAST:2,

    TEREDO:1, V6TO4:1,
    GLOBAL:0, GLOBALV6:0,
]);

//! Returns true if @[which] is less globally accessible than
//! @[towhat].
bool ip_less_global( int|string which, int|string towhat, bool|void prefer_v4 )
{
    if( !which )
        return true;
    string which_type = special_networks()->lookup(which);
    string towhat_type = special_networks()->lookup(towhat);
    if( prefer_v4 &&
        locality[which_type] == locality[towhat_type] )
    {
        if( is_ipv6( which ) )
            return true;
        return false;
    }
    return locality[which_type] > locality[towhat_type];
}

//! Normalize the IP specified in @[a].  This normalizes IPv6
//! addresses and converts ::FFFF:<ipv4> and ::<ipv4> to "normal" IPV4
//! addresses.
//!
//! Will return 0 if @[a] is not a valid address.
string normalize_address( string a )
{
    if( !a ) return 0;

    array(string) seg = a/" ";
    int ip = string_to_ip( seg[0] );

    if( ip == -1 )
        return 0;

    seg[0] = ip_to_string(ip);
    return seg * " ";
}

//! Return an @[IpRangeLookup] instance useful for finding out the
//! address category of a ip.  Basically like @[get_network_type]
//! without the "local" category.
IpRangeLookup special_networks()
{
    if( !_special_networks )
    {
        _special_networks = IpRangeLookup(
            ([
                TEREDO:({ "2001:0::/32" }),
                V6TO4:({ "2002::/16" }),
                PRIVATE:({ "10.0.0.0/8", "172.16.0.0/12", "192.168.0.0/16", }),
                PRIVATEV6:({  "fc00::/7" }),
                LINKLOCAL:({  "fe80::/16" }),
                MULTICAST:({ "224.0.0.0/4"}),
                MULTICASTV6:({ "ff00::/8" }),
                GLOBAL:({ "::/96" }),
                GLOBALV6:({ "::/0" }),

                LOCALHOSTV6:({"::1/128"}),
                LOCALHOST:({"127.0.0.0/8"}),
            ]) );
    }
    return _special_networks;
}

//! Determine the network type of a given host
//!
//! @param ipstr
//!    IP address in string or numerical form
//! @param separate_6
//!    Adds a 'v6' to the category for ipv6 addresses (ie, "global" and "globalv6")
//!
//! @returns
//!   "localhost", "local", "private", "multicast", "teredo", "6to4" or "global"
//!
//!  "localhost" is the local computer.
//!
//!  "local" is the local network
//!
//!  "private" is a private network, such as
//!    10.0.0.0/8 or fc00::/7 that is not also a local network.
//!
//!  "multicast" is a multicast address
//!
//!  "teredo" and "6to4" is an address in the teredo and 6to4 tunneling
//!     system, respectively.
//!
//!  "global" is a global address that does not match any other category
NetworkType get_network_type( int|string ipstr, bool|void separate_6  )
{
    if( !ipstr ) return UNDEFINED;

    int ip = intp(ipstr) ? ipstr : string_to_ip( ipstr );
    if( ip < 0 ) return UNDEFINED;

    if( is_local_host( ip ) )
    {
        if( separate_6 && ip < IPV4_START || ip > IPV4_FULL_MASK )
            return LOCALHOSTV6;
        return LOCALHOST;
    }

    string res = special_networks()->lookup( ip );
    if( res == "linklocal" )
        return res;

    if( local_networks()->lookup( ip ) )
    {
        if( separate_6 && ip > IPV4_FULL_MASK )
            return LOCALV6;
        return LOCAL;
    }

    if( !separate_6 && res[<1..] == "v6" )
        return res[..<2];
    return res;
}


// Used for seltests.
array(NetworkType) _connectable_network_types( bool v6, bool v4, bool has_teredo, bool has_6to4 )
{
    array(NetworkType) res =
    ({
         LOCALHOSTV6, LOCALHOST,
         LOCALV6, LOCAL,
         GLOBALV6, GLOBAL,
         MULTICASTV6, MULTICAST,
         TEREDO, V6TO4,
         PRIVATEV6, PRIVATE,
    });

    if( !v6 )
    {
        res -= ({ LOCALHOSTV6, LOCALV6, GLOBALV6, MULTICASTV6, PRIVATEV6, TEREDO, V6TO4 });
    }
    else
    {
        array(NetworkType) prio = ({});
        if( has_teredo )
            prio += ({ TEREDO });
        if( has_6to4 )
            prio += ({ V6TO4 });
        if( sizeof(prio) )
            res = res[..search(res,GLOBALV6)-1] + prio + (res[search(res,GLOBALV6)..]-prio);
    }
    if( !v4 )
    {
        res -= ({ LOCALHOST, LOCAL, GLOBAL, MULTICAST, PRIVATE, });
    }

    return res;
}

//! Returns network types in priority order according to RFC 3484.
//!
//! This function assumes a network category (ipv4 or ipv6) is
//! available if the local host has a configured address (excluding
//! localhost) of that network type.
//!
//! This function will always list the v6 and non-v6 addresses
//! separately.
array(NetworkType) connectable_network_types()
{
    if( __c_n_t ) return __c_n_t;

    bool has_teredo;
    bool has_6to4;

    foreach(local_ips(); string ip; )
    {
        if(get_network_type( ip ) == TEREDO )
            has_teredo = true;
        if(get_network_type( ip ) == V6TO4 )
            has_6to4 = true;
    }

    return __c_n_t
        = _connectable_network_types( has_ipv6(),
                                      has_ipv4(),
                                      has_teredo,
                                      has_6to4 );
}

// Used for seltests.
array(string) _sort_addresses(array(string) addresses,
                              array(NetworkType) exclude_types,
                              bool separate_v6,
                              array(NetworkType) connectable_types )
{
    //1: group according to network type.
    mapping(string:array(string)) tmp = ([]);
    array(string) res = ({});

    foreach( addresses, string ip )
        tmp[get_network_type(ip,true)] += ({ ip });

    //2: Delete ignored types
    if( exclude_types )
    {
        m_delete(tmp, exclude_types[*] );
        if( !separate_v6 )
            m_delete(tmp, (exclude_types[*]+"v6")[*] );
    }

    //3: Sort
    foreach( connectable_types, string type )
        if( tmp[type] )
            res += Array.shuffle(tmp[type]);

    return res;
}

//! Given a list of addresses, sort them according to connectable
//! priority order (RFC 3484).
//!
//! If @[exclude_types] is specified, addresses that match any of the
//! network types (({"local", "localhost"}) for the local network as an example)
//! in the given array will be exluded from the result.
//!
//! If @[separate_v6] is true, @[exclude_types] separates v6 from
//! v4. That is, you can disable "localhost" without also disabling
//! "localhostv6".
//!
//! The addresses inside each group will be returned in random order.
array(string) sort_addresses( array(string) addresses,
                              array(NetworkType)|void exclude_types,
                              bool|void separate_v6)
{
    return _sort_addresses( addresses, exclude_types, separate_v6,
                            connectable_network_types() );
}
