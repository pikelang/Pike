#pike __REAL_VERSION__

//! Decodes a Bittorrent bencoded data chunk.
//! @returns
//!   @array
//!     @elem string|int|array|mapping data
//!       The decoded data. UNDEFINED if no data could be decoded.
//!     @elem string remainder
//!       The trailing data that wasn't decoded.
//!   @endarray
array(string|int|array|mapping) _decode(string what)
{
   if (what=="") return ({UNDEFINED,what});
   if (what==0) error("Cannot decode 0 (zero)\n");

   int i;
   string s;
   
   switch (what[0])
   {
      case 'i': // integer
	 if (sscanf(what,"i%de%s",i,what)<2)
	    return ({UNDEFINED,what});
	 return ({i,what});

      case '0'..'9': // string
	 if (sscanf(what,"%d:%s",i,s)<2 ||
	     strlen(s)<i)
	    return ({UNDEFINED,what});
	 return ({s[..i-1],s[i..]});
	 
      case 'l': // list
	 array res=({});
	 s=what[1..];
	 while (s!="")
	 {
	    if (s[0]=='e')
	       return ({res,s[1..]});
	    array v=_decode(s);
	    if (v[1]==s) 
		break;
	    res+=v[..0];
	    s=v[1];
	 }
	 return ({UNDEFINED,what});

      case 'd': // dictionary
	 array keys=({});
	 array values=({});
	 s=what[1..];
	 while (s!="")
	 {
	    if (s[0]=='e') return ({mkmapping(keys,values),s[1..]});

	    array v=_decode(s);
	    if (v[1]==s) break;
	    keys+=v[..0];
	    s=v[1];

	    v=_decode(s);
	    if (v[1]==s) break;
	    values+=v[..0];
	    s=v[1];
	 }
	 return ({UNDEFINED,what});

      default:
	 error("Error in Bencoding: unknown prefix %O...\n",
	       what[..10]);
   }
}

//! Decodes a Bittorrent bencoded data chunk and ignores the remaining
//! string. Returns UNDEFINED if the data is incomplete.
string|int|array|mapping decode(string what)
{
   array v=_decode(what);
   if (v[1]==what) return UNDEFINED;
   return v[0];
}

//! Encodes a Bittorrent bencoded data chunk.
string encode(string|int|array|mapping data)
{
   switch (sprintf("%t",data))
   {
      case "int":
	 return sprintf("i%de",data);
      case "string":
	 return sprintf("%d:%s",strlen(data),data);
      case "array":
	 return "l"+map(data,encode)*""+"e";
      case "mapping":
	 string res="d";
	 array v=(array)data;
	 sort(column(v,0),v);
	 foreach (v;;[string|int|array|mapping key,
		      string|int|array|mapping value])
	 {
	    if (!stringp(key))
	       error("dictionaries (mappings) must have "
		     "string keys!\n");
	    res+=encode(key)+encode(value);
	 }
	 return res+"e";
      default:
	 error("Cannot Bittorrent-Bencode type: %t\n",data);
   }
}

private static array(string) bits=
   sprintf("%08b",Array.enumerate(256)[*]);
private static array(string) bobs=
   sprintf("%c",Array.enumerate(256)[*]);

//! Convert an array of @expr{int(0..1)@} to a Bittorrent style
//! bitstring. Input will be padded to even bytes.
string bits2string(array(int(0..1)) v)
{
   if (sizeof(v)&7) v+=({0})*(8-sizeof(v)&7);
   return replace(sprintf("%@d",v),bits,bobs);
}

//! Convert a Bittorrent style bitstring to an array of
//! @expr{int(0..1)@}.
array(int(0..1)) string2bits(string s)
{
   return (array(int(0..1)))(replace(s,bobs,bits)/1);
}

//! Convert a Bittorrent style bitstring to an array of indices.
array(int) string2arr(string s)
{
   if (last2arrbits==s) return copy_value(last2arrarr); // simple cache
   array v=string2bits(last2arrbits=s);
   array w=indices(v);
   sort(v,w);
   int i=search(v,1);
   return last2arrarr=w[i..];
}

static private string last2arrbits=0;
static private array(int) last2arrarr=0;
