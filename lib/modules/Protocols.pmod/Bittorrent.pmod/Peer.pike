Stdio.File fd=Stdio.File();
.Torrent parent;

string ip;
int port;
string id;

string mode;
int online=0;
int choked=0;
int interested=0;
int peer_choked=1;
int peer_interested=0;

string bitfield;
int bandwidth_out=0; // bytes/s
int bandwidth_in=0;  // bytes/s

// #define BT_PEER_DEBUG

constant KEEPALIVE_DELAY=60;

constant MSG_CHOKE = 0;
constant MSG_UNCHOKE = 1;
constant MSG_INTERESTED = 2;
constant MSG_NOT_INTERESTED = 3;
constant MSG_HAVE = 4;
constant MSG_BITFIELD = 5;
constant MSG_REQUEST = 6;
constant MSG_PIECE = 7;
constant MSG_CANCEL = 8;
typedef int(0..8) message_id;

function(string,mixed...:void|mixed) warning=werror;

constant BANDWIDTH_O_METER_DELAY=0.5;

// ----------------------------------------------------------------

// created with a peer-info dictionary
void create(Torrent _parent,mapping m)
{
   parent=_parent;
   ip=m->ip;
   port=m->port;
   id=m["peer id"];

   _status("created");

   warning=parent->warning;
}

private static inline void _status(string type,void|string|int data)
{
#ifdef BT_PEER_DEBUG
   werror("%O: %O(%O)\n",ip,type,data);
#endif

   mode=type;
   status(type,data);
}

int is_connectable() 
{ 
   return !online && mode!="dead" && mode!="failed"; 
}

int is_online() 
{ 
   return !!online;
}

//! connect to the peer; this is done async. 
//! status/mode will change from "connecting" to
//! "dead" or to "connected" depending on result.
//! Will throw error if already online.
//!
//! Upon connect, protocol will be initiated in choked mode.
//! When the protocol is up, status will change to "online"
//! (or "failed" if the handshake failed).
//!
void connect()
{
   if (online)
      error("can't connect a peer that is already online\n");

   _status("connecting");

   fd->async_connect(
      ip,port,
      lambda(int success)
      {
	 if (success)
	 {
	    online=1;
	    peer_connected();
	    _status("connected");
	 }
	 else
	 {
	    online=0;
	    _status("dead");
	 }
      });
}

//! disconnect a peer
//! does nothing if we aren't online
//! status/mode will change to "disconnected",1 if we were online
void disconnect()
{
   if (online)
   {
      fd->close();
      online=0;
      Array.uniq(values(piece_callback))(0,0,0); // abort 
      piece_callback=([]);
      parent->peer_lost(this_object());
      remove_call_out(keepalive); 
      remove_call_out(bandwidth_o_meter); 
      _status("disconnected","we");
   }
}

// initialize the connection
static void peer_connected()
{
   fd->set_nonblocking(peer_read,0,peer_close);
   transmit("\23BitTorrent protocol%-8c%20s%20s",
	    0, // 8 reserved bytes
	    parent->info_sha1,
	    parent->my_peer_id);
}

