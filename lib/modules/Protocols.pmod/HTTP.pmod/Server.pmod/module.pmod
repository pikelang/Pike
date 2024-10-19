#pike __REAL_VERSION__

//! Fast HTTP header parser.
constant HeaderParser=_Roxen.HeaderParser;

//!
constant http_decode_string=_Roxen.http_decode_string;


//! Decodes an URL-encoded query into a mapping.
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

protected Regexp getextension = Regexp("\\.([^.~#]+)[~#]*$");

//! Determine the extension for a given filename.
string filename_to_extension(string filename)
{
   array(string) ext = getextension->split(filename);
   return ext && ext[0] || "";
}

//! Looks up the file extension in a table to return a suitable MIME
//! type.
string extension_to_type(string extension)
{
   return MIME.ext_to_media_type(extension) || "application/octet-stream";
}

//! Looks up the file extension in a table to return a suitable MIME
//! type.
string filename_to_type(string filename)
{
   filename = filename_to_extension(filename);
   return extension_to_type(sizeof(filename)
                            ? lower_case(filename) : "default");
}

private int lt=-1;
private string lres;

//!	Makes a time notification suitable for the HTTP protocol.
//! @param time
//!  The time in seconds since the 00:00:00 UTC, January 1, 1970
//! @returns
//!  The date in the HTTP standard date format.
//!  Example : Thu, 03 Aug 2000 05:40:39 GMT
string http_date(int time)
{
   if( time == lt ) return lres;
   lt = time;
   return lres = Calendar.ISO_UTC.Second(time)->format_http();
}

//! Decode a HTTP date to seconds since 1970 (UTC)
//! @returns
//!	zero (UNDEFINED) if the given string isn't a HTTP date
int http_decode_date(string data)
{
   Calendar.ISO_UTC.Second s=Calendar.ISO_UTC.http_time(data);
   if (!s) return UNDEFINED;
   return s->unix_time();
}

// server id prefab

constant http_serverid=version()+": HTTP Server module";
