#pike __REAL_VERSION__

//! Decodes a Bittorrent bencoded data chunk and ignores the remaining
//! string. Returns @expr{UNDEFINED@} if the data is incomplete.
string|int|array|mapping decode(Stdio.Buffer buf)
{
  // Decode strings. Returns UNDEFINED if the declared size is smaller
  // than the rest of the buffer.
  if( array a = buf->sscanf("%d:") )
    return buf->read(a[0]) || UNDEFINED;

  mixed prefix;
  switch(prefix=buf->read(1))
  {
  case "":
  case 0:
    // End of buffer
    return UNDEFINED;

  case "e":
    // End of list
    return UNDEFINED;

  case "i":
    // Integer
    // 0-prefixed integers are illegal, but we don't check for
    // that. Also -0 is illegal, and not checked for.
    return buf->sscanf("%de")[0];

  case "l":
    // List
    array list = ({});
    while(1)
    {
      mixed item = decode(buf);
      if( undefinedp(item) )
        return list;
      list += ({ item });
    };

  case "d":
    // Dictionary
    mapping dic = ([]);
    while(1)
    {
      // Keys must be strings and appear in sorted order. We don't
      // check for these restrictions.
      mixed ind = decode(buf);
      if( undefinedp(ind) ) return dic;
      mixed val = decode(buf);
      if( undefinedp(val) ) return dic;
      dic[ind] = val;
    }

  default:
    error("Error in Bencoding: unknown prefix %O...\n", prefix);
  }
}

variant string|int|array|mapping decode(string what)
{
  return decode(Stdio.Buffer(what));
}

__deprecated__ array(string|int|array|mapping) _decode(string what)
{
  Stdio.Buffer b = Stdio.Buffer(what);
  return ({ decode(b), b->read() });
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

private protected array(string) bits=
   sprintf("%08b",Array.enumerate(256)[*]);
private protected array(string) bobs=
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
   if (i==-1) return last2arrarr=({});
   return last2arrarr=w[i..];
}

protected private string last2arrbits=0;
protected private array(int) last2arrarr=0;
