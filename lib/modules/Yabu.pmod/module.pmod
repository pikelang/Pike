/* Yabu. By Fredrik Noring, 1999.
 *
 * Yabu is an all purpose transaction database, used to store data records
 * associated with a unique key.
 */

#pike __REAL_VERSION__

constant cvs_id = "$Id: module.pmod,v 1.28 2004/01/11 00:39:08 nilsson Exp $";

#define ERR(msg) error( "(Yabu) "+msg+"\n" )
#define IO_ERR(msg) error( "(Yabu) %s, %s (%d)\n",msg,strerror(errno()),errno() )
#define WARN(msg) werror(msg)
#define DEB(msg) /* werror(msg) */
#if constant(hash_7_0)
#define hash hash_7_0
#endif

#define CHECKSUM(s) (hash(s) & 0xffffffff)

#if constant(thread_create)
#define THREAD_SAFE
#define LOCK() do { object key___; catch(key___=mutex_lock())
#define UNLOCK() key___=0; } while(0)
#define INHERIT_MUTEX static inherit Thread.Mutex:mutex; function(int(0..2)|void:object) mutex_lock = mutex::lock;
#else
#undef  THREAD_SAFE
#define LOCK() do {
#define UNLOCK() } while(0)
#define INHERIT_MUTEX
#endif





/*
 * ProcessLock handles lock files with PID:s. It is
 * not 100 % safe, but much better than nothing.
 *
 * ProcessLock will throw an error if the lock could not be obtained.
 */
static private class ProcessLock {
  static private string lock_file;
  static private int have_lock = 0;

  static private int get_locked_pid()
  {
    return (int) (Stdio.read_file(lock_file)||"0");
  }

  void destroy()
  {
    /* Release PID lock when the object is destroyed. */
    if(have_lock)
      rm(lock_file);
  }

  void create(string _lock_file, string mode)
  {
    lock_file = _lock_file;

    for(int tryout = 0; tryout < 3; tryout++) {
      object f = Stdio.File();
      
      if(f->open(lock_file, "cxw")) {
	/* We're being paranoid... */
	mixed err = catch {
	  string s = sprintf("%d", getpid());
	  if(f->write(s) != sizeof(s))
	    ERR("Failed to write lock-file");
	  f->close();
	  if(get_locked_pid() != getpid())
	    ERR("Failed to reread lock-file");
	  
	  /* We now have the lock! */
	  have_lock = 1;
	  return;
	};
	rm(lock_file);
	throw(err);
      }

      if(!file_stat(combine_path(lock_file, "../")))
	ERR(sprintf("The database does not exist (use the `c' flag)."));
      
      /* Failed to obtain lock, now trying some obscure techniques... */
      int pid = get_locked_pid();
      if(pid && kill(pid, 0))
	ERR(sprintf("Out-locked by PID %d", pid));
      
      if(!rm(lock_file))
	ERR("Lock removal failure (insufficient permissions?)");

      sleep(has_value(mode, "Q") ? 0.5 : 10);
    }
    ERR("Lock tryout error (insufficient permissions?)");
  }
}





/*
 * With logging enabled, nearly everything Yabu does will
 * be put in a log file (not yet though...).
 *
 */
class YabuLog {
  int pid;
  object f = Stdio.File();
  
  void log(string ... entries)
  {
    
  }
  
  void create(string logfile)
  {
    pid = getpid();
    if(!f->open(logfile, "caw"))
      ERR("Cannot open log file.");
  }
}




/*
 * FileIO handles all basic file operations in Yabu.
 *
 */
static private class FileIO {
  INHERIT_MUTEX
  static private inherit Stdio.File:file;
  static private string filename, filemode;

  static private int mask = 0;

  static private void seek(int offset)
  {
    if(offset < 0 || file::seek(offset) == -1)
      ERR("seek failed");
  }

  string read_at(int offset, int|void size)
  {
    LOCK();
    seek(offset);
    mixed s = size?file::read(size):file::read();
    if(!stringp(s))
      ERR("read failed");
    return s;
    UNLOCK();
  }

