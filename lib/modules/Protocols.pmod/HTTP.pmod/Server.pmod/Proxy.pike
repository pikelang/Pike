#pike __REAL_VERSION__

// example HTTP proxy implementation 

inherit Protocols.HTTP.Server.Port;

Filesystem.Base fs;

string serverid=version()+": HTTP Proxy";

string ourvia="Via: "+serverid+" ["+random(1<<128)->digits(16)+"]";


void create(void|int portno,
	    void|string interface)
{
   ::create(Proxify,portno,interface);
}

class Proxify
{
   class Query
   {
      inherit Protocols.HTTP.Query;

      int stillheader=1;

      void write_more()
      {
	 int i=rid->my_fd->write(buf);
// 	 werror("%s: wrote %d\n",rid->full_query,i);
	 buf=buf[i..];
	 if (!strlen(buf))
	    rid->my_fd->set_nonblocking(0,0,client_close);
      }

      void addmyvia()
      {
	 int i=search(buf,"\r\n\r\n");
	 if (i!=-1)
	 {
	    buf=buf[..i+1]+ourvia+"\r\n"+buf[i+2..];
	    stillheader=0;
	 }
      }

      void send_to_client(string s)
      {
	 if (!strlen(buf))
	 {
	    rid->my_fd->set_nonblocking(0,write_more,client_close);
	    buf=s;

	    stillheader && addmyvia();

	    write_more();
	 }
	 else buf+=s;
	 stillheader && addmyvia();
      }

      void client_close()
      {
// 	 werror("%s: client close\n",rid->full_query);
	 destruct(con);
	 destruct(this_object());
      }

      void async_close()
      {
// 	 werror("%s: server close\n",rid->full_query);
	 rid->finish(0);
      }

      void async_read(mixed dummy,string data)
      {
// 	 werror("%s: read %d\n",rid->full_query,strlen(data));
	 send_to_client(data);
      }
   }

   Query q=Query();
   Protocols.HTTP.Server.Request rid;

   void create(Protocols.HTTP.Server.Request _rid)
   {
      rid=_rid;
//       werror("%O\n",mkmapping(indices(rid),values(rid)));

      Standards.URI url;
// normal Proxy URL
      if (rid->not_query[..6]=="http://")
	 url=Standards.URI(rid->not_query);
// if we're transparent, we at least get a host header (hopefully)
      else if (rid->request_headers->host)
      {
	 url=Standards.URI("http://"+
			   rid->request_headers->host+"/"+rid->not_query);
      }
      else
      {
	 rid->response_and_finish( (["error":"500",
				     "type":"text/html",
				     "data":"500 Internal error (Proxy) "
				     "can't understand URL (not http?)"]) );
	 return;
      }
      
      string path=url->path;
      if(path=="") path="/";

      array v=rid->raw/"\r\n\r\n"+({"",""});
      v[0]=(v[0]/"\r\n")[1..]*"\r\n"; // not first line

      if (search(v[0],ourvia)!=-1)
      {
	 rid->response_and_finish( (["error":"500",
				     "type":"text/html",
				     "data":"500 Internal error (Proxy) "
				     "proxy is recursing"]) );
	 return;
      }
      if (v[0]!="") v[0]+="\r\n";
      v[0]+=ourvia+"\r\n";
      if (search(v[0],"Host:")==-1)
	 v[0]+="Host: "+url->host+":"+url->port+"\r\n";

//       werror("%O\n",v);

      q->async_request(url->host,url->port,
		       rid->request_type+" "+path+
		       ((rid->query!="")?("?"+rid->query):"")+" "+
		       "HTTP/1.0",
		       v[0],v[1]);
   }
}
