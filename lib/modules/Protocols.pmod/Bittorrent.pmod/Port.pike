// Bittorrent client - originally by Mirar 
#pike __REAL_VERSION__

Stdio.Port port;
int portno;
.Torrent parent;

//! bind a port for this Torrent
void create(.Torrent _parent)
{
   parent=_parent;
   int portno=parent->my_port;

   port=Stdio.Port();

   for (int i=0; i<100; i++)
      if (!port->bind(portno,new_connection))
      {
	 if (i<10)
	    portno++;
	 else
	    portno=random(65536-8192)+8192; // random high number
      }
      else
      {
	 parent->my_port=portno;
	 return;
      }

   error("Torrent: Failed to bind port for incoming traffic\n");
}

void destroy() { destruct(port); }

static void new_connection()
{
   Stdio.File fd=port->accept();
   
   array v=fd->query_address()/" ";

   parent->peer_program(
      parent,
      (["peer id":"?",
	"ip":v[0],
	"port":(int)v[1],
	"fd":fd ]) );
}