  void write_at(int offset, string s)
  {
    LOCK();
    seek(offset);
    while(sizeof(s)) {
      int n = file::write(s);
      if(n < 0 && !(<11,12,16,24,28,49>)[file::errno()])
	ERR(strerror(file::errno()));
      if(n == sizeof(s))
	break;
      s = s[n..];
      if(n<0)
	WARN(strerror(file::errno())+" (sleeping)");
      else
	WARN("disk seems to be full. (sleeping)");
      sleep(1);
    }
    UNLOCK();
  }

  void file_close()
  {
    file::close();
  }

  void file_open(string _filename)
  {
    filename = _filename;
    if(!file::open(filename, filemode))
      ERR(strerror(file::errno()));
  }
  
  void create(string _filename, string _filemode)
  {
    file::create();

    filename = _filename;
    filemode = _filemode;
    file_open(filename);
  }
}





/*
 * The Yabu chunk.
 *
 */
class Chunk {
  INHERIT_MUTEX
  static private inherit FileIO:file;

  static private object parent;
  static private int magic, compress, write;
  static private string start_time, filename;

  /* Point in file from which new chunks can be allocated. */
  static private int eof = 0;
  
  static private mapping frees = ([]), keys = ([]);

  /* Escape special characters used for synchronizing
   * the contents of Yabu files.
   */
  static private string escape(string s)
  {
    return replace(s, ({ "\n", "%" }), ({ "%n", "%p" }));
  }

  static private string descape(string s)
  {
    return replace(s, ({ "%n", "%p" }), ({ "\n", "%" }));
  }

  /*
   * Encode/decode Yabu numbers (8 digit hex numbers).
   */
  static private string encode_num(int x)
  {
    string s = sprintf("%08x", x);
    if(sizeof(s) != 8)
      ERR("Encoded number way too large ("+s+")");
    return s;
  }

  static private int decode_num(string s, int offset)
  {
    int x;
    if(sizeof(s) < offset+8)
      ERR("chunk short");
    if(sscanf(s[offset..offset+7], "%x", x) != 1)
      ERR("chunk number decode error");
    return x;
  }

  /*
   * Encode/decode Yabu keys.
   */
  static private string encode_key(int offset, int type)
  {
    return encode_num(offset)+":"+encode_num(type);
  }

#define DECODE_KEY(key, offset, type) \
    { if(sizeof(key) != 17 || sscanf(key, "%x:%x", offset, type) != 2) \
         ERR("Key not valid"); }

  /* This magic string of characters surrounds all chunk in order to
   * check if the same magic appears on both sides of the chunk contents.
   */
  static private string next_magic()
  {
    return start_time + encode_num(magic++);
  }

  /* The chunk block allocation policy. By using fixed chunk sizes, reuse
   * of empty chunks is encouraged.
   */
  static private int find_nearest_2x(int num)
  {
    for(int b=4;b<30;b++) if((1<<b) >= num) return (1<<b);
    ERR("Chunk too large (max 1 gigabyte)");
  }

  /* Generate the null chuck, which is the same as the empty chunk. */
  static private mapping null_chunk(int t)
  {
    string entry = "";
    string magic = next_magic();
    string type = encode_num(t);
    string checksum = encode_num(CHECKSUM(entry));
    string size = encode_num(sizeof(entry));
    
    return ([ "type":t,
	      "entry":("\n"+magic+type+checksum+size+entry+"%m"+magic)]);
  }

  /*
   * Encode/decode chunks.
   */
  static private mapping encode_chunk(mixed x)
  {
    string entry = encode_value(x);
#if constant(Gz.inflate)
    if(compress)
      catch { entry = Gz.deflate()->deflate(entry); };
#endif
    entry = escape(entry);
    string magic = next_magic();
    string checksum = encode_num(CHECKSUM(entry));
    string size = encode_num(sizeof(entry));

    entry = checksum+size+entry+"%m"+magic;
    int t = find_nearest_2x(1+16+8+sizeof(entry));
    string type = encode_num(t);
    entry = "\n"+magic+type+entry;
    return ([ "type":t, "entry":entry ]);
  }

