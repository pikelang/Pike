// Bittorrent client - originally by Mirar 

//! Bittorrent peer - download and share.
//! Read more about bittorrent at 
//! http://bitconjurer.org/BitTorrent/introduction.html
//! 
//! @example
//! The smallest usable torrent downloader. As first argument,
//! it expects a filename to a .torrent file.
//! @code
//!   int main(int ac,array am)
//!   {
//!      // initialize Torrent from file:
//!      Protocols.Bittorrent.Torrent t=Protocols.Bittorrent.Torrent(); 
//!      t->load_metainfo(am[1]); 
//!   
//!      // Callback when download status changes:
//!      // t->downloads_update_status=...;
//!   
//!      // Callback when pieces status change (when we get new stuff):
//!      // t->pieces_update_status=...; 
//!   
//!      // Callback when peer status changes (connect, disconnect, choked...):
//!      // t->peer_update_status=...;
//!   
//!      // Callback when download is completed:
//!      t->download_completed_callback=
//!         lambda()
//!         {
//!             call_out(exit,3600,0);    // share for an hour, then exit
//!         };
//!   
//!      // Callback to print warnings (same args as sprintf):
//!      //   t->warning=werror; 
//!   
//!      // type of progress function used below:
//!      void progress(int n,int of) { /* ... */ };
//!   
//!      // Initiate targets from Torrent,
//!      // if target was created, no need to verify:
//!      if (t->fix_targets(1,0,progress)==1)
//!         t->verify_targets(progress); 
//!   
//!      // Open port to listen on,
//!      // we want to do this to be able to talk to firewalled peers:
//!      t->my_port=6881;
//!      t->open_port();
//!   
//!      // Ok, start calling tracker to get peers,
//!      // and tell about us:
//!      t->start_update_tracker();
//!   
//!      // Finally, start the download:
//!      t->start_download();
//!   
//!      return -1;
//!   }
//! @endcode

#pike __REAL_VERSION__

import .Bencoding;

mapping(string:int|array|string|mapping) metainfo;
mapping(string:int|array|string|mapping) info; // info part of metainfo

array(int(0..1)) file_got=0;
multiset(int) file_want;
int total_bytes;
string info_sha1;
array(Target) targets;

string all_pieces_bits=0; // to compare to see which hosts are complete
string no_pieces_bits=0; // to compare to see which hosts are empty

string my_peer_id=generate_peer_id();

string my_ip;
int my_port=7881;
.Port listen_port;
int uploaded=0;   // bytes
int downloaded=0; // bytes

int downloading=0; // flag
int we_are_completed=0; // set when no more to download

int allow_free=100000; // free bytes before demanding back from peers

string datamode="rw";

function(this_program,mapping:object(.Peer)) peer_program=.Peer;
mapping(string:object(.Peer)) peers=([]);
array(.Peer) peers_ordered=({});
array(.Peer) peers_unused=({});
// stop adding peers at this number, increased if we need more
int max_peers=100; 

//! if set, called when we got another piece downloaded (no args)
function pieces_update_status=0;

//! if set, called when we start to download another piece (no args)
function downloads_update_status=0;

//! if set, called when peer status changes
function peer_update_status=0;

//! if set, called when download is completed
function download_completed_callback=0;

//! called if there is a protocol error; defaults to werror
function(string,mixed...:void|mixed) warning=lambda() {};


Protocols.DNS.async_client dns_async=Protocols.DNS.async_client();


//! loads the metainfo from a file
void load_metainfo(string filename)
{
   string s=Stdio.read_file(filename);
   if (!s)
      error("Failed to read metainfo file %O: %s\n",
	    filename,strerror(errno()));


   mixed err=catch {
      metainfo=decode(s);
      if (!mappingp(metainfo))
	 error("Metainfo file does not contain a dictionary (%t)\n",metainfo);
      info=metainfo->info;
      if (!mappingp(info))
	 error("Metainfo->info file does not "
	       "contain a dictionary (%t)\n",metainfo->info);

//       if (((info->length+info["piece length"]-1)/info["piece length"])*20
// 	  != strlen(info["pieces"]))
// 	 error("Metainfo->info->pieces has the wrong length; "
// 	       "expected %d (%d*20), got %d\n",
// 	       ((info->length+info["piece length"]-1)/info["piece length"])*20,
// 	       ((info->length+info["piece length"]-1)/info["piece length"]),
// 	       strlen(info["pieces"]));

#if 0
// check if our encoder makes the same string; it should
// (but only if there isn't stuff after "info" in the dictionary)
      string t;
      sscanf(s,"%*s4:info%s",t);

      if (encode(info)+"e"!=t)
	 error("internal encoder error (?): %O\n !=\n %O\n",
	       encode(info)+"e",t);
#endif
      info_sha1=SHA1(encode(info));
   };
   if (!err) return;

   err[0]=sprintf("Failed to read metainfo file %O:\n",filename)+err[0];
   throw(err);
}

