// Bittorrent client - originally by Mirar
#pike __REAL_VERSION__
#require constant(Protocols.Bittorrent.Torrent)

constant dont_dump_program = 1;

object(Stdio.File)|zero fd = Stdio.File();

.Torrent parent;

string ip;
int port;
string id;
string client_version="unknown";

string mode;
int online=0;
int strangled=0; // choke because of tit-for-tat
int were_choking=1;
int were_interested=0;
int peer_choking=1;
int peer_interested=0;
int peer_choking_since=0;
int uploading=0;
multiset uploading_pieces=(<>);

string bitfield;
int is_complete=0;

// #define BT_PEER_DEBUG

constant KEEPALIVE_DELAY=60;
constant CONNECTION_TIMEOUT=30;
constant CONNECTION_DEAD_RETRY=-1;
constant CONNECTION_DROP_RETRY=30;
constant CONNECTION_HANDSHAKE_DROP_RETRY=-1;
constant GARB_DELAY=600;

// Assignments from BEP0004 http://bittorrent.org/beps/bep_0004.html

// Core Protocol
constant MSG_CHOKE          = 0x00;
constant MSG_UNCHOKE        = 0x01;
constant MSG_INTERESTED     = 0x02;
constant MSG_NOT_INTERESTED = 0x03;
constant MSG_HAVE           = 0x04;
constant MSG_BITFIELD       = 0x05;
constant MSG_REQUEST        = 0x06;
constant MSG_PIECE          = 0x07;
constant MSG_CANCEL         = 0x08;

// DHT Extension
constant MSG_PORT           = 0x09;

// Fast Extension
constant MSG_SUGGEST        = 0x0D;
constant MSG_HAVE_ALL       = 0x0E;
constant MSG_HAVE_NONE      = 0x0F;
constant MSG_REJECT_REQUEST = 0x10;
constant MSG_ALLOWED_FAST   = 0x11;

constant MSG_LTEP_HANDSHAKE = 0x14;

// Hash Transfer Protocol
constant MSG_HASH_REQUEST   = 0x15;
constant MSG_HASHES         = 0x16;
constant MSG_HASH_REJECT    = 0x17;

typedef int(0..8) message_id;

constant msg_to_string=([
   MSG_CHOKE:"choke",
   MSG_UNCHOKE:"unchoke",
   MSG_INTERESTED:"interested",
   MSG_NOT_INTERESTED:"not_interested",
   MSG_HAVE:"have",
   MSG_BITFIELD:"bitfield",
   MSG_REQUEST:"request",
   MSG_PIECE:"piece",
   MSG_CANCEL:"cancel",
]);

function(string, __unknown__ ...:void|mixed) warning = werror;
int cancelled=0;

int bandwidth_out=0; // bytes/s
int bandwidth_in=0;  // bytes/s
array(int) bandwidth_out_a=({});
array(int) bandwidth_in_a=({});

int bytes_in=0;
int bytes_out=0;

constant BANDWIDTH_O_METER_DELAY=0.5;
constant BANDWIDTH_O_METER_C=(int)(20/BANDWIDTH_O_METER_DELAY);

// ----------------------------------------------------------------

// created with a peer-info dictionary
protected void create(.Torrent _parent,mapping m)
{
   parent=_parent;
   ip=m->ip;
   port=m->port;
   id=m["peer id"]||"?";
   if (id!="?")
   {
      client_version=.PeerID.identify_peer(id);
//       werror("%O is %O is %O (out)\n",ip,id,client_version);
   }
//    else
//       werror("incoming from %O\n",ip);

   _status("created");

   warning=parent->warning;
   bitfield="\0"*strlen(parent->all_pieces_bits);

   if (m->fd)
   {
      fd=m->fd;

      bandwidth_in_a=({});
      bandwidth_out_a=({});

      online=1;
      fd->set_nonblocking(peer_read,0,peer_close);

      parent->peers_ordered+=({this});

      _status("incoming");

      call_out(connection_timeout,CONNECTION_TIMEOUT);
   }
}

private protected inline void _status(string type,void|string|int data)
{
#ifdef BT_PEER_DEBUG
   werror("%O: %O(%O)\n",ip,type,data);
#endif

   mode=type;
   Function.call_callback(parent->peer_update_status,type,data);
   status(type,data);
}