  static private mapping decode_chunk(string chunk)
  {
    mapping m = ([]);

    if(!sizeof(chunk) || chunk[0] != '\n')
      ERR("No chunk start");
    chunk = chunk[1..];
    m->magic = chunk[0..15];
    m->type = decode_num(chunk, 16);
    m->checksum = decode_num(chunk, 24);
    m->size = decode_num(chunk, 32);
    chunk = chunk[40..];

    m->entry = chunk[0..m->size-1];
    if(sizeof(m->entry) < m->size)
      ERR("Chunk too small");
    if(CHECKSUM(m->entry) != m->checksum)
      ERR("Chunk checksum error");
    if(chunk[m->size..m->size+1] != "%m")
      ERR("No magic");
    if(m->magic != chunk[m->size+2..m->size+17])
      ERR("Magic diff");

    if(m->size) {
      m->entry = descape(m->entry);
#if constant(Gz.inflate)
      catch { m->entry = Gz.inflate()->inflate(m->entry); };
#endif
      m->entry = decode_value(m->entry);
    }

    return m;
  }

  /* Allocate chunks by reuse or from to the end of the file. */
  static private int allocate_chunk(int type, mapping m)
  {
    array f;
    if(f = frees[type]) {
      if(sizeof(f) > 1)
	frees[type] = f[1..];
      else
	m_delete(frees, type);
      return f[0];
    }
    int x = eof;
    if(eof < 0 && 0 <= eof+type)
      ERR("Database too large!");
    eof += type;
    return x;
  }

  /* Perform consistency check. Returns 0 for failure, otherwise success. */
  static private int consistency()
  {
    multiset k = mkmultiset(indices(keys));
    foreach(indices(frees), int type)
      foreach(frees[type], int offset)
	if(k[encode_key(offset, type)])
	  return 0;
    return 1;
  }
  
  array(string) list_keys()
  {
    LOCK();
    return indices(keys);
    UNLOCK();
  }

  string set(mixed x)
  {
    LOCK();
    if(!write)
      ERR("Cannot set in read mode");
    mapping m = encode_chunk(x);
    int offset = allocate_chunk(m->type, m);
    string key = encode_key(offset, m->type);
    file::write_at(offset, m->entry);
    keys[key] = offset;
    return key;
    UNLOCK();
  }

  mixed get(string key, void|int attributes)
  {
    LOCK();
    if(!attributes && zero_type(keys[key])) {
      if(attributes)
	return 0;
      else
	ERR(sprintf("Unknown key '%O'", key));
    }

    int offset, type;
    DECODE_KEY(key, offset, type);
    
    mapping m = decode_chunk(file::read_at(offset, type));
    if(m->size == 0) {
      if(attributes)
	return 0;
      else
	ERR(sprintf("Cannot decode free chunk! [consistency status: %s]",
		    consistency()?"#OK":"#FAILURE"));
    }
    if(attributes)
      return m;
    return m->entry;
    UNLOCK();
  }

  void free(string key)
  {
    LOCK();
    if(!write)
      ERR("Cannot free in read mode");
    if(zero_type(keys[key]))
      ERR(sprintf("Unknown key '%O'", key));

    int offset, type;
    DECODE_KEY(key, offset, type);

    m_delete(keys, key);
    file::write_at(offset, null_chunk(type)->entry);
    frees[type] = (frees[type]||({})) | ({ offset });
    UNLOCK();
  }

  mapping state(array|void almost_free)
  {
    LOCK();
    mapping m_frees = copy_value(frees);
    mapping m_keys = copy_value(keys);
    foreach(almost_free||({}), string key) {
      if(zero_type(keys[key]))
	ERR(sprintf("Unknown state key '%O'", key));

      int offset, type;
      DECODE_KEY(key, offset, type);
      m_frees[type] = (m_frees[type]||({})) | ({ offset });
      m_delete(m_keys, key);
    }
    return copy_value(([ "eof":eof, "keys":m_keys, "frees":m_frees ]));
    UNLOCK();
  }