//! Each bittorrent has one or more target files.
//! This represents one of those.
class Target(string base,int length,int offset,void|array path)
{
   string filename;
   Stdio.File fd=Stdio.File();
   int created=-1;


   void open(int create_ok)
   {
      if (path)
	 filename=combine_path(base,@path);
      else
	 filename=base;

      if (!file_stat(filename) && create_ok)
      {
	 Stdio.mkdirhier(combine_path(filename,".."));
	 if (!fd->open(filename,datamode+"c",0666))
	    error("failed to create target file %O: %s\n",
		  filename,strerror(fd->errno()));
	 created=1;
      }
      else
      {
	 if (!fd->open(filename,datamode))
	    error("failed to open target file %O: %s\n",
		  filename,strerror(fd->errno()));
	 created=0;
      }
   }

   void pwrite(int off,string data)
   {
//       werror("%O: pwrite(%O,%d bytes) =%O..%O of %O\n",
// 	     this_object(),off,strlen(data),
// 	     off-offset,off-offset+strlen(data),length);
      if (off>offset+length) return; // noop
      int end=off+strlen(data);
      if (end<=off) return; // noop

      if (off<offset)
	 data=data[offset-off..],off=offset;
      if (end>offset+length)
	 data=data[0..offset+length-off-1];

      fd->seek(off-offset);
      if (fd->write(data)<strlen(data))
	 error("failed to write %d bytes to %O: %s\n",
	       filename,strerror(fd->errno()));
   }

   string pread(int off,int bytes)
   {
      if (off>offset+length) return ""; // noop
      int end=off+bytes;

//       werror("%O: pread(%O,%d bytes) =%O..%O of %O\n",
// 	     this_object(),off,bytes,
// 	     off-offset,off-offset+bytes,length);

      if (off<offset)
	 bytes-=(offset-off),off=offset;
      if (end>offset+length)
	 bytes=offset+length-off;

      if (bytes<0) return "";

      fd->seek(off-offset);
      string s=fd->read(bytes);
      if (!s) s="";
      if (strlen(s)<bytes)
	 s+="\0"*bytes; 

      return s;
   }

   void pclear(int off,string data)
   {
      if (created) pwrite(off,data);
   }

   void allocate_peak()
   {
      if (fd->stat()->size>=length) return; // done
      fd->seek(length-1);
      if (fd->write("\0")<1)
	 error("failed to write last byte to %O: %s\n",
	       filename,strerror(fd->errno()));
   }

   string _sprintf(int t)
   {
      if (t=='O') return sprintf("Torrent.Target(%O, %d bytes at +%d)",
				 filename,length,offset);
   }
}

