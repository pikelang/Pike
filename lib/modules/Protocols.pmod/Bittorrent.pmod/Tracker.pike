
#pike __REAL_VERSION__

//! The query interval reported back to clients. Defaults to
//! @expr{1800@}.
int(0..) interval = 1800;

int(1..) default_numwants = 50;

//! Allow clients to dynamically add torrents to the tracker.
int(0..1) dynamic_add_torrents = 1;

class Client (string ip, int port) {

  int uploaded;
  int downloaded;
  int left;

  int poked;

  void update_stats( string u, string d, string l ) {
    uploaded = (int)u;
    downloaded = (int)d;
    left = (int)l;

    int poked = time();
  }
}

class TorrentInfo {
  mapping(string:Client) clients = ([]);
  array(string) seeds = ({});
  int completed;

  int num_seeds() {
    return sizeof(seeds);
  }

  int num_leechers() {
    return sizeof(clients)-num_seeds();
  }

  int num_completed() {
    return completed;
  }

  Client get_client(string peer_id) {
    return clients[peer_id];
  }

  void remove_client(string peer_id) {
    m_delete(clients, peer_id);
    seeds -= ({ peer_id });
  }

  void add_client(string peer_id, Client c) {
    clients[peer_id] = c;
  }

  void rebuild_seed_list() {
    array s = ({});
    foreach(clients; string id; Client c)
      if(!c->left) s += ({ id });
    seeds = s;
  }

  string compact_peerlist(array peers) {
    String.Buffer buf = String.Buffer(sizeof(peers)*6);
    foreach(peers, string id) {
      Client c = clients[id];
      string ip = c->ip;
      for(int i; i<4; i++)
	buf->putchar( ip[i] );
      buf->add( sprintf("%2c", c->port) );
    }
    return (string)buf;
  }

  array(mapping(string:string|int)) peerless_peerlist(array peers) {
    array ret = ({});
    foreach(peers, string id) {
      Client c = clients[id];
      ret += ({ ([ "ip":c->ip, "port":c->port ]) });
    }
    return ret;
  }

  array(mapping(string:string|int)) peerlist(array peers) {
    array ret = ({});
    foreach(peers, string id) {
      Client c = clients[id];
      ret += ({ ([ "peer id":id, "ip":c->ip, "port":c->port ]) });
    }
    return ret;
  }

  void gc() {
    foreach(clients; string id; Client c)
      if( c->poked < time()-interval*2 )
	remove_client(id);
  }
}

mapping(string:TorrentInfo) torrents = ([]);

//! Add a torrent to the tracker.
//! @param id
//!   The info hash of the torrent file.
void add_torrent(string id) {
  torrents[id] = TorrentInfo();
}

mapping report_failure(string why) {
  return ([ "failure reason" : why ]);
}

//! Handles HTTP announce queries to the tracker.
string announce(mapping args, string ip) {
  return .Bencoding.encode( low_announce(args, ip) );
}

mapping(string:mixed) low_announce(mapping args, string ip) {

  if( !args->info_hash )
    return report_failure( "No info_hash query variable." );
  if( sizeof(args->info_hash)!=20 )
    return report_failure( "Wrong size of info_hash data." );
  if( !torrents[ args->info_hash ] ) {
    if( !dynamic_add_torrents )
      return report_failure( "No such torrent tracked by this tracker." );
    add_torrent( args->info_hash );
  }

  TorrentInfo torrent = torrents[ args->info_hash];

  if( args->ip ) {
    array temp_ip=array_sscanf(args->ip, "%d.%d.%d.%d");
    if( sizeof(temp_ip)!=4 )
      return report_failure( "Could not parse ip query variable." );
#if 1
    if( temp_ip[0]!=10 ||
	(temp_ip[0]!=172 && temp_ip[1]!=16) ||
	(temp_ip[0]!=192 && temp_ip[1]!=168) )
      return report_failure( "This tracker only allows remapping "
			     "RFC 1918 addresses." );
#endif
    ip = sprintf("%c%c%c%c", @temp_ip);
  }
  else
    ip = sprintf("%c%c%c%c", @array_sscanf(ip, "%d.%d.%d.%d"));

  if( !args->peer_id )
    return report_failure( "No peer_id query variable." );
  if( sizeof(args->peer_id)!=20 )
    return report_failure( "Wrong size of peer_id data." );
  string peer_id = args->peer_id;

  if( !args->port )
    return report_failure( "No port query variable." );
  int port = (int)args->port;
  if( port<1 || port>65535 )
    return report_failure( "Port out of range." );

  Client c = torrent->get_client(peer_id);

  if( args->event == "stopped" ) {
    if( c ) {
      if( c->ip!=ip )
	return report_failure( "Unregsiter your own clients!!" );
      torrent->remove_client(peer_id);
    }
    return ([]); // Don't bother compiling a proper directory.
  }
  else if( args->event == "complete" ) {
    torrent->completed++;
  }

  // FIXME: Always treat as "started"?
  if( !c ) {
    c = Client(ip, port);
    torrent->add_client(peer_id, c);
  }
  c->update_stats( args->uploaded, args->downloaded, args->left );

  if(args->left && !(int)args->left)
    torrent->rebuild_seed_list();

  array(string) peers = indices(torrent->clients);
  peers -= ({ peer_id });

  if( !c->left )
    peers -= torrent->seeds;

#if 0
  if( args->event == "started" ) {
    // sort list as closest match to ip.
  }
  else
#endif
    peers = Array.shuffle(peers); // Don't trust indices to be random.

  peers = peers[..((int)args->numwants || 50)];

  mapping m = ([ "interval":interval ]);
  if( args->compact )
    m->peers = torrent->compact_peerlist(peers);
  else if ( args->no_peer_id )
    m->peers = torrent->peerless_peerlist(peers);
  else
    m->peers = torrent->peerlist(peers);

  return m;
}

