
//! module Protocols
//! submodule HTTP
//! method object(Protocols.HTTP.Query) get_url(string url)
//! 	Sends a HTTP GET request to the server in the URL
//!	and returns the created and initialized <ref>Query</ref> object.
//!	0 is returned upon failure.
//!
//! method array(string) get_url_nice(string url)
//!	Returns an array of ({content_type,data})
//!	after calling the requested server for the information.
//!	0 is returned upon failure.

object get_url(string url)
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
      error("Protocols.HTTP can't handle %O or any other protocol then HTTP\n",
	    prot);

   con->sync_request(host,port,
		     "GET /"+query+" HTTP/1.0",
		     (["user-agent":
		       "Mozilla/4.0 compatible (Pike HTTP client)"]));

   if (!con->ok) return 0;
   return con;
}

array(string) get_url_nice(string url)
{
   object c=get_url(url);
   return c && ({c->headers["content-type"],c->data()});
}