#ifdef BT_TRAFFIC_DEBUG
int traffic_start=time(1);
float traffic_last_choke=time(traffic_start);
void traffic_debug(string fmt,mixed...args)
{
   fmt=sprintf(fmt,@args);
   int t=time();
   mapping tm=localtime(t);
   werror("%02d:%02d:%02d %7.1f %6.1f %-15s %-20s %s",
	  tm->hour,tm->min,tm->sec,
	  time(traffic_start),
	  time(traffic_start)-traffic_last_choke,
	  ip,
	  client_version,
	  fmt);
}
#endif

//! Returns true if we can connect to this peer,
//! when new or disconnected but not fatally.
int is_connectable()
{
   return !online && mode!="dead" && mode!="failed";
}

//! Returns true if this peer is online and connected.
int is_online()
{
   return !!online;
}

//! Returns true if this peer is completed, as in
//! has downloaded everything already - and we
//! shouldn't need to upload to get stuff.
int is_completed()
{
   return is_complete;
}

//! Returns true if this peer is available, as in
//! we can use it to download stuff.
int is_available()
{
   return !peer_choking && online==2 &&
      (handover || !sizeof(piece_callback));
}

//! Returns true if this peer is activated, as in
//! we're downloading from it.
int is_activated()
{
   return !!sizeof(piece_callback);
}

//! Returns true if this peer is strangled;
//! as in we don't want to upload more, because we're not
//! getting any back.
int is_strangled()
{
   return strangled;
}

//! Returns true if this peer is choking, as in
//! doesn't send more data to us.
int is_choked()
{
   return peer_choking;
}

//! Returns as multiset what this peer is downloading.
multiset(int) downloading_pieces()
{
   return (multiset)(array(int))indices(piece_callback);
}

//! Connect to the peer; this is done async.
//! status/mode will change from @expr{"connecting"@} to
//! @expr{"dead"@} or to @expr{"connected"@} depending on result.
//! Will throw error if already online.
//!
//! Upon connect, protocol will be initiated in choked mode.
//! When the protocol is up, status will change to @expr{"online"@}
//! (or @expr{"failed"@} if the handshake failed).
//!
void connect()
{
   if (online)
      error("Can't connect a peer that is already online.\n");

   _status("connecting");

   remove_call_out(connect); // just in case
   call_out(connection_timeout,CONNECTION_TIMEOUT);

   string ip2;
   sscanf(ip,"%[0-9.]",ip2);
   if (ip2==ip)
      connect2(ip);
   else
   {
      parent->dns_async->
	 host_to_ip(ip,
		    lambda(string name,string ip)
		    {
// 		       werror("%O = %O\n",name,ip);
		       connect2(ip);
		    });
   }
}

void connect2(string ip2)
{
   fd=Stdio.File();
   fd->async_connect(
      ip2,port,
      lambda(int success)
      {
	 if (success)
	 {
	    bandwidth_in_a=({});
	    bandwidth_out_a=({});

	    online=1;
	    peer_connected();
	    _status("connected");
	 }
	 else
	 {
	    remove_call_out(connection_timeout);
	    drop("dead","failed");
	    if (CONNECTION_DEAD_RETRY>0)
	       call_out(connect,CONNECTION_DEAD_RETRY);
	 }
      });
}

void drop(string|zero how,void|string misc)
{
   online=0;
   if (how) _status(how,misc);

   catch { fd->set_blocking(); };
   catch { fd->close(); };
   destruct(fd);
   fd=0;

   remove_call_out(keepalive);
   remove_call_out(bandwidth_o_meter);

#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("-- connection dropped %O,%O\n",how,misc);
#endif

   map(Array.uniq(values(piece_callback)),
       lambda(function f) { catch { f(-1,0,"disconnected",this); }; });
   piece_callback=([]);

   catch { parent->peer_lost(this); };

   call_out(garb,GARB_DELAY);
}

void garb()
{
   if (online || find_call_out(connect)!=-1) return;
   parent->peers_ordered-=({this});
   parent->peers_unused-=({this});
   m_delete(parent->peers,id);
   destruct(this);
}

void connection_timeout()
{
   if (CONNECTION_DEAD_RETRY>0)
      call_out(connect,CONNECTION_DEAD_RETRY);
   drop("dead","timeout");
}