string be_report_failure( string err ) {
  return .Bencoding.encode( report_failure(err) );
}

//! Returns the result of a scrape query.
string scrape(mapping args) {
  mapping res = ([ "files" : ([]) ]);

  array ts;
  if(args->info_hash) {
    if( sizeof(args->info_hash)!=20 )
      return be_report_failure( "Wrong size of info_hash data." );
    if( !torrents[ args->info_hash ] )
      return be_report_failure( "No such torrent tracked by this tracker." );
    ts = ({ args->info_hash });
  }
  else
    ts = indices(torrents);

  foreach(ts, string hash) {
    TorrentInfo t = torrents[hash];
    mapping f = ([ "complete" : t->num_seeds(),
		   "downloaded" : t->num_completed(),
		   "incomplete" : t->num_leechers() ]);
    res->files[hash] = f;
  }

  return .Bencoding.encode(res);
}

// --- UDP Code

class Msg {
  inherit ADT.Struct;
}

class MsgConnectReq {
  inherit Msg;
  Item connection_id = uint64();
  Item action = uint32();
  Item transaction_id = uint32();
}

class MsgConnectAns {
  inherit Msg;
  Item action = uint32(0);
  Item transaction_id = uint32();
  Item connection_id = uint64();
}

string make_udp_header(int action, int id) {
  return sprintf("%4c%4c", action, id);
}

string udp_error(int id, string msg) {
  return make_udp_header(3, id) + msg;
}

class MsgReqHeader {
  inherit Msg;
  Item connection_id = uint64();
  Item action = uint32();
}

class MsgAnnounceReq {
  inherit Msg;
  Item connection_id = uint64();
  Item action = uint32();
  Item transaction_id = uint32();
  Item info_hash = Chars(20);
  Item peer_id = Chars(20);
  Item downloaded = uint64();
  Item left = uint64();
  Item uploaded = uint64();
  Item event = uint32();
  Item ip = uint32();
  Item key = uint32();
  Item num_want = int32();
  Item port = uint16();
  Item extensions = uint16();
}

class MsgAnnounceAns {
  inherit Msg;
  Item action = uint32(1);
  Item transaction_id = uint32();
  Item interval = uint32();
  Item leechers = uint32();
  Item seeders = uint32();
}

class MsgScrapeReq {
  inherit Msg;
  Item connection_id = uint64();
  Item action = uint32();
  Item transaction_id = uint32();
  Item num_info_hashes = uint16();
  Item extensions = uint16();
}

int udp_id = random( pow(2,64) );

string udp(string msg, string ip) {

  if(msg[..8] == String.hex2string("0000041727101980")) {
    // action 0: connect
    Msg m = MsgConnectReq(msg);
    if(m->action) return udp_error(0, "Wrong handshake action.");

    udp_id++;
    if(udp_id==0x41727101980)
      udp_id++;
    udp_id %= pow(2,64);

    Msg r = MsgConnectAns();
    r->transaction_id = m->transaction_id;
    r->connection_id = udp_id;
    return (string)r;
  }

  Msg header = MsgReqHeader(msg);

  switch(header->action) {
  case 1: { // announce
    Msg m = MsgAnnounceReq(msg);
    mapping args = ([ "info_hash" : m->info_hash,
		      "peer_id" : m->peer_id,
		      "port" : m->port,
		      "uploaded" : m->uploaded,
		      "downloaded" : m->downloaded,
		      "left" : m->left,
		      "compact" : 1,
		      "key" : m->key ]);
    if(m->num_want != -1) args->numwant = m->num_want;
    if(m->event) {
      args->event = ([ 1 : "completed",
		       2 : "started",
		       3 : "stopped" ])[ m->event ];
      if(!args->event)
	return udp_error(header->transaction_id, "Unknown event.");
    }
    if(m->ip) args->ip = sprintf("%d.%d.%d.%d", @(array)sprintf("%4c",m->ip) );

    mapping res = low_announce(args, ip);
    if(res["failure reason"])
      return udp_error(header->transaction_id, res["Failure reason"]);

    Msg ret = MsgAnnounceAns();
    ret->transaction_id = m->transaction_id;
    ret->interval = res->interval;

    TorrentInfo t = torrents[m->info_hash];
    ret->leechers = t->num_leechers();
    ret->seeders = t->num_seeds();

    return (string)ret + res->peers;
    break;
  }

  case 2: { // scrape
    Msg m = MsgScrapeReq(msg);
    msg = msg[..sizeof(m)-1];
    array ts = msg/20;
    ts = ts[..m->num_info_hashes-1];
    if(sizeof(ts)<m->num_info_hashes)
      return udp_error(header->transaction_id, "Too few info hashes.");
    string ret = "";
    foreach(ts, string hash) {
      TorrentInfo t = torrents[hash];
      if( !t )
	ret += "\0\0\0\0""\0\0\0\0""\0\0\0\0";
      else
	ret += sprintf("%4c%4c%4c", t->num_completed(),
		       t->num_seeds(), t->num_leechers);
    }
    return make_udp_header(2, header->transaction_id) + ret;
    break;
  }

  default:
    return udp_error(0, "Unkown action.");
  }

}


void clean_torrents() {
  values(torrents)->gc();
  call_out(clean_torrents, interval);
}

void create() {
  call_out(clean_torrents, interval);
}
