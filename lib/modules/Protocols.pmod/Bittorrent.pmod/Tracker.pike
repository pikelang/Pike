
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

string report_failure(string why) {
  return .Bencoding.encode( ([ "failure reason" : why ]) );
}

//! Handles HTTP announce queries to the tracker.
string announce(mapping args, string ip) {

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
    return "de"; // Don't bother compiling a proper directory.
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

  return .Bencoding.encode(m);
}

//! Returns the result of a scrape query.
string scrape(mapping args) {
  mapping res = ([ "files" : ([]) ]);

  array ts;
  if(args->info_hash) {
    if( sizeof(args->info_hash)!=20 )
      return report_failure( "Wrong size of info_hash data." );
    if( !torrents[ args->info_hash ] )
      return report_failure( "No such torrent tracked by this tracker." );
    ts = ({ args->info_hash });
  }
  else
    ts = indices(torrents);

  foreach(ts, string hash) {
    TorrentInfo t = torrents[hash];
    mapping f = ([ "complete" : t->num_seeds(),
		   "downloaded" : t->num_completed(),
		   "incomplete" : t->num_leechers() ]);
  }

}

void clean_torrents() {
  values(torrents)->gc();
  call_out(clean_torrents, interval);
}

void create() {
  call_out(clean_torrents, interval);
}
