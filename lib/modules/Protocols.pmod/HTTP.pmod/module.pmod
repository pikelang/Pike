#pike __REAL_VERSION__

//! module Protocols
//! submodule HTTP
//! method object(Protocols.HTTP.Query) get_url(string|Standards.URI url)
//! method object(Protocols.HTTP.Query) get_url(string|Standards.URI url,mapping query_variables)
//! method object(Protocols.HTTP.Query) get_url(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method object(Protocols.HTTP.Query) get_url(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! 	Sends a HTTP GET request to the server in the URL
//!	and returns the created and initialized <ref>Query</ref> object.
//!	0 is returned upon failure. If a query object having
//!	request_headers->Connection=="Keep-Alive" from a previous request is
//!	provided and the already established server connection can be used for
//!	the next request, you may gain some performance.
//!
//! method object(Protocols.HTTP.Query) put_url(string|Standards.URI url)
//! method object(Protocols.HTTP.Query) put_url(string|Standards.URI url,string file)
//! method object(Protocols.HTTP.Query) put_url(string|Standards.URI url,string file,mapping query_variables)
//! method object(Protocols.HTTP.Query) put_url(string|Standards.URI url,string file,mapping query_variables,mapping request_headers)
//! method object(Protocols.HTTP.Query) put_url(string|Standards.URI url,string file,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! 	Sends a HTTP PUT request to the server in the URL
//!	and returns the created and initialized <ref>Query</ref> object.
//!	0 is returned upon failure. If a query object having
//!	request_headers->Connection=="Keep-Alive" from a previous request is
//!	provided and the already established server connection can be used for
//!	the next request, you may gain some performance.
//!
//! method object(Protocols.HTTP.Query) delete_url(string|Standards.URI	url)
//! method object(Protocols.HTTP.Query) delete_url(string|Standards.URI	url,mapping query_variables)
//! method object(Protocols.HTTP.Query) delete_url(string|Standards.URI	url,mapping query_variables,mapping request_headers)
//! method object(Protocols.HTTP.Query) delete_url(string|Standards.URI	url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! 	Sends a HTTP DELETE request to the server in the URL
//!	and returns the created and initialized <ref>Query</ref> object.
//!	0 is returned upon failure. If a query object having
//!	request_headers->Connection=="Keep-Alive" from a previous request is
//!	provided and the already established server connection can be used for
//!	the next request, you may gain some performance.
//!
//! method array(string) get_url_nice(string|Standards.URI url,mapping query_variables)
//! method array(string) get_url_nice(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method array(string) get_url_nice(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! method string get_url_data(string|Standards.URI url,mapping query_variables)
//! method string get_url_data(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method string get_url_data(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//!	Returns an array of ({content_type,data}) and just the data
//!	string respective, 
//!	after calling the requested server for the information.
//!	0 is returned upon failure.
//!
//!
//! method array(string) post_url_nice(string|Standards.URI url,mapping query_variables)
//! method array(string) post_url_nice(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method array(string) post_url_nice(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! method string post_url_data(string|Standards.URI url,mapping query_variables)
//! method string post_url_data(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method string post_url_data(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! method object(Protocols.HTTP.Query) post_url(string|Standards.URI url,mapping query_variables)
//! method object(Protocols.HTTP.Query) post_url(string|Standards.URI url,mapping query_variables,mapping request_headers)
//! method object(Protocols.HTTP.Query) post_url(string|Standards.URI url,mapping query_variables,mapping request_headers,object(Protocols.HTTP.Query) query)
//! 	Similar to the <ref>get_url</ref> class of functions, except that the
//!	query variables is sent as a post request instead of a get.
//!

object do_method(string method,
		 string|Standards.URI url,
		 void|mapping query_variables,
		 void|mapping request_headers,
		 void|Protocols.HTTP.Query con, void|string data)
{
  if(!con) {
    con = Protocols.HTTP.Query();
  }
  if(!request_headers)
    request_headers = ([]);
  
  
  if(stringp(url))
    url=Standards.URI(url);
  
#if constant(SSL.sslfile) 	
  if(url->scheme!="http" && url->scheme!="https")
    error("Protocols.HTTP can't handle %O or any other protocols than HTTP or HTTPS\n",
	  url->scheme);
  
  con->https= (url->scheme=="https")? 1 : 0;
#else
  if(url->scheme!="http"	)
    error("Protocols.HTTP can't handle %O or any other protocol than HTTP\n",
	  url->scheme);
  
#endif
  

  if(!request_headers)
    request_headers = ([]);
  mapping default_headers = ([
    "user-agent" : "Mozilla/4.0 compatible (Pike HTTP client)",
    "host" : url->host ]);

  if(url->user || url->passwd)
    default_headers->authorization = "Basic "
				   + MIME.encode_base64(url->user + ":" +
							(url->password || ""));
  request_headers = default_headers | request_headers;

  string query=url->query;
  if(query_variables && sizeof(query_variables))
  {
    if(query)
      query+="&"+http_encode_query(query_variables);
    else
      query=http_encode_query(query_variables);
  }

  string path=url->path;
  if(path=="") path="/";


  con->sync_request(url->host,url->port,
		    method+" "+path+(query?("?"+query):"")+" HTTP/1.0",
		    request_headers, data);
  
  if (!con->ok) return 0;
  return con;

}