//! Disconnect a peer.
//! Does nothing if we aren't online.
//! status/mode will change to @expr{"disconnected"@},1 if we were online.
void disconnect()
{
   if (online)
   {
#ifdef BT_PEER_DEBUG
      werror("%O call disconnected %O\n",this,piece_callback);
#endif
      drop("disconnected","we");
   }
}

// initialize the connection
protected void peer_connected()
{
   fd->set_nonblocking(peer_read,0,peer_close);
   transmit("\23BitTorrent protocol%-8c%20s%20s",
	    0, // 8 reserved bytes
	    parent->info_sha1,
	    parent->my_peer_id);
}

protected private string sendbuf="";
protected private void transmit(string fmt,mixed ...args)
{
   if (sizeof(args)) fmt=sprintf(fmt,@args);

#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("-> %db: %O\n",
	  strlen(fmt),
	  strlen(fmt)>40?fmt[..29]+"..."+fmt[strlen(fmt)-10..]:fmt);
#endif

//    hexdump(fmt,0,0,"> ");

   if (sendbuf=="")
   {
      sendbuf=fmt;

      fd->set_write_callback(peer_write);
   }
   else
      sendbuf+=fmt;
}

array lastmsg=({-1,""});

void send_message(message_id n,string fmt,mixed ...args)
{
   fmt=sprintf("%c"+fmt,n,@args);
#ifdef BT_PEER_DEBUG
   werror("%O send message %d, %O\n",ip,n,sizeof(fmt));
#endif
#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("-> message %d (%s)\n",
	  n,msg_to_string[n]);
#endif


   lastmsg=({n,fmt});

   transmit("%4c%s",strlen(fmt),fmt);

// no need for a keepalive message for a while
   remove_call_out(keepalive);
   call_out(keepalive,KEEPALIVE_DELAY);
}

protected private void peer_write()
{
   for (;;)
   {
      if (sendbuf=="")
      {
#ifdef BT_PEER_DEBUG
	 werror("%O: should send piece? %d left - i:%O,o:%O - %O\n",ip,
		sizeof(queued_pieces),
		bytes_in,bytes_out,bytes_in-bytes_out);
#endif

	 if (were_choking || !sizeof(queued_pieces))
	 {
	    fd->set_write_callback(0);
	    return; // we shouldn't do anything
	 }

   // tit-for-tat
	 if (parent->do_we_strangle(this,bytes_in,bytes_out) &&
	     !parent->we_are_completed)
	 {
#ifdef BT_PEER_DEBUG
	    werror("%O doesn't give us enough, i:%d o:%d i-o:%O: choking\n",
		   ip,bytes_in,bytes_out,bytes_in-bytes_out);
#endif
	    strangle();
	    fd->set_write_callback(0);
	    return;
	 }

	 remove_call_out(fill_queue);
	 if (queued_pieces[0][3]==0)
	    fill_queue();
	 remove_call_out(fill_queue);
	 call_out(fill_queue,0.0001); // after this transmission

#ifdef BT_PEER_DEBUG
	 werror("%O: sending piece %d,%d,%db\n",ip,
		queued_pieces[0][0],
		queued_pieces[0][1],
		strlen(queued_pieces[0][3]));
#endif

	 send_message(MSG_PIECE,"%4c%4c%s",
		      queued_pieces[0][0],
		      queued_pieces[0][1],
		      queued_pieces[0][3]);
	 bytes_out+=strlen(queued_pieces[0][3]);

	 queued_pieces=queued_pieces[1..];
      }

      int i=fd->write(sendbuf);
      bandwidth_out_count+=i;
#ifdef BT_PEER_DEBUG
      werror("%O: wrote %d bytes: %O\n",ip,i,sendbuf[..min(40,i)]);
#endif
      sendbuf=sendbuf[i..];

      if (sendbuf!="")
      {
// 	 werror("%O: %d bytes left to send\n",ip,sizeof(sendbuf));
	 return;
      }
      else if (!sizeof(queued_pieces))
      {
	 fd->set_write_callback(0);
	 uploading=0,uploading_pieces=(<>);
	 return;
      }
      if (were_choking)
	 werror("%O: choking but uploading?\n",ip);
   }
}

