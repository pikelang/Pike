#pike __REAL_VERSION__

// FIXME! doc bad

//! module Protocols
//! submodule HTTP
//! class HeaderParser
//!	Fast HTTP header parser. 

//! module Protocols
//! submodule HTTP
//! method string http_decode_string(string what)

constant HeaderParser=_Roxen.HeaderParser;
constant http_decode_string=_Roxen.http_decode_string;

//! method mapping(string:string|array(string)) http_decode_urlencoded_query(string query,void|mapping dest)
//!	Decodes an URL-encoded query into a mapping.

mapping(string:string|array(string))
   http_decode_urlencoded_query(string query,
				void|mapping dest)
{
   if (!dest) dest=([]);

   foreach (query/"&",string s)
   {
      string i,v;
      if (sscanf(s,"%s=%s",i,v)<2) v=i=http_decode_string(s);
      else i=http_decode_string(replace(i,"+"," ")),v=http_decode_string(replace(v,"+"," "));
      if (dest[i]) 
	 if (arrayp(dest[i])) dest[i]+=({v});
	 else dest[i]=({dest[i],v});
      else dest[i]=v;
   }
   
   return dest;
}


//! method string filename_to_type(string filename)
//! method string extension_to_type(string extension)
//!	Looks up the file extension in a table to return
//!	a suitable MIME type.

string extension_to_type(string extension)
{
   return MIME.ext_to_media_type(extension) || "application/octet-stream";
}

string filename_to_type(string filename)
{
   array v=filename/".";
   if (sizeof(v)<2) return extension_to_type("default");
   return extension_to_type(v[-1]);
}

//! method string http_date(int time)
//!	Makes a time notification suitable for the HTTP protocol.
//! @param time
//!  The time in seconds since the 00:00:00 UTC, January 1, 1970
//! @returns
//!  The date in the HTTP standard date format.
//!  Example : Thu, 03 Aug 2000 05:40:39 GMT
string http_date(int time)
{
   return Calendar.ISO_UTC.Second(time)->format_http();
}

//! method int http_decode_date(string date)
//! 	Decode a HTTP date to seconds since 1970 (UTC)
//! @returns
//!	zero (UNDEFINED) if the given string isn't a HTTP date
int http_decode_date(string data)
{
   Calendar.ISO_UTC.Second s=
      Calendar.ISO_UTC.parse("%e, %D %M %Y %h:%m:%s GMT",data);
   if (!s) return UNDEFINED;
   return s->unix_time();
}

// server id prefab

constant http_serverid=version()+": HTTP Server module";