//! opens target datafile(s)
//! "allocate" determines allocation procedure if the file doesn't exist:
//!  0 don't allocate
//!  1 allocate virtual file size (seek, write end byte)
//!  2 allocate for real (will call progress_callback(pos,length))
//!  -1 means never create a file, only open old files
//! "my_filename" is to substitute the metainfo base target filename
//!
//! returns 1 if the (a) file was already there,
//! 2 if all target files were created 
//!
//! If all files are created, the verify info will be filled as well,
//! but if it isn't created, a call to verify_target() is necessary after
//! this call.
//!
int fix_targets(void|int(-1..2) allocate, void|string base_filename,
		void|function(int,int:void) progress_callback)
{
   if (!metainfo) error("no metainfo initiated\n");
   if (!base_filename)
      if (!info->name)
	 error("error in info (name=%O)\n",info->name);
      else
	 base_filename=info->name;

   if (info->files)
   {
      targets=({});
      int offset=0;
      foreach (info->files;;mapping m)
      {
// 	 werror("%O\n",m);
	 targets+=({Target(base_filename,m->length,
			   offset,m->path)});
	 offset+=m->length;
      }
      total_bytes=offset;
   }
   else
   {
      targets=({Target(base_filename,info->length,0)});
      total_bytes=info->length;
   }

   targets->open(allocate!=-1);
   switch (allocate)
   {
      case 1:
	 targets->allocate_peak();
	 break;

      case 2:
	 string s="\0";
#define ALLOCCHUNK 131072
	 s=s*ALLOCCHUNK; // not as a constant string! make it here

	 for (int pos=0; pos<total_bytes; pos+=ALLOCCHUNK)
	 {
	    if (progress_callback) progress_callback(pos,total_bytes);
	    targets->pclear(pos,s);
	 }
	 if (progress_callback) progress_callback(total_bytes,total_bytes);
	 break;
   }

   if (search(targets->created,0)==-1)
   {
      array(string) want_sha1s=info->pieces/20;
      string zeropiece=SHA1("\0"*info["piece length"]);
      file_got=map(want_sha1s,`==,zeropiece);

      if (total_bytes%info["piece length"])
	 file_got[-1]=
	    SHA1("\0"*(total_bytes%info["piece length"]))
	    == want_sha1s[-1];
   }
   else
      file_got=({0})*
      ((total_bytes+info["piece length"]-1)/info["piece length"]);

   file_want=(multiset)filter(indices(file_got),
			      lambda(int i) { return !file_got[i]; });

   all_pieces_bits=bits2string(replace(copy_value(file_got),0,1));
   no_pieces_bits="\0"*strlen(file_got);

   return search(targets->created,0)==-1 ? 2 : 1;
}

//! verify the file and fill file_got (necessary after load_info,
//! but needs open_file before this call)
//! [ progress_callback(at chunk,total chunks) ]
void verify_targets(void|function(int,int:void) progress_callback)
{
   array(string) want_sha1s=info->pieces/20;
   int n=0;
   file_got=({});
   foreach (want_sha1s;;string want_sha1)
   {
      if (progress_callback) progress_callback(n,sizeof(want_sha1s));

      string sha1=SHA1(targets->pread(n*info["piece length"],
				      info["piece length"])*"");
      file_got+=({want_sha1==sha1});
      n++;
   }
   if (progress_callback) progress_callback(n,sizeof(want_sha1s));

   file_want=(multiset)filter(indices(file_got),
			      lambda(int i) { return !file_got[i]; });
}

//! open the port we're listening on
void open_port()
{
   listen_port=.Port(this_object());
}

// helper functions

private static inline string SHA1(string what)
{
   return Crypto.sha()->update(what)->digest();
}

private static inline string generate_peer_id()
{
   int i=(random(2->pow(80))->next_prime()*random(2->pow(80))->next_prime());
   string s=i->digits(256);
   while (strlen(s)<20) s="\0"+s;
   return s[..19];
};

//! calculate the bytes successfully downloaded (full pieces)
int bytes_done()
{
   if (!file_got)
      error("Torrent is not initialized/verified (run verify_targets?)\n");

   int done=info["piece length"]*sizeof(file_got-({0}));

   if (total_bytes%info["piece length"] &&
       file_got[-1]==1)
      done=done
	 -info["piece length"]
	 +(total_bytes%info["piece length"]);

   return done;
}

//! calculate the bytes left to download
int bytes_left()
{
   return total_bytes-bytes_done();
}

int last_tracker_update;
int tracker_update_interval=300;
int tracker_call_if_starved=60; // delay until ok to call if starved

//! contact and update the tracker with current status
//! will fill the peer list
void update_tracker(void|string event,void|int contact)
{
   mapping req=
      (["info_hash":info_sha1,
	"peer_id":my_peer_id,
	"uploaded":(string)uploaded,
	"downloaded":(string)downloaded,
	"left":(string)bytes_left(),
	"port":(string)my_port,
      ]);
   if (my_ip) req->ip=my_ip;
   if (event) req->event=event;

   object q=Protocols.HTTP.get_url(
      metainfo->announce,
      req);

   if (!q)
   {
      error("tracker request failed, %s\n",
	    strerror(errno()));
   }

   if (q->status!=200)
   {
      error("tracker request failed, code %d %O:\n%O\n",
	    q->status,q->status_desc[..50],
	    q->data()[..77]);
   }

   mapping m;
   mixed err=catch {
      m=decode(q->data());
   };
   if (err)
   {
      error("tracker request failed, says "
	    "(raw, doesn't speak bencoded):\n%O\n",
	    q->data()[..77]);
   }
   else
   {
      if (q["failure reason"])
	 error("tracker request failed, failure reason:\n%O\n",
	       q["failure reason"][..1000]);
   }

   if (m->peers)
   {
      foreach (m->peers;;mapping m)
	 if (!peers[m["peer id"]])
	 {
	    .Peer p;
	    if (peers[m["peer id"]]) continue;

	    peers[m["peer id"]]=(p=peer_program(this_object(),m));
	    if (sizeof(peers_ordered)<max_peers && contact) 
	    {
	       peers_ordered+=({p});
 	       p->connect();
	    }
	    else 
	       peers_unused+=({p});
	 }
   }

   last_tracker_update=time(1);

   if (find_call_out(update_tracker_loop)!=-1)
   {
      remove_call_out(update_tracker_loop);
      call_out(update_tracker_loop,tracker_update_interval);
   }
}

int last_increase=0;
int min_time_between_increase=30;
void increase_number_of_peers(void|int n)
{
   if (time(1)-last_increase<min_time_between_increase)
   {
      remove_call_out(increase_number_of_peers);
      call_out(increase_number_of_peers,
	       min_time_between_increase+last_increase-time(1));
      return;
   }
   last_increase=time(1);
   
   if (n<=0) n=25;

   max_peers=max(max_peers, sizeof(peers_ordered)+n);

#if 0
   werror("want to increase number of peers. "
	  "time left before next update: %O\n",
	  last_tracker_update+tracker_call_if_starved-time(1));
#endif

   if (sizeof(peers_unused)<n &&
       find_call_out(update_tracker_loop)!=-1 &&
       time(1)>=last_tracker_update+tracker_call_if_starved)
   { 
      remove_call_out(update_tracker_loop);
      call_out(update_tracker_loop,
	       last_tracker_update+tracker_call_if_starved-time(1));
   }

   n=max_peers-sizeof(peers_ordered);
   array v=peers_unused[..n-1];
   peers_ordered+=v;
   peers_unused=peers_unused[n..];

   foreach (v;;.Peer p)
      if (p->online) 
      {
	 werror("%O online but unused?\n",p->ip);
	 v-=({p});
      }

   v->connect();
}

//! starts to contact the tracker at regular intervals,
//! giving it the status and recieving more peers to talk to,
//! will also contact these peers
//!
//! default interval is 5 minutes
//!
//! if given an event, will update tracker with it
void start_update_tracker(void|int interval)
{
   remove_call_out(update_tracker_loop);
   if (interval>0) tracker_update_interval=interval;
   update_tracker_loop();
}

static void update_tracker_loop()
{
   call_out(update_tracker_loop,tracker_update_interval);
   if (!sizeof(peers))
      update_tracker(sizeof(file_want)?"started":"completed",1);
   else if (sizeof(peers))
      update_tracker(0,!we_are_completed);
}

//! stops updating the tracker; will send the event as a last
//! event, if set
//! will not contact new peers
void stop_update_tracker(void|string event)
{
   if (event) update_tracker(event,0);
   remove_call_out(update_tracker_loop);
}

//! contact all or n peers 
void contact_peers(void|int n)
{
   array v=filter(values(peers),"is_connectable");
   if (n>0) v=v[..n-1];
   v->connect();
}

string _file_got_bitfield=0;
// usually called from peers
//! returns the file got field as a string bitfield (cached)
string file_got_bitfield()
{
   return _file_got_bitfield ||
      (_file_got_bitfield = bits2string(file_got));
}

// string _file_xor_bitfield=0;
// string _file_want_bitfield=0;
// //! returns the what we want as a bitfield
// string file_want_bitfield()
// {
//    if (!_file_xor_bitfield)
//       _file_xor_bitfield = bits2string( allocate(sizeof(file_got),1) );

//    return _file_want_bitfield ||
//       (_file_got_bitfield = bits2string(file_got)^_file_xor_bitfield);
// }

// ----------------------------------------------------------------

//! initiate the downloading scheme
void start_download()
{
   downloading=1;
   download_more();
}

int max_downloads=15;
int min_completed_peers=2;

// downloads lost by disconnect or choking that needs to be taken up again
mapping(int:PieceDownload) lost_in_space=([]);
mapping(int:PieceDownload) downloads=([]);
mapping(int:PieceDownload) handovers=([]);

// ----------------

void download_more()
{
   int did=0;

   if (!sizeof(file_want)) 
   {
      if (!we_are_completed) // all done, tidy up
      {
	 we_are_completed=1;
	 downloading=0;
	 Function.call_callback(download_completed_callback);
	 
	 max_peers=sizeof(peers_ordered);
	 update_tracker("completed");

	 remove_call_out(increase_number_of_peers);
	 filter(filter(peers_ordered,"is_online"),"is_completed")
	    ->disconnect();
      }
      return;
   }

   while (download_one_more()) did++;

   if (did) Function.call_callback(downloads_update_status);
}

int download_one_more()
{
   multiset(.Peer) available_peers;
   multiset(.Peer) completed_peers;
   multiset(.Peer) activated_peers;
   multiset(.Peer) completed_peers_avail;

   available_peers=(multiset)filter(values(peers),"is_available");
   activated_peers=(multiset)filter(values(peers),"is_activated");
   completed_peers=(multiset)filter(values(peers),"is_completed");
   completed_peers_avail=completed_peers & available_peers;

   int completed_peers_used=sizeof(completed_peers&activated_peers);

#if 0
   werror("downloads: %d/%d  available: %d  complete: %d  activated: %d  "
	  "c-a: %d\n",
	  sizeof(downloads), sizeof(handovers), 
	  sizeof(available_peers),
	  sizeof(completed_peers),
	  sizeof(activated_peers),
	  sizeof(completed_peers_avail));
#endif

   if (!downloading || sizeof(downloads)>=max_downloads)
   {
#if 0
      werror("doesn't download: too many downloads %O/%O\n",
	     sizeof(downloads),max_downloads);
#endif
      return 0;
   }

   if (!sizeof(available_peers))
   {
#if 0
      werror("doesn't download: no available peers\n");
#endif
      increase_number_of_peers();
      return 0; 
   }

// ----------------

   array v=indices(file_peers)-indices(downloads)-indices(handovers);
   array w=map(rows(file_peers,v),sizeof);
   sort(w,v);

   multiset from_peers=available_peers;

   array choices=({});
   int minp=0;

   if (sizeof(completed_peers_avail) &&
       (completed_peers_used<min_completed_peers ||
	sizeof(file_want) == sizeof(file_got))) // first seed
      from_peers&=completed_peers_avail;

   if (!sizeof(from_peers)) 
   {
      werror("NO SOURCE!!\n");
      exit(1);
      return 0; // no source
   }

//    if (sizeof(lost_in_space))
//       werror("lost_in_space=%O\n",lost_in_space);

   for (int i=!sizeof(lost_in_space); i<2 && !sizeof(choices); i++)
   {
      array u=v;
      if (!i) u&=indices(lost_in_space); // restrain
      foreach (u;;int n)
      {
	 multiset(.Peer) p=file_peers[n];

	 if (!minp) minp=sizeof(p);
	 else if (minp<sizeof(p) && sizeof(choices)) break;

	 multiset(.Peer) p2=p&from_peers;
	 if (!sizeof(p2)) continue;
	 choices+=({({n,(sizeof(p2)!=sizeof(p))?p2:p})});
      }
   }

   if (sizeof(choices))
   {
      int i=random(sizeof(choices));
      array v=(array)choices[i][1];
      int n=choices[i][0];

      .Peer peer=v[random(sizeof(v))];

//       werror("download piece %O from %O\n",n,peer);

      if (lost_in_space[n])
      {
	 lost_in_space[n]->reactivate(peer);
	 m_delete(lost_in_space,n);
      }
      else
	 PieceDownload(peer,n);

      return 1;
   }

// ----------------

// switch algorithm - download same block from more then one peer

// try downloading random one *again*
   w=Array.shuffle((array)file_want);

// don't download from the handover peers though
   from_peers-=(multiset)values(handovers)->peer;

   foreach (w;;int piece)
   {
      multiset m=(file_peers[piece]||(<>))&from_peers;

      if (sizeof(m))
      {
	 .Peer peer=((array)m)[random(sizeof(m))];
	 
	 (downloads[piece]||handovers[piece])
	    ->use_more_peers(peer);

	 return 1;
      }
   }

#if 0
   werror("doesn't download: endgame (%d pieces left) but no more useable\n",
	  sizeof(file_want));
#endif
   increase_number_of_peers();
}

int download_chunk_size=32768;
int download_chunk_max_queue=4; // number of chunks to queue

class PieceDownload
{
   .Peer peer;
   int piece;
   array(.Peer) more_peers=({});
   int using_more_peers=0; // to know if we should warn 

   mapping chunks=([]);
   mapping expect_chunks=([]);
   mapping queued_chunks=([]);

   int handed_over=0;

   void create(.Peer _peer,int n)
   {
      peer=_peer;
      piece=n;

      downloads[piece]=this_object();

      int size=info["piece length"];
      if (piece+1==sizeof(file_got) &&
	  total_bytes%info["piece length"])
	 size=total_bytes%info["piece length"];
      for (int i=0; i<size; i+=download_chunk_size)
      {
	 int z=min(size-i,download_chunk_size);
	 expect_chunks[i]=z;

// 	 werror("expect p=%d o=%d %d bytes\n",piece,i,z);
      }

      queue_chunks();
   }

   void reactivate(.Peer _peer)
   {
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
      werror("reactivate lost download: %O %O %O\n",
 	     peer,piece,indices(chunks));
#endif

      peer=_peer;

      downloads[piece]=this_object();

      queue_chunks();
   }

   void use_more_peers(.Peer p2)
   {
      more_peers+=({p2});

// queue a random part that we haven't got yet      
      array v=indices(expect_chunks)-indices(chunks);
      int i=v[random(sizeof(v))];

      p2->request(piece,i,expect_chunks[i],got_data);
   }

   void queue_chunks()
   {
      foreach (sort(indices(expect_chunks-chunks-queued_chunks));;int i)
      {
	 if (sizeof(peer->piece_callback)>=download_chunk_max_queue)
	    return;

	 int z=expect_chunks[i];
	 queued_chunks[i]=z;
	 peer->request(piece,i,z,got_data);
      }

      if (!handed_over &&
	  sizeof(peer->piece_callback)<download_chunk_max_queue)
      {
	 peer->handover=1;
	 handovers[piece]=this_object();
	 disjoin();
	 handed_over=1;
      }
   }

// seconds before aborting if choked and not unchoked
   constant choke_abort_delay=45; 

   void got_data(int n,int i,string data,object from)
   {
      downloaded+=strlen(data); // always, even if bad

      if (n==-1)
	 switch (data)
	 {
	    case "choked":
	       abort();
#if 0
	       if (find_call_out(abort)==-1)
	       {
		  call_out(abort,choke_abort_delay);
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
		  werror("call_out abort\n");
#endif
	       }
#endif
	       return;
	    case "unchoked":
#if 0
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
	       werror("remove_call_out abort\n");
#endif
	       remove_call_out(abort);
#endif
	       return;
	    case "disconnected":
	       abort(); 
	       return; 
	    default:
	       error("unknown message from Peer: %O\n",data);
	 }

      if (from!=peer)
      {
	 more_peers-=({from});
	 download_more();
      }

      if (piece==n && expect_chunks[i]==strlen(data))
      {
// 	 werror("got piece %d off %d bytes %d\n",n,i,strlen(data));
		
	 chunks[i]=data;
	 m_delete(queued_chunks,i);

	 if (sizeof(expect_chunks-chunks))
	 {
	    queue_chunks(); // request more
	 }
	 else
	 {
	    string res="";
	    foreach (sort(indices(chunks));;int i)
	       res+=chunks[i];

// 	    Stdio.write_file("chunk "+n+" got",res);
// 	    Stdio.write_file("chunk "+n+" had",
// 			     targets->pread(piece*info["piece length"],
// 					    info["piece length"])*"");

	    if (SHA1(res)!=info->pieces[20*piece..20*piece+19])
	    {
	       Function.call_callback(
		  warning,"sha1 checksum failed on piece %d from %s:%d\n",
		  piece,peer->ip,peer->port);
// 	       werror("%d bytes: %O!=%O\n",
// 		      strlen(res),SHA1(res),info->pieces[20*n..20*n+19]);
	    }
	    else
	    {
	       got_piece(piece,res);
	    }
	    finish();
	 }
      }
      else if (!use_more_peers)
      {
	 Function.call_callback(
	    warning,"got unrequested chunk from %s:%d, "
	    "piece=%d offset=%d bytes=%d\n",
	    peer->ip,peer->port,
	    n,i,strlen(data));
      }
   }

   void abort()
   {
      if (!this_object()) werror("abort in destructed\n");
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
      werror("%O abort\n",this_object());
#endif

      remove_call_out(abort);
      if (sizeof(chunks))
      {
	 if (!peer) return; // we're already lost in space, wtf?

   // ok, put us in the queue-to-fill
	 lost_in_space[piece]=this_object();

   // cancel outstanding
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
	 werror("%O %O->cancel\n",this_object(),peer);
#endif
	 if (peer->online) peer->cancel_requests(0);

   // reset some stuff
	 disjoin();
	 handed_over=0; // if we ever were

	 peer=0;
	 queued_chunks=([]);

	 download_more();
      }
      else // we didn't get any, just drop it
      {
	 if (peer->online) peer->cancel_requests(0);
	 finish(); 
      }
   }

   void disjoin()
   {
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
      werror("%O disjoin\n",this_object());
#endif

      if (!handed_over)
	 m_delete(downloads,piece);
      else
	 m_delete(handovers,piece);
      Function.call_callback(downloads_update_status);
   }

   void finish()
   {
#ifdef TORRENT_PIECEDOWNLOAD_DEBUG
      werror("finish\n");
#endif
      disjoin();
      download_more();

      if (handovers[piece]==this_object() ||
	  downloads[piece]==this_object())
      {
	 werror("destruction of download still in structs:\n"
		"download: %O\ndownloads: %O\nhandovers: %O\n",
		this_object(),downloads,handovers);

	 werror("%s\n",master()->describe_backtrace(backtrace()));

	 exit(1);
      }

      more_peers->cancel_requests(0);

      destruct(this_object());
   }

   string _sprintf(int t)
   {
      if (t=='O') 
	 return sprintf(
	    piece+"  "+
	    "Torrent/PieceDownload(%d/%d done from %d from %O)",
			sizeof(chunks),sizeof(expect_chunks),
			piece,peer);
      return 0;
   }
}

// ----------------------------------------------------------------

// these are already filtered with what we want
mapping(int:multiset(.Peer)) file_peers=([]); 
multiset(int) file_available=(<>);

void peer_lost(.Peer peer)
{
   if (!file_peers) return;
   foreach ( file_want & ((multiset)string2arr(peer->bitfield)); 
	     int i; )
   {
      multiset m;
      m=file_peers[i];
      if (!m) continue;
      m[peer]=0;
      if (!sizeof(m))
      {
	 m_delete(file_peers,i);
	 file_available[i]=0;
      }
   }
}

void peer_gained(.Peer peer)
{
   multiset m;

   multiset mz=(multiset)string2arr(peer->bitfield);

   foreach ( file_want & mz; 
	     int i; )
      if ((m=file_peers[i])) m[peer]=1;
      else file_peers[i]=(<peer>);
   file_available|=(multiset)indices(file_peers);

#ifdef TORRENT_PEERS_DEBUG
   werror("%O gained: %d blocks, we want %d of those\n",
	  peer->ip,sizeof(mz),sizeof(mz&file_want));
#endif

   if (sizeof(file_want & mz))
   {
      if (!peer->were_interested) peer->show_interest();
      download_more();
   }
}

void peer_unchoked(.Peer peer)
{
   multiset m=(multiset)string2arr(peer->bitfield);
#ifdef TORRENT_PEERS_DEBUG
   werror("%O unchoked: %d blocks, we want %d of those\n",
	  peer->ip,sizeof(m),sizeof(m&file_want));
#endif

   download_more();
}

void peer_have(.Peer peer,int n)
{
   if (!file_want[n]) return; // ignore
   
   file_available[n]=1;
   if (file_peers[n])
      file_peers[n][peer]=1;
   else
      file_peers[n]=(<peer>);
   if (file_want[n])
   {
      if (!peer->were_interested) peer->show_interest();
      download_more();
   }
}

string get_piece_chunk(int piece,int offset,int length)
{
   return targets->pread(piece*info["piece length"]+offset,length)*"";
}

void got_piece(int piece,string data)
{
   targets->pwrite(piece*info["piece length"],data);

   values(peers)->send_have(piece);

   file_got[piece]=1;
   _file_got_bitfield=0;
   file_want[piece]=0;
   m_delete(file_peers,piece);
   file_available[piece]=0;

   Function.call_callback(pieces_update_status);

   if (!sizeof(file_want) &&
       find_call_out(update_tracker_loop)!=-1)
      update_tracker("completed");

// open up interested peers for requests
   if (sizeof(file_want)==sizeof(file_got)-1)
      foreach (peers_ordered;;.Peer p)
	 if (p->peer_interested)
	    p->unchoke();
}

// ----------------------------------------------------------------