protected private string readbuf="";
protected private void peer_read(mixed dummy,string s)
{
   remove_call_out(peer_read_timeout);
   call_out(peer_read_timeout,300);

   bandwidth_in_count+=strlen(s);
#ifdef BT_PEER_DEBUG
   werror("%O: read %d bytes: %O\n",
	  ip,strlen(s),s[..min(100,strlen(s))]);
#endif
   readbuf+=s;
   if (mode=="connected" || mode=="incoming")
   {
      if (readbuf[..min(strlen(readbuf)-1,19)] !=
	  "\23BitTorrent protocol"[..min(strlen(readbuf)-1,19)])
      {
#ifdef BT_TRAFFIC_DEBUG
	 traffic_debug("<- %db: %O\n",
		       strlen(readbuf),
		       strlen(readbuf)>40?readbuf[..29]+"..."+
		       readbuf[strlen(readbuf)-10..]:readbuf);
#endif
	 drop("failed","not bittorrent");

	 warning("got non-bittorrent connection from %O (%s %s): %O\n",
		 ip,
		 mode,
		 client_version,
		 readbuf[..40]);

	 return;
      }

      if (strlen(readbuf)<68) return; // wait for more data

#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("<- %db: %O\n",
	  strlen(readbuf),
	  strlen(readbuf)>40?readbuf[..29]+"..."+readbuf[strlen(readbuf)-10..]:readbuf);
#endif

      if (readbuf[28..47]!=parent->info_sha1)
      {
	 drop("failed","torrent metainfo mismatch");
	 return;
      }
      if (id=="?")
      {
	 id=readbuf[48..67];
	 client_version=.PeerID.identify_peer(id);
// 	 werror("(inc) %O is %O is %O\n",ip,id,client_version);

	 if (parent->peers[id]) // no cheating by reconnecting :)
	 {
	    bytes_in=parent->peers[id]->bytes_in;
	    bytes_out=parent->peers[id]->bytes_out;

#if 1
	    parent->peers[id]->disconnect();

	    warning("%O (%s): disconnected old connection, we got a new\n",
		    ip,client_version);
#endif
	 }

	 parent->peers[id]=this;
      }
      else if (readbuf[48..67]!=id)
      {
	 drop("failed","peer id mismatch");
	 return;
      }

      if (mode=="incoming")
	 peer_connected(); // send handshake

      readbuf=readbuf[68..];
      call_out(keepalive,KEEPALIVE_DELAY);
      bandwidth_o_meter();

      send_message(MSG_BITFIELD,
		   "%s",parent->file_got_bitfield());
// 	 if (!parent->default_is_choked)
// 	    send_message(MSG_UNCHOKE,"");

      if (were_interested)
	 send_message(MSG_INTERESTED,"");
      if (!were_choking)
	 send_message(MSG_UNCHOKE,"");


      if (-1==search(parent->peers_ordered,this))
	 parent->peers_ordered+=({this});
      parent->peers_unused-=({this});


      online=2;
      _status("online");
      remove_call_out(connection_timeout);
   }

   if (mode=="online")
   {
      while (strlen(readbuf)>3)
      {
	 int n;
	 sscanf(readbuf,"%4c",n);
	 if (strlen(readbuf)>=4+n)
	 {
#ifdef BT_TRAFFIC_DEBUG
	    traffic_debug("<- %db: %O\n",
		   4+n,
		   4+n>40?readbuf[..29]+"..."+
			  readbuf[strlen(readbuf)-10..]:readbuf[..3+n]);
#endif

	    string msg=readbuf[4..3+n];
	    readbuf=readbuf[4+n..];
	    got_message_from_peer(msg);
	 }
	 else break; // wait for more
      }
   }
}

// no action for 5 minutes, drop and reconnect
void peer_read_timeout()
{
#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("-- timeout, reconnecting\n");
#endif

   disconnect();
   fd=Stdio.File();
   connect();
}