  void purge()
  {
    LOCK();
    if(!write)
      ERR("Cannot purge in read mode");
    file::file_close();
    rm(filename);
    keys = 0;
    frees = 0;
    write = 0;
    filename = 0;
    UNLOCK();
  }

  void move(string new_filename)
  {
    LOCK();
    if(!write)
      ERR("Cannot move in read mode");
    file::file_close();
    if(!mv(filename, new_filename))
      IO_ERR("Move failed");
    filename = new_filename;
    file::file_open(filename);
    UNLOCK();
  }
  
  void destroy()
  {
    if(parent && write)
      parent->sync();
  }

  void create(string filename_in, string mode,
	      object|void parent_in, mapping|void m)
  {
    magic = 0;
    filename = filename_in;
    parent = parent_in;
    start_time = encode_num(time());

    if(search(mode, "C")+1)
      compress = 1;
    if(search(mode, "w")+1) {
      write = 1;
      file::create(filename, "cwr");
    } else
      file::create(filename, "r");

    if(m) {
      eof = m->eof;
      keys = m->keys||([]);
      frees = m->frees||([]);
    } else {
      string s;
      array offsets = ({});
      int n, offset = 0;
      while(sizeof(s = file::read_at(offset, 4096))) {
	n = -1;
	while((n = search(s, "\n", n+1)) >= 0)
	  offsets += ({ offset+n });
	offset += 4096;
      }

      eof = 0;
      for(int i = 0; i < sizeof(offsets); i++) {
	int offset = offsets[i];
	int size = (i+1 < sizeof(offsets)?offsets[i+1]-offset:0);
	string key = encode_key(offset, size);
	if(!size || size == find_nearest_2x(size)) {
	  mapping m = get(key, 1);
	  if(!m) {
	    if(size) {
	      frees[size] = (frees[size]||({})) + ({ offset });
	      eof = offset+size;
	    }
	  } else {
	    keys[encode_key(offset, m->type)] = offset;
	    eof = offset+m->type;
	  }
	}
      }
    }
  }
}





/*
 * The transaction table.
 *
 */
class Transaction {
  static private int id;
  static private object table, keep_ref;

  void sync()
  {
    table->sync();
  }
  
  void destroy()
  {
    if(table)
      table->t_destroy(id);
  }
  
  mixed set(string handle, mixed x)
  {
    return table->t_set(id, handle, x);
  }

  mixed get(string handle)
  {
    return table->t_get(id, handle);
  }

  mixed delete(string handle)
  {
    return table->t_delete(id, handle);
  }

  array list_keys()
  {
    return table->t_list_keys(id);
  }

  void commit()
  {
    table->t_commit(id);
  }

  void rollback()
  {
    table->t_rollback(id);
  }

  mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  mixed `[](string handle)
  {
    return get(handle);
  }

  array _indices()
  {
    return list_keys();
  }

  array _values()
  {
    return map(_indices(), `[]);
  }

  void create(object table_in, int id_in, object keep_ref_in)
  {
    table = table_in;
    id = id_in;
    keep_ref = keep_ref_in;
  }
}





/*
 * The basic Yabu table.
 *
 */
class Table {
  INHERIT_MUTEX
  static private object index, db, lock_file;

  static private string mode, filename;
  static private mapping handles, changes;
  static private mapping t_start, t_changes, t_handles, t_deleted;
  static private int sync_timeout, write, dirty, magic, id = 0x314159;

  static private void modified()
  {
    dirty++;
    if(sync_timeout && dirty >= sync_timeout)
      sync();
  }

