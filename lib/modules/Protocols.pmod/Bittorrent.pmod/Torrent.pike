import .Bencoding;

mapping(string:int|array|string|mapping) metainfo;
mapping(string:int|array|string|mapping) info; // info part of metainfo

array(int(0..1)) file_got=0;
multiset(int) file_want;
int total_bytes;
string info_sha1;
array(Target) targets;

string all_pieces_bits=0; // to compare to see which hosts are complete

string my_peer_id=generate_peer_id();

string my_ip;
int my_port=6881;
int uploaded=0;   // bytes
int downloaded=0; // bytes

int downloading=0; // flag

string datamode="rw";

function(this_program,mapping:object(.Peer)) peer_program=.Peer;
mapping(string:object(.Peer)) peers=([]);

//! if set, called when we got another piece downloaded (no args)
function pieces_update_status=0;

//! if set, called when we start to download another piece (no args)
function downloads_update_status=0;

//! called if there is a protocol error; defaults to werror
function(string,mixed...:void|mixed) warning=werror;

//! loads the metainfo from a file
void load_metainfo(string filename)
{
   string s=Stdio.read_file(filename);
   if (!s)
      error("Failed to read metainfo file %O: %s\n",
	    filename,strerror(errno()));

   string t;
   sscanf(s,"%*s4:info%s",t);

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

#if 1
// check if our encoder makes the same string; it should
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
//! but if it isn't created, a call to verify_file() is necessary after
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

   if (targets->created==({1})*sizeof(targets))
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

   all_pieces_bits=bits2string(replace(file_got,0,1));

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

//       werror(strlen(targets->pread(n*info["piece length"],
// 				   info["piece length"])*"")+"\n");
      string sha1=SHA1(targets->pread(n*info["piece length"],
				      info["piece length"])*"");
//       if (sha1!=".\0\17§èWYÇôÂTÔÙÃ>ô\201äY§")
//       werror("got %O want %O\n",sha1,want_sha1);
//       if (want_sha1!=sha1)
//       {
// 	 werror("%d failed verify\n",n);
// 	 Stdio.write_file("piece "+n,
// 			  targets->pread(n*info["piece length"],
// 					 info["piece length"])*"");
// 	 exit(1);
//       }
//       if (n<352)
	 file_got+=({want_sha1==sha1});
//       else
// 	 file_got+=({0});
      n++;
   }
   if (progress_callback) progress_callback(n,sizeof(want_sha1s));

   file_want=(multiset)filter(indices(file_got),
			      lambda(int i) { return !file_got[i]; });
   
//    werror("%O\n",file_want);
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

//! contact and update the tracker with current status
//! will fill the peer list
void update_tracker(void|string event)
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
	    peers[m["peer id"]]=peer_program(this_object(),m);
   }
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

int max_downloads=10;
int min_completed_peers=2;

// downloads lost by disconnect or choking that needs to be taken up again
mapping(int:PieceDownload) lost_in_space=([]);
mapping(int:PieceDownload) downloads=([]);
mapping(int:PieceDownload) handovers=([]);

// ----------------

void download_more()
{
   int did=0;
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

   werror("downloads: %d  available: %d  complete: %d  activated: %d  "
	  "c-a: %d\n",
	  sizeof(downloads),
	  sizeof(available_peers),
	  sizeof(completed_peers),
	  sizeof(activated_peers),
	  sizeof(completed_peers_avail));

   if (!downloading || sizeof(downloads)>=max_downloads ||
       !sizeof(available_peers))
      return 0;

// ----------------

   array v=indices(file_peers)-indices(downloads);
   array w=map(rows(file_peers,v),sizeof);
   sort(w,v);

   multiset from_peers=available_peers;

   array choices=({});
   int minp=0;

   if (completed_peers_used
       < min(sizeof(completed_peers_avail),
	     min_completed_peers) ||
       sizeof(file_want) == sizeof(file_got)) // first seed
      from_peers&=completed_peers_avail;

   if (!sizeof(from_peers)) 
      return 0; // no source

   if (sizeof(lost_in_space))
      werror("lost_in_space=%O\n",lost_in_space);

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
	 
//       werror("download piece %O, available from %O\n",
// 	     n,v);

      .Peer peer=v[random(sizeof(v))];

      if (lost_in_space[n])
      {
	 lost_in_space[n]->reactivate(peer);
	 m_delete(lost_in_space,n);
      }
      else
	 PieceDownload(peer,n);

      return 1;
   }

// switch algorithm - download same block from more then one peer
      
// ----------------
}

int download_chunk_size=32768;
int download_chunk_max_queue=4; // number of chunks to queue

class PieceDownload
{
   .Peer peer;
   int piece;

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
      werror("reactivate lost download: %O %O %O\n",
	     peer,piece,indices(chunks));

      peer=_peer;

      downloads[piece]=this_object();

      queue_chunks();
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
      }
   }

// seconds before aborting if choked and not unchoked
   constant choke_abort_delay=10; 

   void got_data(int n,int i,string data,object from)
   {
      if (!peer || peer!=from) 
      {
	 werror("you shouldn't call me!!\n");
	 exit(1);
      }
      if (n==-1)
	 switch (data)
	 {
	    case "choked":
	       if (find_call_out(abort)==-1)
	       {
		  call_out(abort,choke_abort_delay);
		  werror("call_out abort\n");
	       }
	       return;
	    case "unchoked":
	       werror("remove_call_out abort\n");
	       remove_call_out(abort);
	       return;
	    case "disconnected":
	       abort(); 
	       return; 
	    default:
	       error("unknown message from Peer: %O\n",data);
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
      else
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
      werror("%O abort\n",this_object());

      remove_call_out(abort);
      if (sizeof(chunks))
      {
	 if (!peer) return; // we're already lost in space, wtf?

   // ok, put us in the queue-to-fill
	 lost_in_space[piece]=this_object();

   // cancel outstanding
	 werror("%O %O->cancel\n",this_object(),peer);
	 peer->cancel_requests();

   // reset some stuff
	 disjoin();

	 peer=0;
	 queued_chunks=([]);

	 download_more();
      }
      else // we didn't get any, just drop it
      {
	 peer->cancel_requests();
	 finish(); 
      }
   }

   void disjoin()
   {
      werror("%O disjoin\n",this_object());

      m_delete(downloads,piece);
      Function.call_callback(downloads_update_status);
   }

   void finish()
   {
      werror("finish\n");
      disjoin();
      download_more();
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

   foreach ( file_want & ((multiset)string2arr(peer->bitfield)); 
	     int i; )
      if ((m=file_peers[i])) m[peer]=1;
      else file_peers[i]=(<peer>);
   file_available=(multiset)indices(file_peers);

   if (sizeof(file_available))
      download_more();
}

void peer_have(.Peer peer,int n)
{
   file_available[n]=1;
   if (file_peers[n])
      file_peers[n][peer]=1;
   else
      file_peers[n]=(<peer>);
   if (file_want[n])
      download_more();
}

// void peer_got_piece(int n)
// {
//    if (file_peers) 
//    {
//       m_delete(file_peers,n);
//       file_available[n]=0;
//    }
// }

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
}

// ----------------------------------------------------------------