object get_url(string|Standards.URI url,
	       void|mapping query_variables,
	       void|mapping request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("GET", url, query_variables, request_headers, con);
}

object put_url(string|Standards.URI url,
	       void|string file,
	       void|mapping query_variables,
	       void|mapping request_headers,
	       void|Protocols.HTTP.Query con)
{
  return do_method("PUT", url, query_variables, request_headers, con);
}

object delete_url(string|Standards.URI url,
		  void|mapping query_variables,
		  void|mapping request_headers,
		  void|Protocols.HTTP.Query con)
{
  return do_method("DELETE", url, query_variables, request_headers, con);
}

array(string) get_url_nice(string|Standards.URI url,
			   void|mapping query_variables,
			   void|mapping request_headers,
			   void|Protocols.HTTP.Query con)
{
  object c = get_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

string get_url_data(string|Standards.URI url,
		    void|mapping query_variables,
		    void|mapping request_headers,
		    void|Protocols.HTTP.Query con)
{
  object z = get_url(url, query_variables, request_headers, con);
  return z && z->data();
}

object post_url(string|Standards.URI url,
		mapping query_variables,
		void|mapping request_headers,
		void|Protocols.HTTP.Query con)
{
  return do_method("POST", url, 0,
		   (request_headers||([]))|
		   (["content-type":
		     "application/x-www-form-urlencoded"]),
		   con,
		   http_encode_query(query_variables));
}

array(string) post_url_nice(string|Standards.URI url,
			    mapping query_variables,
			    void|mapping request_headers,
			    void|Protocols.HTTP.Query con)
{
  object c = post_url(url, query_variables, request_headers, con);
  return c && ({ c->headers["content-type"], c->data() });
}

string post_url_data(string|Standards.URI url,
		     mapping query_variables,
		     void|mapping request_headers,
		     void|Protocols.HTTP.Query con)
{
  object z = post_url(url, query_variables, request_headers, con);
  return z && z->data();
}

//!
//! method string unentity(string s)
//!	Helper function for replacing HTML entities
//!	with the corresponding iso-8859-1 characters.
//! note:
//! 	All characters isn't replaced, only those with
//!	corresponding iso-8859-1 characters.

string unentity(string s)
{
   return replace(
      s,

      ({"&AElig;", "&Aacute;", "&Acirc;", "&Agrave;", "&Aring;", "&Atilde;",
	"&Auml;", "&Ccedil;", "&ETH;", "&Eacute;", "&Ecirc;", "&Egrave;",
	"&Euml;", "&Iacute;", "&Icirc;", "&Igrave;", "&Iuml;", "&Ntilde;",
	"&Oacute;", "&Ocirc;", "&Ograve;", "&Oslash;", "&Otilde;", "&Ouml;",
	"&THORN;", "&Uacute;", "&Ucirc;", "&Ugrave;", "&Uuml;", "&Yacute;",
	"&aacute;", "&acirc;", "&aelig;", "&agrave;", "&apos;", "&aring;",
	"&ast;", "&atilde;", "&auml;", "&brvbar;", "&ccedil;", "&cent;",
	"&colon;", "&comma;", "&commat;", "&copy;", "&deg;", "&dollar;",
	"&eacute;", "&ecirc;", "&egrave;", "&emsp;", "&ensp;", "&equals;",
	"&eth;", "&euml;", "&excl;", "&frac12;", "&frac14;", "&frac34;",
	"&frac18;", "&frac38;", "&frac58;", "&frac78;", "&gt;", "&gt",
	"&half;", "&hyphen;", "&iacute;", "&icirc;", "&iexcl;", "&igrave;",
	"&iquest;", "&iuml;", "&laquo;", "&lpar;", "&lsqb;", "&lt;", "&lt",
	"&mdash;", "&micro;", "&middot;", "&nbsp;", "&ndash;", "&not;",
	"&ntilde;", "&oacute;", "&ocirc;", "&ograve;", "&oslash;", "&otilde;",
	"&ouml;", "&para;", "&percnt;", "&period;", "&plus;", "&plusmn;",
	"&pound;", "&quest;", "&quot;", "&raquo;", "&reg;", "&rpar;",
	"&rsqb;", "&sect;", "&semi;", "&shy;", "&sup1;", "&sup2;", "&sup3;",
	"&szlig;", "&thorn;", "&tilde;", "&trade;", "&uacute;", "&ucirc;",
	"&ugrave;", "&uuml;", "&yacute;", "&yen;", "&yuml;", "&verbar;",
	"&amp;"}),

      ({ "Æ", "Á", "Â", "À", "Å", "Ã", "Ä", "Ç", "Ð", "É", "Ê", "È",
	 "Ë", "Í", "Î", "Ì", "Ï", "Ñ", "Ó", "Ô", "Ò", "Ø", "Õ", "Ö", "Þ", "Ú",
	 "Û", "Ù", "Ü", "Ý", "á", "â", "æ", "à", "&apos;", "å", "&ast;", "ã",
	 "ä", "¦", "ç", "¢", ":", ",", "&commat;", "©", "°",
	 "$", "é", "ê", "è", "&emsp;", "&ensp;", "&equals;", "ð", "ë",
	 "!", "½", "¼", "¾", "&frac18;", "&frac38;", "&frac58;",
	 "&frac78;", ">", ">", "&half;", "&hyphen;", "í", "î", "¡", "ì", "¿",
	 "ï", "«", "(", "&lsqb;", "<", "<", "&mdash;", "µ", "·", "",
	 "&ndash;", "¬", "ñ", "ó", "ô", "ò", "ø", "õ", "ö", "¶", "%",
	 ".", "+", "±", "£", "?", "\"", "»", "®", ")",
	 "&rsqb;", "§", "&semi;", "­", "¹", "²", "³", "ß", "þ", "~",
	 "&trade;", "ú", "û", "ù", "ü", "ý", "¥", "ÿ", "&verbar;", "&", }),
      );
}

//! method string http_encode_query(mapping variables)
//!	Encodes a query mapping to a string;
//!	this protects odd - in http perspective - characters
//!	like '&' and '#' and control characters,
//!	and packs the result together in a HTTP query string.
//!
//!	Example:
//!	<pre>
//!	&gt; Protocols.HTTP.http_encode_query( (["anna":"eva","lilith":"blue"]) );  
//!     Result: "lilith=blue&anna=eva"
//!     &gt; Protocols.HTTP.http_encode_query( (["&amp;":"&","'=\"":"\0\0\0"]) );  
//!     Result: "%26amp%3b=%26&%27%3d%22=%00%00%00"
//!	</pre>


string http_encode_query(mapping(string:int|string|array(string)) variables)
{
   return Array.map((array)variables,
		    lambda(array(string|int|array(string)) v)
		    {
		       if (intp(v[1]))
			  return http_encode_string(v[0]);
		       if (arrayp(v[1]))
			 return map(v[1], lambda (string val) {
					    return 
					      http_encode_string(v[0])+"="+
					      http_encode_string(val);
					  })*"&";
		       return http_encode_string(v[0])+"="+
			 http_encode_string(v[1]);
		    })*"&";
}

//! method string http_encode_string(string in)
//!	This protects all odd - see <ref>http_encode_query</ref> - 
//!	characters for transfer in HTTP.
//!
//!	Do not use this function to protect URLs, since
//!	it will protect URL characters like '/' and '?'.

string http_encode_string(string f)
{
   return replace(
      f, 
      ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007", 
	 "\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017", 
	 "\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027", 
	 "\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037", 
	 "\200", "\201", "\202", "\203", "\204", "\205", "\206", "\207", 
	 "\210", "\211", "\212", "\213", "\214", "\215", "\216", "\217", 
	 "\220", "\221", "\222", "\223", "\224", "\225", "\226", "\227", 
	 "\230", "\231", "\232", "\233", "\234", "\235", "\236", "\237", 
	 " ", "%", "'", "\"", "+", "&", "=", "/", 
	 "#", ";", "\\", "<", ">" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",    
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", 
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", 
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f", 
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", 
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f", 
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", 
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f", 
	 "%20", "%25", "%27", "%22", "%2b", "%26", "%3d", "%2f", 
	 "%23", "%3b", "%5c", "%3c", "%3e"}));
}

string http_encode_cookie(string f)
{
   return replace(
      f, 
      ({ "\000", "\001", "\002", "\003", "\004", "\005", "\006", "\007", 
	 "\010", "\011", "\012", "\013", "\014", "\015", "\016", "\017", 
	 "\020", "\021", "\022", "\023", "\024", "\025", "\026", "\027", 
	 "\030", "\031", "\032", "\033", "\034", "\035", "\036", "\037", 
	 "\200", "\201", "\202", "\203", "\204", "\205", "\206", "\207", 
	 "\210", "\211", "\212", "\213", "\214", "\215", "\216", "\217", 
	 "\220", "\221", "\222", "\223", "\224", "\225", "\226", "\227", 
	 "\230", "\231", "\232", "\233", "\234", "\235", "\236", "\237", 
	 " ", "%", "'", "\"", ",", ";", "=" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",    
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", 
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", 
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f", 
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", 
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f", 
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", 
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f", 
	 "%20", "%25", "%27", "%22", "%2c", "%3b", "%3d" }));
}


