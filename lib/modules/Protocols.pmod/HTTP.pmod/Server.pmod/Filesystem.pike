// example server implementation user the Port server backend

inherit Protocols.HTTP.Server.Port;

Filesystem.Base fs;

string serverid=version()+": Filesystem HTTP Server";

void create(Filesystem.Base _fs,
	    void|int portno,
	    void|string interface)
{
   fs=_fs;
   ::create(got_request,portno,interface);
}

void got_request(Protocols.HTTP.Server.Request rid)
{
   Filesystem.Stat st=fs->stat(rid->not_query);
   if (st && st->isreg())
   {
      Stdio.File f=fs->open(rid->not_query,"r");
      if (f)
      {
	 rid->response_and_finish(
	    (["file":f,
	      "server":serverid]));
	 return;
      }
   }

   rid->response_and_finish(
      (["error":404,
	"type":"text/html",
	"data":"<h1>404 File not found</h1>",
	"server":serverid]));
}
