
//! module Protocols
//! submodule HTTP
//! method object(Protocols.HTTP.Query) get_url(string url)
//! method object(Protocols.HTTP.Query) get_url(string url,mapping query_variables)
//! 	Sends a HTTP GET request to the server in the URL
//!	and returns the created and initialized <ref>Query</ref> object.
//!	0 is returned upon failure.
//!
//! method array(string) get_url_nice(string url)
//! method array(string) get_url_nice(string url,mapping query_variables)
//!	Returns an array of ({content_type,data})
//!	after calling the requested server for the information.
//!	0 is returned upon failure.

object get_url(string url,void|mapping query_variables)
{
   object con=master()->resolv("Protocols")["HTTP"]["Query"]();
   
   string prot="http",host;
   int port=80;
   string query;

   sscanf(url,"%[^:/]://%[^:/]:%d/%s",prot,host,port,query) == 4 ||
      (port=80,sscanf(url,"%[^:/]://%[^:/]/%s",prot,host,query)) == 3 ||
      (prot="http",sscanf(url,"%[^:/]:%d/%s",host,port,query)) == 3 ||
      (port=80,sscanf(url,"%[^:/]/%s",host,query)) == 2 ||
      (host=url,query="");

   if (prot!="http")
      error("Protocols.HTTP can't handle %O or any other protocol than HTTP\n",
	    prot);

   if (query_variables)
   {
      if (search(query,"?")!=-1)
	 query+="&"+http_encode_query(query_variables);
      else
	 query+="?"+http_encode_query(query_variables);
   }

   con->sync_request(host,port,
		     "GET /"+query+" HTTP/1.0",
		     (["user-agent":
		       "Mozilla/4.0 compatible (Pike HTTP client)"]));

   if (!con->ok) return 0;
   return con;
}

array(string) get_url_nice(string url,void|mapping query_variables)
{
   object c=get_url(url,query_variables);
   return c && ({c->headers["content-type"],c->data()});
}

array(string) get_url_result(string url,void|mapping query_variables)
{
   return get_url(url,query_variables)[1];
}

object post_url(string url,mapping query_variables)
{
   object con=master()->resolv("Protocols")["HTTP"]["Query"]();
   
   string prot="http",host;
   int port=80;
   string query;

   sscanf(url,"%[^:/]://%[^:/]:%d/%s",prot,host,port,query) == 4 ||
      (port=80,sscanf(url,"%[^:/]://%[^:/]/%s",prot,host,query)) == 3 ||
      (prot="http",sscanf(url,"%[^:/]:%d/%s",host,port,query)) == 3 ||
      (port=80,sscanf(url,"%[^:/]/%s",host,query)) == 2 ||
      (host=url,query="");

   if (prot!="http")
      error("Protocols.HTTP can't handle %O or any other protocol than HTTP\n",
	    prot);

   con->sync_request(host,port,
		     "POST /"+query+" HTTP/1.0",
		     (["user-agent":
		       "Mozilla/4.0 compatible (Pike HTTP client)",
		       "content-type":
		       "application/x-www-form-urlencoded"]),
		     http_encode_query(query_variables));

   if (!con->ok) return 0;
   return con;
}

array(string) post_url_nice(string url,mapping query_variables)
{
   object c=post_url(url,query_variables);
   return c && ({c->headers["content-type"],c->data()});
}

array(string) post_url_result(string url,mapping query_variables)
{
   return post_url(url,query_variables)[1];
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

string http_encode_query(mapping variables)
{
   return Array.map(indices(variables),
		    lambda(string ind)
		    {
		       return http_encode_string(ind)+"="+
			  http_encode_string(variables[ind]);
		    })*"&";
}

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
	 " ", "%", "'", "\"" }),
      ({ 
	 "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07",    
	 "%08", "%09", "%0a", "%0b", "%0c", "%0d", "%0e", "%0f", 
	 "%10", "%11", "%12", "%13", "%14", "%15", "%16", "%17", 
	 "%18", "%19", "%1a", "%1b", "%1c", "%1d", "%1e", "%1f", 
	 "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", 
	 "%88", "%89", "%8a", "%8b", "%8c", "%8d", "%8e", "%8f", 
	 "%90", "%91", "%92", "%93", "%94", "%95", "%96", "%97", 
	 "%98", "%99", "%9a", "%9b", "%9c", "%9d", "%9e", "%9f", 
	 "%20", "%25", "%27", "%22"}));
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