static private string sendbuf="";
static private void transmit(string fmt,mixed ...args)
{
   if (sizeof(args)) fmt=sprintf(fmt,@args);

//    hexdump(fmt,0,0,"> ");

   if (sendbuf=="")
   {
      sendbuf=fmt;

      if (!choked) fd->set_write_callback(peer_write);
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

   lastmsg=({n,fmt});

   transmit("%4c%s",strlen(fmt),fmt);

// no need for a keepalive message for a while
   remove_call_out(keepalive); 
   call_out(keepalive,KEEPALIVE_DELAY);
}

static private void peer_write()
{
   if (sendbuf=="")
   {
      if (!sizeof(queued_pieces)) return; // huh?

      remove_call_out(fill_queue);
      if (queued_pieces[0][3]==0)
	 fill_queue();
      call_out(fill_queue,0.0001); // after this transmission

      send_message(MSG_PIECE,"%4c%4c%s",
		   queued_pieces[0][0],
		   queued_pieces[0][1],
		   queued_pieces[0][3]);
      queued_pieces=queued_pieces[1..];
   }

   int i=fd->write(sendbuf);
   bandwidth_out_count+=i;
#ifdef BT_PEER_DEBUG
   werror("%O: wrote %d bytes: %O\n",ip,i,sendbuf[..min(40,i)]);
#endif
   sendbuf=sendbuf[i..];
   
   if (sendbuf=="")
      fd->set_write_callback(0);
}

static private string readbuf="";
static private void peer_read(mixed dummy,string s)
{
   bandwidth_in_count+=strlen(s);
#ifdef BT_PEER_DEBUG
   werror("%O: read %d bytes: %O\n",
	  ip,strlen(s),s[..min(100,strlen(s))]);
#endif
   readbuf+=s;
   if (mode=="connected")
   {
      if (strlen(readbuf)<68) return; // wait for more data
      if (readbuf[..19]=="\23BitTorrent protocol")
      {
	 if (readbuf[28..47]!=parent->info_sha1)
	 {
	    fd->close();
	    online=0;
	    _status("failed","torrent metainfo mismatch");
	    return;
	 }
	 if (readbuf[48..67]!=id)
	 {
	    fd->close();
	    online=0;
	    _status("failed","peer id mismatch");
	    return;
	 }

	 readbuf=readbuf[68..];
	 call_out(keepalive,KEEPALIVE_DELAY);
	 bandwidth_o_meter();

	 send_message(MSG_BITFIELD, 
		      parent->file_got_bitfield());
	 if (!parent->default_is_choked)
	    send_message(MSG_UNCHOKE,""); 
	 send_message(MSG_INTERESTED,""); 

	 _status("online");
      }
   }

   if (mode=="online")
   {
      while (strlen(readbuf)>3)
      {
	 int n;
	 sscanf(readbuf,"%4c",n);
	 if (strlen(readbuf)>=4+n)
	 {
	    string msg=readbuf[4..3+n];
	    readbuf=readbuf[4+n..];
	    got_message_from_peer(msg);
	 }
	 else break; // wait for more
      }
   }
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
   werror("%O: got message type %d: %O (%d bytes)\n",ip,msg[0],msg[1..40],strlen(msg));
#endif

   int n,o,l;
   string data;

   switch (msg[0])
   {
      case MSG_CHOKE:
	 peer_choked=1;
	 if (sendbuf) fd->set_write_callback(0);
	 _status("online","choked");
	 break;
      case MSG_UNCHOKE:
	 peer_choked=0;
	 if (sendbuf) fd->set_write_callback(peer_write);
	 _status("online","unchoked");
	 break;
      case MSG_INTERESTED:
	 peer_interested=1;
	 _status("online","interested");
	 break;
      case MSG_NOT_INTERESTED:
	 peer_interested=0;
	 _status("online","uninterested");
	 break;
      case MSG_HAVE:
	 sscanf(msg,"%*c%4c",n);
	 if (!bitfield)
	 {
	    Function.call_callback(
	       warning,"%O gave us HAVE but no BITFIELD\n",ip);
	    break;
	 }
	 bitfield[n/8]=bitfield[n/8]|(1<<(n&7));

	 if (!parent->file_available[n])
	    parent->peer_have(this_object(),n);

	 break;
      case MSG_BITFIELD:
	 bitfield=msg[1..];
	 online=2;
	 parent->peer_gained(this_object());
	 break;
      case MSG_REQUEST:
	 if (sscanf(msg,"%*c%4c%4c%4c",n,o,l)<4)
	 {
	    Function.call_callback(
	       warning,"%O: got illegal piece message, too short (%d bytes)\n",
	       ip,msg[0],strlen(msg));
	    return;
	 }
	 else
	    queue_piece(n,o,l);
	 break;
      case MSG_PIECE:
	 if (sscanf(msg,"%*c%4c%4c%s",n,o,data)<4)
	 {
	    Function.call_callback(
	       warning,"%O: got illegal piece message, too short (%d bytes)\n",
	       ip,msg[0],strlen(msg));
	    return;
	 }
	 got_piece_from_peer(n,o,data);
	 break;
      case MSG_CANCEL:
	 queued_pieces=({});
	 break;
      default: 
	 Function.call_callback(
	    warning,"%O: got unknown message type %d, %d bytes\n",
	    ip,msg[0],strlen(msg));
   }
}

void peer_close()
{
   parent->peer_lost(this_object());

   remove_call_out(keepalive); 
   remove_call_out(bandwidth_o_meter); 

   destruct(fd);
   fd=Stdio.File();
   if (online)
   {
      if (online==2)
      {
	 Array.uniq(values(piece_callback))(0,0,0); // abort 
	 piece_callback=([]);
      }
      online=0;

      if (mode=="connected")
	 _status("failed","remote host closed connection in handshake");
      else
      {
// 	 werror("%O disconnected: last message was %d:%O\n",
// 		ip,@lastmsg);
	 _status("disconnected","peer");
      }
   }
}

static private void keepalive()
{
   call_out(keepalive,KEEPALIVE_DELAY);
   if (!choked)
   {
#ifdef BT_PEER_DEBUG
      werror("%O: sending keepalive\n",ip);
#endif
      transmit("\0\0\0\0"); // keepalive empty message
   }
#ifdef BT_PEER_DEBUG
   else
      werror("%O: would send keepalive, but is choked\n",ip);
#endif
}

//! send a have message to tell I now have piece n
//! ignored if not online
void send_have(int n)
{
   if (online<2) return;
   send_message(MSG_HAVE,"%4c",n);
}

// ----------------------------------------------------------------
mapping(string:function(int,int,string:void|mixed)) piece_callback=([]);

//! called to request a chunk from this peer
void request(int piece,int offset,int bytes,
	     function(int,int,string:void|mixed) callback)
{
   piece_callback[piece+":"+offset+":"+bytes]=callback;
   send_message(MSG_REQUEST,
		"%4c%4c%4c",piece,offset,bytes);
}

void got_piece_from_peer(int piece,int offset,string data)
{
   function(int,int,string:void|mixed) f;
   string s;
   if ((f=piece_callback[s=piece+":"+offset+":"+strlen(data)]))
   {
      m_delete(piece_callback,s);
      f(piece,offset,data);
   }
   else
   {
      Function.call_callback(
	 warning,"%O: got unrequested piece %d/%d+%db\n",
	 piece,offset,strlen(data));
   }
}

// ----------------------------------------------------------------

array(array(int)) queued_pieces=({});

static void queue_piece(int piece,int offset,int length)
{
   if (length>524288 || 
       length>parent->info["piece length"])
   {
      Function.call_callback(
	 warning,
	 "%O: got request for a too large chunk, %d bytes - ignored\n",
	 ip,length);
      return;
   }

   queued_pieces+=({({piece,offset,length,0})});
   if (queued_pieces[0][3]==0)
   {
      remove_call_out(fill_queue);
      fill_queue();
   }

   if (sendbuf=="" && !choked)
      fd->set_write_callback(peer_write);
}

static void fill_queue()
{
   if (!sizeof(queued_pieces) ||
       queued_pieces[0][3]!=0) return;
   queued_pieces[0][3]=parent->get_piece_chunk(@queued_pieces[0][..2]);
}

// ----------------------------------------------------------------
// stubs
   
//! called whenever there is a status change for this peer:
//! "created": always called when the object is created
//!
//! does not need to call inherited function
void status(string type,void|int|string data)
{
}

//! called for downloading progress; this is called from zero
//! to total for each downloaded piece.
//!
//! does not need to call inherited function
void progress(int pieceno,int byte,int total)
{
}

// ----------------------------------------------------------------

string _sprintf(int t)
{
   if (t=='O') return sprintf("Bittorrent.Peer(%s:%d)",ip,port);
   return 0;
}

void destroy()
{
   destruct(fd);
   remove_call_out(keepalive);
   remove_call_out(bandwidth_o_meter);
}

// ----------------------------------------------------------------

static private int bandwidth_in_count=0;
static private int bandwidth_out_count=0;
static private int bandwidth_t0=time(1);
static private float bandwidth_t=time(bandwidth_t0);

void bandwidth_o_meter()
{
   call_out(bandwidth_o_meter,BANDWIDTH_O_METER_DELAY);

   float t1=time(bandwidth_t0);
   float t=t1-bandwidth_t;
   if (t<0.001) return;
   bandwidth_in=(int)(bandwidth_in_count/t);
   bandwidth_out=(int)(bandwidth_out_count/t);

   bandwidth_in_count=0;
   bandwidth_out_count=0;
   bandwidth_t=t1;
}