void got_message_from_peer(string msg)
{
   if (msg=="")
   {
#ifdef BT_PEER_DEBUG
      werror("%O: got keepalive message\n",ip);
#endif
      return;
   }
#ifdef BT_PEER_DEBUG
   werror("%O: got message type %d: %O (%d bytes)\n",
	  ip,msg[0],msg[1..40],strlen(msg));
#endif

   int n,o,l;
   string data;

#ifdef BT_TRAFFIC_DEBUG
   traffic_debug("<- - got message %d (%s)\n",
	  msg[0],msg_to_string[msg[0]]);
#endif

   switch (msg[0])
   {
      case MSG_CHOKE:
	 if (!peer_choking)
	 {
	    peer_choking=1;
	    peer_choking_since=time(1);

#ifdef BT_PEER_DEBUG
 	    werror("%O choke %O\n",this,piece_callback,this);
#endif

	    Array.uniq(values(piece_callback))
	       (-1,0,"choked",this); // choked
	 }
	 _status("online","choked");
	 break;
      case MSG_UNCHOKE:
	 if (peer_choking)
	 {
	    peer_choking=0;
#ifdef BT_PEER_DEBUG
 	    werror("%O unchoke %O\n",this,piece_callback);
#endif
	    Array.uniq(values(piece_callback))
	       (-1,0,"unchoked",this); // choked

	    parent->peer_unchoked(this);
	 }
	 _status("online","unchoked");
	 break;
      case MSG_INTERESTED:
	 peer_interested=1;
	 _status("online","interested");

	 if (were_choking)
	    if (sizeof(parent->file_got)>sizeof(parent->file_want))
	       ;
	    else
	    {
	       Function.call_callback(
		  warning,
		  "%O (%s): sent interested but we have none "
		  "(want=%d got=%d)\n",
		  ip,client_version,
		  sizeof(parent->file_got),sizeof(parent->file_want)
				      );
	    }
	 if (!strangled) unchoke();
	 break;
      case MSG_NOT_INTERESTED:
	 peer_interested=0;
	 _status("online","uninterested");
	 break;
      case MSG_HAVE:
	 sscanf(msg,"%*c%4c",n);
	 bitfield[n/8]=bitfield[n/8]|(128>>(n&7));
	 is_complete=(bitfield==parent->all_pieces_bits);

	 if (is_complete && !sizeof(parent->file_want))
	    disconnect(); // we're complete, they're complete
	 else if (!parent->file_available[n])
	    parent->peer_have(this,n);

	 break;
      case MSG_BITFIELD:
	 bitfield=msg[1..];
	 is_complete=(bitfield==parent->all_pieces_bits);
	 parent->peer_gained(this);

	 if (is_complete && !sizeof(parent->file_want))
	    disconnect(); // we're complete, they're complete
	 break;
      case MSG_REQUEST:
	 if (sscanf(msg,"%*c%4c%4c%4c",n,o,l)<4)
	 {
	    Function.call_callback(
	       warning,"%O (%s): got illegal piece message, "
	       "too short (%d bytes)\n",
	       ip,client_version,msg[0],strlen(msg));
	    return;
	 }
	 else
	    queue_piece(n,o,l);
	 break;
      case MSG_PIECE:
	 if (sscanf(msg,"%*c%4c%4c%s",n,o,data)<4)
	 {
	    Function.call_callback(
	       warning,"%O (%s): got illegal piece message, "
	       "too short (%d bytes)\n",
	       ip,client_version,msg[0],strlen(msg));
	    return;
	 }
	 got_piece_from_peer(n,o,data);
	 break;
      case MSG_CANCEL:
	 queued_pieces=({});
	 break;
      default:
	 Function.call_callback(
	    warning,"%O (%s): got unknown message type %d, %d bytes\n",
	    ip,client_version,msg[0],strlen(msg));
   }
}

void peer_close()
{
   parent->peer_lost(this);

   remove_call_out(keepalive);
   remove_call_out(bandwidth_o_meter);

   drop(0);
   if (online)
   {
      if (online==2)
      {
	 online=0;
#ifdef BT_PEER_DEBUG
	 werror("%O call disconnected %O\n",this,piece_callback);
#endif
	 Array.uniq(values(piece_callback))
	    (-1,0,"disconnected",this);
	 piece_callback=([]);
      }
      else
	 online=0;

      if (mode=="connected")
      {
	 _status("failed","remote host closed connection in handshake");

	 if (CONNECTION_HANDSHAKE_DROP_RETRY>0)
	    call_out(connect,CONNECTION_HANDSHAKE_DROP_RETRY);
      }
      else
      {
// 	 werror("%O disconnected: last message was %d:%O\n",
// 		ip,@lastmsg);

	 if (sizeof(parent->file_want)) // reconnect, we want more stuf
	 {
	    parent->peers_unused+=({this});
	    parent->peers_ordered-=({this});
	 }

	 _status("disconnected","peer");
      }
   }
}

