// FIXME! doc bad

//! module Protocols
//! submodule HTTP
//! subclass HeaderParser
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
      if (sscanf(s,"%s=%s",i,v)<2) v=i=http_decode_string(i);
      else i=http_decode_string(i),v=http_decode_string(v);
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
//!	a suitable MIME type. The table is located in the
//!	[...]pike/lib/modules/Protocols.pmod/HTTP.pmod/Server.pmod/extensions.txt
//!	file.

mapping extensions=0;

string extension_to_type(string extension)
{
   if (!extensions)
   {
      Stdio.File f=Stdio.FILE(
	 combine_path(__FILE__,"..","extensions.txt"),"r");

      mapping res=([]);

      while (array a=f->ngets(1000))
	 foreach (a,string l)
	    if (sscanf(l,"%*[ \t]%[^ \t]%*[ \t]%[^ \t]",
		       string ext,string type)==4 &&
		ext!="" && ext[0]!='#')
	       res[ext]=type;

      extensions=res;
   }

   return extensions[extension] || extensions["default"] ||
      "application/octet-stream";
}

string filename_to_type(string filename)
{
   array v=filename/".";
   if (sizeof(v)<2) return extension_to_type("default");
   return extension_to_type(v[-1]);
}

//! method string http_date(int time)
//!	Makes a time notification suitable for the HTTP protocol.

string http_date(int time)
{
   return Calendar.ISO_UTC.Second(time)->format_http();
}


// server id prefab

constant http_serverid=version()+": HTTP Server module";