  void sync()
  {
    LOCK();
    if(!write || !dirty) return;

    array almost_free = db->list_keys() - values(handles);
    string key = index->set(([ "db":db->state(almost_free),
			       "handles":handles ]));

    foreach(index->list_keys() - ({ key }), string k)
      index->free(k);

    array working = ({});
    if(t_handles)
      foreach(indices(t_handles), int id)
	working += values(t_handles[id]);
    foreach(almost_free - working, string k)
      db->free(k);

    dirty = 0;
    UNLOCK();
  }

  int reorganize(float|void ratio)
  {
    LOCK();
    if(!write) ERR("Cannot reorganize in read mode");

    ratio = ratio || 0.70;

    /* Check if the level of usage is above the given ratio. */
    if(ratio < 1.0) {
      mapping st = this->statistics();
      float usage = (float)st->used/(float)(st->size||1);
      if(usage > ratio)
	return 0;
    }
    
    /* Remove old junk, just in case. */
    rm(filename+".opt");
    
    /* Create new database. */
    object opt = Chunk(filename+".opt", mode, this, ([]));

    /* Remap all ordinary handles. */
    mapping new_handles = ([]);
    foreach(indices(handles), string handle)
      new_handles[handle] = opt->set(db->get(handles[handle]));
    
    /* Remap all transaction handles. */
    mapping new_t_handles = ([]);
    foreach(indices(t_handles), int id) {
      new_t_handles[id] = ([]);
      foreach(indices(t_handles[id]), string t_handle)
	new_t_handles[id][t_handle] =
	  opt->set(db->get(t_handles[id][t_handle]));
    }

    /* Switch databases. */
    db->purge();
    index->purge();
    opt->move(filename+".chk");
    handles = new_handles;
    t_handles = new_t_handles;
    db = opt;
    index = Chunk(filename+".inx", mode, this);

    /* Reconstruct db and index. */
    modified();
    sync();
    
    return 1;
    UNLOCK();
  }
  
  static private int next_magic()
  {
    return magic++;
  }

  static private mixed _set(string handle, mixed x, mapping handles)
  {
    if(!write) ERR("Cannot set in read mode");

    handles[handle] = db->set(([ "handle":handle, "entry":x ]));
    // modified();
    return x;
  }

  static private mixed _get(string handle, mapping handles)
  {
    return handles[handle]?db->get(handles[handle])->entry:0;
  }

  void delete(string handle)
  {
    LOCK();
    if(!write) ERR("Cannot delete in read mode");
    if(!handles[handle]) ERR(sprintf("Unknown handle '%O'", handle));

    m_delete(handles, handle);
    if(changes)
      changes[handle] = next_magic();
    modified();
    UNLOCK();
  }

  mixed set(string handle, mixed x)
  {
    LOCK();
    if(!write) ERR("Cannot set in read mode");
    if(changes)
      changes[handle] = next_magic();
    modified();
    return _set(handle, x, handles);
    UNLOCK();
  }

  mixed get(string handle)
  {
    LOCK();
    return _get(handle, handles);
    UNLOCK();
  }

  //
  // Transactions.
  //
  mixed t_set(int id, string handle, mixed x)
  {
    LOCK();
    if(!write) ERR("Cannot set in read mode");
    if(!t_handles[id]) ERR("Unknown transaction id");

    t_changes[id][handle] = t_start[id];
    return _set(handle, x, t_handles[id]);
    UNLOCK();
  }

  mixed t_get(int id, string handle)
  {
    LOCK();
    if(!t_handles[id]) ERR("Unknown transaction id");

    t_changes[id][handle] = t_start[id];
    if(t_deleted[id][handle])
      return 0;
    return _get(handle, t_handles[id][handle]?t_handles[id]:handles);
    UNLOCK();
  }

  void t_delete(int id, string handle)
  {
    LOCK();
    if(!write) ERR("Cannot delete in read mode");
    if(!t_handles[id]) ERR("Unknown transaction id");

    t_deleted[id][handle] = 1;
    t_changes[id][handle] = t_start[id];
    if(t_handles[id][handle])
      m_delete(t_handles[id], handle);
    UNLOCK();
  }