protected private void keepalive()
{
   call_out(keepalive,KEEPALIVE_DELAY);
#ifdef BT_PEER_DEBUG
   werror("%O: sending keepalive\n",ip);
#endif
   transmit("\0\0\0\0"); // keepalive empty message
}

//! Send a have message to tell I now have piece n.
//! Ignored if not online.
void send_have(int n)
{
   if (online<2) return;
   send_message(MSG_HAVE,"%4c",n);
}

// ----------------------------------------------------------------
mapping(string:function(int,int,string,object:void|mixed)) piece_callback=([]);
int handover=0; // more downloads is ok

//! Called to request a chunk from this peer.
void request(int piece,int offset,int bytes,
	     function(int,int,string,object:void|mixed) callback)
{
   if (!were_interested)
      show_interest();

//    foreach (map(values(piece_callback),function_object)->peer;;.Peer p)
//       if (p!=this)
//       {
// 	 werror("%O: funny thing in queue: %O\n",
// 		this,piece_callback);
// 	 exit(1);
//       }

   piece_callback[piece+":"+offset+":"+bytes]=callback;
   send_message(MSG_REQUEST,
		"%4c%4c%4c",piece,offset,bytes);
   handover=0;
}

void got_piece_from_peer(int piece,int offset,string data)
{
   function(int,int,string,object:void|mixed) f;
   string s;
   if ((f=piece_callback[s=piece+":"+offset+":"+strlen(data)]))
   {
      m_delete(piece_callback,s);
#ifdef BT_PEER_DEBUG
      werror("%O got piece %O\n",this,piece);
#endif
      f(piece,offset,data,this);
   }
   else if (cancelled)
      cancelled--;
   else
   {
      Function.call_callback(
	 warning,"%O (%s): got unrequested piece %d/%d+%db\n",
	 ip,client_version,piece,offset,strlen(data));
   }

   bytes_in+=strlen(data);
   if (strangled &&
       !parent->do_we_strangle(this,bytes_in,bytes_out))
   {
#ifdef BT_PEER_DEBUG
      werror("%O gave us enough, i:%d o:%d i-o:%O: unchoking\n",
	     ip,bytes_in,bytes_out,bytes_in-bytes_out);
#endif
      unstrangle();
   }
}

void cancel_requests(int real)
{
   if (real) send_message(MSG_CANCEL,"");
   if (readbuf!="") cancelled=1; // expect one more without complaint
   piece_callback=([]);

#ifdef BT_PEER_DEBUG
   werror("%O cancel_requests\n",this);
#endif
}

// ----------------------------------------------------------------

array(array(int|string)) queued_pieces=({});

protected void queue_piece(int piece,int offset,int length)
{
   if (were_choking)
   {
#if 0
// lots of clients to this, so don't warn
      Function.call_callback(
	 warning,
	 "%O (%s): got request for a piece while we're choking\n",
	 ip,client_version);
#endif
      return;
   }

   if (length>524288 ||
       length>parent->info["piece length"])
   {
      Function.call_callback(
	 warning,
	 "%O (%s): got request for a too large chunk, %d bytes - ignored\n",
	 ip,client_version,length);
      return;
   }

#ifdef BT_PEER_DEBUG
   werror("%O: request for %d:o %d %db: %s\n",
	  ip,piece,offset,length,
	  parent->file_want[piece]?"we don't have it??!":"we have it");
#endif

   if (parent->file_want[piece])
   {
      warning("%O (%s) requested piece %d, but we don't have it?\n",
	      ip,client_version,piece);
      return;
   }

   queued_pieces+=({({piece,offset,length,0})});
   if (queued_pieces[0][3]==0)
   {
      remove_call_out(fill_queue);
      fill_queue();
   }

   if (sendbuf=="")
      fd->set_write_callback(peer_write);
}

protected void fill_queue()
{
   if (!sizeof(queued_pieces) ||
	  queued_pieces[0][3]!=0)
      {
	 uploading=0;
	 uploading_pieces=(<>);
	 return;
      }
   queued_pieces[0][3]=parent->get_piece_chunk(@queued_pieces[0][..2]);

   if (!uploading ||
       !uploading_pieces[queued_pieces[0][0]])
   {
      uploading=1;
      uploading_pieces=(<queued_pieces[0][0]>);

      Function.call_callback(parent->downloads_update_status);
   }
}

void unchoke()
{
   werror("%O (%s): unchoke\n",ip,client_version);

   were_choking=0;
   send_message(MSG_UNCHOKE,"");

//    if (queued_pieces && sendbuf=="") // wont happen (transmit above)
//       fd->set_write_callback(peer_write);
   _status("online","unchoking");
}

void choke()
{
//    werror("%O (%s): choke\n",ip,client_version);
#ifdef BT_TRAFFIC_DEBUG
   traffic_last_choke=time(traffic_start);
#endif

   send_message(MSG_CHOKE,"");
   were_choking=1;
   _status("online","choking");

// if choke means "I'll drop all pieces in the queue"
#if 1
   queued_pieces=({});
   uploading_pieces=(<>);
   uploading=0;
#endif
}

protected int last_strangle;

void strangle()
{
   strangled=1;
   choke();
   last_strangle=time(1);
}

void unstrangle()
{
   if (time(1)-last_strangle<10)
   {
      void unstrangle2() { if (strangled) unstrangle(); };
      remove_call_out(unstrangle2);
      call_out(unstrangle2,time(1)+10-last_strangle);
   }
   else
   {
      strangled=0;
      unchoke();
   }
}

void show_interest()
{
#ifdef BT_PEER_DEBUG
   werror("%O: interested\n",ip);
#endif
   send_message(MSG_INTERESTED,"");
   were_interested=1;
}

void show_uninterest()
{
#ifdef BT_PEER_DEBUG
   werror("%O: uninterested\n",ip);
#endif
   send_message(MSG_NOT_INTERESTED,"");
   were_interested=0;
}

// ----------------------------------------------------------------
// stubs

//! Called whenever there is a status change for this peer.
//! Always called with @expr{"created"@} when the object is created.
//!
//! Does not need to call inherited function.
void status(string type,void|int|string data)
{
}

// ----------------------------------------------------------------

protected string|zero _sprintf(int t)
{
   if (t=='O')
      return sprintf("Bittorrent.Peer(%s:%d %O%s%s%s%s%s%s%s v%d ^%d b/s q:%d p:%d sb:%d)",
		     ip,port,mode,
		     is_activated()?" dl":"",
		     uploading?" ul":"",
		     is_complete?" complete":"",
		     peer_choking?" choking":"",
		     strangled?" strangled":"",
		     is_activated()?" activated":"",
		     handover?" handover":"",
		     bandwidth_in,bandwidth_out,
		     sizeof(queued_pieces),
		     sizeof(piece_callback),
		     sizeof(sendbuf));
   return 0;
}

protected void _destruct()
{
   drop(0);
}

// ----------------------------------------------------------------

protected private int bandwidth_in_count=0;
protected private int bandwidth_out_count=0;
protected private int bandwidth_t0=time(1);
protected private float bandwidth_t=time(bandwidth_t0);

void bandwidth_o_meter()
{
   call_out(bandwidth_o_meter,BANDWIDTH_O_METER_DELAY);

   float t1=time(bandwidth_t0);
   float t=t1-bandwidth_t;
   if (t<0.001) return;
   bandwidth_in_a+=({(int)(bandwidth_in_count/t)});
   bandwidth_out_a+=({(int)(bandwidth_out_count/t)});
   if (sizeof(bandwidth_in_a)>BANDWIDTH_O_METER_C)
   {
      bandwidth_in_a=bandwidth_in_a[1..];
      bandwidth_out_a=bandwidth_out_a[1..];
   }

   bandwidth_in=Array.sum(bandwidth_in_a)/sizeof(bandwidth_in_a);
   bandwidth_out=Array.sum(bandwidth_out_a)/sizeof(bandwidth_out_a);

   bandwidth_in_count=0;
   bandwidth_out_count=0;
   bandwidth_t=t1;
}