  void t_commit(int id)
  {
    LOCK();
    if(!write) ERR("Cannot commit in read mode");
    if(!t_handles[id]) ERR("Unknown transaction id");

    foreach(indices(t_changes[id]), string handle)
      if(t_changes[id][handle] < changes[handle])
	ERR("Transaction conflict");
    
    foreach(indices(t_handles[id]), string handle) {
      changes[handle] = next_magic();
      handles[handle] = t_handles[id][handle];
    }

    foreach(indices(t_deleted[id]), string handle) {
      changes[handle] = next_magic();
      m_delete(handles, handle);
    }
    
    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);
    modified();
    UNLOCK();
  }

  void t_rollback(int id)
  {
    LOCK();
    if(!t_handles[id]) ERR("Unknown transaction id");

    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);
    UNLOCK();
  }

  array t_list_keys(int id)
  {
    LOCK();
    if(!t_handles[id]) ERR("Unknown transaction id");

    return (Array.uniq(indices(handles) + indices(t_handles[id])) -
	    indices(t_deleted[id]));
    UNLOCK();
  }

  void t_destroy(int id)
  {
    LOCK();
    if(!t_handles[id]) ERR("Unknown transaction id");

    m_delete(t_start, id);
    m_delete(t_changes, id);
    m_delete(t_handles, id);
    m_delete(t_deleted, id);
    UNLOCK();
  }

  object transaction(object|void keep_ref)
  {
    LOCK();
    if(!changes) ERR("Transactions are not enabled");

    id++;
    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);
    UNLOCK();
    return Transaction(this, id, keep_ref);
  }
  //
  //
  //

  array list_keys()
  {
    LOCK();
    return indices(handles);
    UNLOCK();
  }

  //
  // Interface functions.
  //

  void sync_schedule()
  {
    remove_call_out(sync_schedule);
    sync();
    call_out(sync_schedule, 120);
  }

  mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  mixed `[](string handle)
  {
    return get(handle);
  }

  void destroy()
  {
    sync();
    destruct(index);
    destruct(db);
    remove_call_out(sync_schedule);
  }

  void _destroy()
  {
    write = 0;
    destruct(this);
  }

  void close()
  {
    sync();
    _destroy();
  }

  void purge()
  {
    LOCK();
    if(!write) ERR("Cannot purge in read mode");

    write = 0;
    rm(filename+".inx");
    rm(filename+".chk");
    destruct(this);
    UNLOCK();
  }

  array _indices()
  {
    return list_keys();
  }

  array _values()
  {
    return map(_indices(), `[]);
  }

  mapping(string:string|int) statistics()
  {
    LOCK();
    mapping m = ([ "keys":sizeof(handles),
		   "size":Stdio.file_size(filename+".inx")+
		          Stdio.file_size(filename+".chk"),
		   "used":`+(@Array.map((values(handles) |
					 (t_handles && sizeof(t_handles)?
					  `+(@Array.map(values(t_handles),
							values)):({}))) +
					index->list_keys(),
					lambda(string s)
					{ int x;
					  sscanf(s, "%*x:%x", x);
					  return x; })) ]);

    m->size = max(m->size, m->used);
    return m;
    UNLOCK();
  }
  
  void create(string filename_in, string _mode, object _lock_file)
  {
    filename = filename_in;
    mode = _mode;
    lock_file = _lock_file;

    if(search(mode, "w")+1)
      write = 1;
    if(search(mode, "t")+1)
      changes = ([]);
    t_start = ([]);
    t_changes = ([]);
    t_handles = ([]);
    t_deleted = ([]);

    index = Chunk(filename+".inx", mode, this);
    mapping m = ([]);
    if(sizeof(index->list_keys()))
      m = index->get(sort(index->list_keys())[-1]);
    handles = m->handles||([]);
    db = Chunk(filename+".chk", mode, this, m->db||([]));

    if(write) {
      sync_timeout = 128;
      if(search(mode, "s")+1)
	sync_schedule();
    }
  }
}





/*
 * The shadow table.
 *
 */
class _Table {
  static object table;
  static string handle;
  static function table_destroyed;

  void sync()
  {
    table->sync();
  }
  
  void delete(string handle)
  {
    table->delete(handle);
  }
  
  mixed set(string handle, mixed x)
  {
    return table->set(handle, x);
  }
  
  mixed get(string handle)
  {
    return table->get(handle);
  }
  
  array list_keys()
  {
    return table->list_keys();
  }
  
  object transaction()
  {
    return table->transaction(this);
  }
  
  void close()
  {
    destruct(this);
  }
  
  void purge()
  {
    table->purge();
  }

  mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  mixed `[](string handle)
  {
    return get(handle);
  }

  array _indices()
  {
    return list_keys();
  }  
  
  array _values()
  {
    return map(_indices(), `[]);
  }

  void destroy()
  {
    if(table_destroyed)
      table_destroyed(handle);
  }

  int reorganize(float|void ratio)
  {
    return table->reorganize(ratio);
  }
    
  /*
   * Compile table statistics.
   */
  static private string st_keys(int size, mapping m)
  {
    return sprintf("%4d", size);
  }
  
  static private string st_size(int size, mapping m)
  {
    return sprintf("%7.3f Mb", (float)size/(1024.0*1024.0));
    
    string r = (string)size;
    int e = (int) (log((float)(size||1))/log(1024.0));
    
    if(`<=(1, e, 3))
      r = sprintf("%7.2f",(float)size/(float)(((int)pow(1024.0,(float)e))||1));
    return sprintf("%s %s", r, ([ 1:"Kb", 2:"Mb", 3:"Gb" ])[e]||"b ");
  }

  static private string st_used(int used, mapping m)
  {
    return sprintf("%3d %%", (int) (100.0*(float)used/(float)m->size));
  }
  
  mapping(string:string|int) statistics()
  {
    return table->statistics();
  }

  string ascii_statistics()
  {
    mapping m = statistics();
    return "["+Array.map(sort(indices(m)),
			 lambda(string inx, mapping m)
			 {
			   mapping f = ([ "keys":st_keys,
					  "size":st_size,
					  "used":st_used ]);
			   return sprintf("%s:%s", inx,f[inx]?f[inx](m[inx],m):
					  (string)m[inx]);
			 }, m)*"   "+"] \""+handle+"\"";
  }

  void create(string _handle, object _table, function _table_destroyed)
  {
    handle = _handle;
    table = _table;
    table_destroyed = _table_destroyed;
  }
}





/*
 * The Yabu database object itself.
 *
 */
class db {
  INHERIT_MUTEX

  static string dir, mode;
  static mapping tables = ([]), table_refs = ([]);
  static int write, id;
  static object lock_file;

  void sync()
  {
    LOCK();
    foreach(values(tables), object o)
      if(o)
	o->sync();
    UNLOCK();
  }
  
  static void _table_destroyed(string handle)
  {
    LOCK();
    table_refs[handle]--;
    // DEB(sprintf("### refs[%s]-- = %d\n", handle, table_refs[handle]));
    DEB(sprintf("! tables '%s': %d (%s)\n",
		reverse(reverse(dir/"/")[0..1])*"/", sizeof(tables), handle));
    if(!table_refs[handle]) {
      // DEB(sprintf("### zonking[%s]\n", handle));
      destruct(tables[handle]);
      m_delete(tables, handle);
      m_delete(table_refs, handle);
    }
    UNLOCK();
  }

  object table(string handle)
  {
    LOCK();
    if(!tables[handle])
      tables[handle] = Table(dir+"/"+handle, mode, lock_file);
    table_refs[handle]++;
    // DEB(sprintf("### refs[%s]++ = %d\n", handle, table_refs[handle]));
    DEB(sprintf("# tables '%s': %d (%s)\n",
		reverse(reverse(dir/"/")[0..1])*"/", sizeof(tables), handle));
    return _Table(handle, tables[handle], _table_destroyed);
    UNLOCK();
  }

  mixed `[](string handle)
  {
    return table(handle);
  }

  array list_tables()
  {
    LOCK();
    return Array.map(glob("*.chk", get_dir(dir)||({})),
		     lambda(string s) { return s[0..sizeof(s)-5]; });
    UNLOCK();
  }

  array _indices()
  {
    return list_tables();
  }

  array _values()
  {
    return map(_indices(), `[]);
  }

  /* Remove maximum one level of directories and files. */
  static private void level2_rm(string f)
  {
    if(has_suffix(f, "/"))
      f = f[..sizeof(f)-2];
    if(Stdio.is_dir(f))
      foreach(get_dir(f)||({}), string file)
	rm(f+"/"+file);
    rm(f);
  }

  void purge()
  {
    LOCK();
    foreach(values(tables), object o)
      if(o)
	destruct(o);
    level2_rm(dir);
    destruct(this);
    UNLOCK();
  }

  static void mkdirhier(string from)
  {
    string a, b;
    array f;

    f=(from/"/");
    b="";

    foreach(f[0..sizeof(f)-2], a)
    {
      mkdir(b+a);
      b+=a+"/";
    }
  }

  void destroy()
  {
    sync();
    foreach(values(tables), object o)
      if(o)
	destruct(o);
    destruct(lock_file);
  }

  void close()
  {
    destruct(this);
  }
  
  int reorganize(float|void ratio)
  {
    int r = 0;
    foreach(list_tables(), string name)
      r |= table(name)->reorganize(ratio);
    return r;
  }
    
  mapping(string:int) statistics()
  {
    mapping m = ([]);
    foreach(list_tables(), string name)
      m[name] = table(name)->statistics();
    return m;
  }
    
  string ascii_statistics()
  {
    string r = "";
    foreach(list_tables(), string name)
      r += table(name)->ascii_statistics()+"\n";
    return r;
  }
    
  void create(string dir_in, string mode_in)
  {
    atexit(close);
    
    dir = dir_in;
    mode = mode_in;

    if(search(mode, "w")+1) {
      write = 1;
      if(search(mode, "c")+1)
	mkdirhier(dir+"/");
    }
    lock_file = ProcessLock(dir+"/lock.pid", mode);
  }
}




/*
 * Special extra bonus. This database is optimized for lots of very small
 * data records.
 */
class LookupTable {
  INHERIT_MUTEX
  
  static private int minx;
  static private object table;

  static private string h(string s)
  {
    return (string)(hash(s) & minx);
  }
  
  mixed get(string handle)
  {
    LOCK();
    return (table->get(h(handle))||([]))[handle];
    UNLOCK();
  }

  mixed set(string handle, mixed x)
  {
    LOCK();
    mapping m = table->get(h(handle))||([]);
    m[handle] = x;
    return table->set(h(handle), m)[handle];
    UNLOCK();
  }

  mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  mixed `[](string handle)
  {
    return get(handle);
  }

  void purge()
  {
    LOCK();
    table->purge();
    UNLOCK();
  }
  
  void sync()
  {
    LOCK();
    table->sync();
    UNLOCK();
  }
  
  void create(string filename, string mode, int _minx)
  {
    minx = _minx;
    table = Table(filename, mode, 0);
  }
}




/*
 * The lookup database.
 *
 */
class lookup {
  inherit db;
  static private int minx;
  
  object table(string handle)
  {
    LOCK();
    return (tables[handle] =
	    tables[handle]||LookupTable(dir+"/"+handle, mode, minx));
    UNLOCK();
  }

  void create(string dir, string mode, mapping|void opt)
  {
    ::create(dir, (mode-"t"));
    minx = (opt||([]))->index_size || 0x7ff;
    if(!sscanf(Stdio.read_bytes(dir+"/hs")||"", "%4c", minx))
      Stdio.write_file(dir+"/hs", sprintf("%4c", minx));
  }
}
