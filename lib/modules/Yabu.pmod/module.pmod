/* Yabu. By Fredrik Noring, 1999.
 *
 * Yabu is an all purpose transaction database, used to store data records
 * associated with a unique key.
 */

//! Yabu is an all purpose transaction database written in pike, used
//! to store data records associated with a unique key.

//! @ignore
#pike __REAL_VERSION__

#define ERR(msg,args...) error( "(Yabu) "+msg+"\n", args )
#define IO_ERR(msg) error( "(Yabu) %s, %s (%d)\n",msg,strerror(errno()),errno() )
#define WARN(msg) werror(msg)
#if constant(hash_7_0)
#define hash hash_7_0
#endif

#define CHECKSUM(s) (hash(s) & 0xffffffff)

#if constant(thread_create)
#define THREAD_SAFE
#define LOCK() do { object key___; catch(key___=mutex_lock())
#define UNLOCK() key___=0; } while(0)
#define INHERIT_MUTEX protected inherit Thread.Mutex:mutex; function(int(0..2)|void:object) mutex_lock = mutex::lock;
#else
#undef  THREAD_SAFE
#define LOCK() do {
#define UNLOCK() } while(0)
#define INHERIT_MUTEX
#endif
//! @endignore



/*
 * ProcessLock handles lock files with PID:s. It is
 * not 100 % safe, but much better than nothing.
 *
 * ProcessLock will throw an error if the lock could not be obtained.
 */
protected private class ProcessLock {
  protected private string lock_file;
  protected private int have_lock = 0;

  protected private int get_locked_pid()
  {
    return (int)Stdio.read_file(lock_file);
  }

  protected void destroy()
  {
    /* Release PID lock when the object is destroyed. */
    if(have_lock)
      rm(lock_file);
  }

  protected void create(string lock_file, string mode)
  {
    this::lock_file = lock_file;

    for(int tryout = 0; tryout < 3; tryout++) {
      Stdio.File f = Stdio.File();

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
	ERR("The database does not exist (use the \"c\" flag).");

      /* Failed to obtain lock, now trying some obscure techniques... */
      int pid = get_locked_pid();
      if(pid && kill(pid, 0))
	ERR("Out-locked by PID %d", pid);

      if(!rm(lock_file))
	ERR("Lock removal failure (insufficient permissions?)");

      sleep(has_value(mode, "Q") ? 0.5 : 10);
    }
    ERR("Lock tryout error (insufficient permissions?)");
  }
}



/*
 * FileIO handles all basic file operations in Yabu.
 *
 */
protected private class FileIO {
  //! @ignore
  INHERIT_MUTEX;
  //! @endignore
  protected private inherit Stdio.File:file;
  protected private string filename, filemode;

  protected private int mask = 0;

  protected private void seek(int offset)
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
	ERR("%s [%d]", strerror(file::errno()), file::errno());
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

  void file_open(string filename)
  {
    this::filename = filename;
    if(!file::open(filename, filemode))
      ERR(strerror(file::errno()));
  }

  protected void create(string filename, string filemode)
  {
    file::create();

    this::filename = filename;
    this::filemode = filemode;
    file_open(filename);
  }
}


/*
 * The Yabu chunk.
 *
 */
class Chunk {
  //! @ignore
  INHERIT_MUTEX;
  //! @endignore
  private inherit FileIO:file;

  private Table parent;
  private int magic, compress, write;
  private string start_time, filename;

  /* Point in file from which new chunks can be allocated. */
  private int eof = 0;

  private mapping frees = ([]), keys = ([]);

  /* Escape special characters used for synchronizing
   * the contents of Yabu files.
   */
  private string escape(string s)
  {
    return replace(s, ({ "\n", "%" }), ({ "%n", "%p" }));
  }

  private string descape(string s)
  {
    return replace(s, ({ "%n", "%p" }), ({ "\n", "%" }));
  }

  /*
   * Encode/decode Yabu numbers (8 digit hex numbers).
   */
  private string encode_num(int x)
  {
    string s = sprintf("%08x", x);
    if(sizeof(s) != 8)
      ERR("Encoded number way too large (%O)", s);
    return s;
  }

  private int decode_num(string s, int offset)
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
  private string encode_key(int offset, int type)
  {
    return encode_num(offset)+":"+encode_num(type);
  }

#define DECODE_KEY(key, offset, type) \
    { if(sizeof(key) != 17 || sscanf(key, "%x:%x", offset, type) != 2) \
         ERR("Key not valid"); }

  /* This magic string of characters surrounds all chunk in order to
   * check if the same magic appears on both sides of the chunk contents.
   */
  private string next_magic()
  {
    return start_time + encode_num(magic++);
  }

  /* The chunk block allocation policy. By using fixed chunk sizes, reuse
   * of empty chunks is encouraged.
   */
  private int find_nearest_2x(int num)
  {
    for(int b=4;b<30;b++) if((1<<b) >= num) return (1<<b);
    ERR("Chunk too large (max 1 gigabyte)");
  }

  /* Generate the null chuck, which is the same as the empty chunk. */
  private mapping null_chunk(int t)
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
  private mapping encode_chunk(mixed x)
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

  private mapping decode_chunk(string chunk)
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
  private int allocate_chunk(int type, mapping m)
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
  private int consistency()
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
    if(!attributes && !has_index(keys, key)) {
      if(attributes)
	return 0;
      else
	ERR("Unknown key %O, keys: %O", key, keys);
    }

    int offset, type;
    DECODE_KEY(key, offset, type);

    mapping m = decode_chunk(file::read_at(offset, type));
    if(m->size == 0) {
      if(attributes)
	return 0;
      else
	ERR("Cannot decode free chunk! [consistency status: %s]",
            consistency()?"#OK":"#FAILURE");
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
    if(!has_index(keys, key))
      ERR("Unknown key %O", key);

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
      if(!has_index(keys, key))
	ERR("Unknown state key %O", key);

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

  protected void destroy()
  {
    if(parent && write)
      parent->sync();
  }

  protected void create(string filename, string mode,
                        Table|void parent, mapping|void m)
  {
    magic = 0;
    this::filename = filename;
    this::parent = parent;
    start_time = encode_num(time());

    if(has_value(mode, "C"))
      compress = 1;
    if(has_value(mode, "w")) {
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


class Transaction
//! A transaction. Created by calling transaction() in the table object.
//!
//! It provides the same functions the table does, in addition to
//! @[commit] and @[rollback]. Changes done using the transaction
//! object will not be in the actual table @[commit] is called in the
//! transaction.
{
  private int id;
  private Table table;
  private _Table keep_ref;

  void sync()
  {
    table->sync();
  }

  protected void destroy()
  {
    if(table)
      table->t_destroy(id);
  }

  //! Set @[handle] to x
  mixed set(string handle, mixed x)
  {
    return table->t_set(id, handle, x);
  }

  //! Get the value of @[handle]
  mixed get(string handle)
  {
    return table->t_get(id, handle);
  }

  //! Delete @[handle] from the database
  void delete(string handle)
  {
    table->t_delete(id, handle);
  }

  //! List all keys
  array list_keys()
  {
    return table->t_list_keys(id);
  }

  //! Commit all changes done so far in the transaction to the table
  void commit()
  {
    table->t_commit(id);
  }

  //! Undo all changes done so far in the transaction to the table
  void rollback()
  {
    table->t_rollback(id);
  }

  //! Identical to set
  protected mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  //! Identical to get
  protected mixed `[](string handle)
  {
    return get(handle);
  }

  //! Identical to @[delete]
  protected mixed _m_delete(string handle)
  {
    mixed val = get(handle);
    delete(handle);
    return val;
  }

  //! Identical to list_keys();
  protected array _indices()
  {
    return list_keys();
  }

  //! Identical to get(list_keys()][*]);
  protected array _values()
  {
    return map(_indices(), `[]);
  }

  protected void create(Table table, int id, _Table keep_ref)
  {
    this::table = table;
    this::id = id;
    this::keep_ref = keep_ref;
  }
}


class Table
//! The basic Yabu table
{
  //! @ignore
  INHERIT_MUTEX;
  //! @endignore
  private Chunk index, db;
  private ProcessLock lock_file;

  private string mode, filename;
  private mapping handles, changes;
  private mapping t_start, t_changes, t_handles, t_deleted;
  private int sync_timeout, write, dirty, magic, id = 0x314159;

  private void modified()
  {
    dirty++;
    if(sync_timeout && dirty >= sync_timeout)
      sync();
  }

  //! Synchronize. Usually done automatically
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

  //! Reorganize the on-disk storage, compacting it.
  //!
  //! If @[ratio] is given it is the lowest ratio of useful/total disk
  //! usage that is allowed.
  //!
  //! As an example, if ratio is 0.7 at lest 70% of the on-disk
  //! storage must be live data, if not the reoganization is done.
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
    Chunk opt = Chunk(filename+".opt", mode, this, ([]));

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

  private int next_magic()
  {
    return magic++;
  }

  private mixed _set(string handle, mixed x, mapping handles)
  {
    if(!write) ERR("Cannot set in read mode");

    handles[handle] = db->set(([ "handle":handle, "entry":x ]));
    // modified();
    return x;
  }

  private mixed _get(string handle, mapping handles)
  {
    return handles[handle]?db->get(handles[handle])->entry:0;
  }

  //! Remove a key
  void delete(string handle)
  {
    LOCK();
    if(!write) ERR("Cannot delete in read mode");
    if(!handles[handle]) ERR("Unknown handle %O", handle);

    m_delete(handles, handle);
    if(changes)
      changes[handle] = next_magic();
    modified();
    UNLOCK();
  }

  //! Set a key
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

  //! Get a key
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

  //! @decl Transaction transaction()
  //! Start a new transaction.

  Transaction transaction(_Table|void keep_ref)
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

  //! List all keys
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

  //! Equivalent to @[set]
  protected mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  //! Equivalent to @[get]
  protected mixed `[](string handle)
  {
    return get(handle);
  }

  //! Equivalent to @[delete]
  protected mixed _m_delete(string handle)
  {
    mixed val = get(handle);
    delete(handle);
    return val;
  }

  protected void destroy()
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

  //! Close the table
  void close()
  {
    sync();
    _destroy();
  }

  //! Close and delete the table from disk
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

  //! Equivalent to list_keys()
  protected array _indices()
  {
    return list_keys();
  }

  //! Fetches all keys from disk
  protected array _values()
  {
    return map(_indices(), `[]);
  }


  //! @decl string ascii_statistics()
  //! Return information about all tables in a human redable format

  //! Return information about the table.
  //! @mapping
  //! @member int "keys"
  //!  The number of keys
  //!
  //! @member int "size"
  //!   The on-disk space, in bytes
  //!
  //! @member int used
  //! @endmapping
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

  protected void create(string filename, string mode, ProcessLock lock_file)
  {
    this::filename = filename;
    this::mode = mode;
    this::lock_file = lock_file;

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
class _Table
{
  protected Table table;
  protected string handle;
  protected function table_destroyed;

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

  Transaction transaction()
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

  protected mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  protected mixed `[](string handle)
  {
    return get(handle);
  }

  protected array _indices()
  {
    return list_keys();
  }

  protected array _values()
  {
    return map(_indices(), `[]);
  }

  protected void destroy()
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
  private string st_keys(int size, mapping m)
  {
    return sprintf("%4d", size);
  }

  private string st_size(int size, mapping m)
  {
    return sprintf("%7.3f Mb", (float)size/(1024.0*1024.0));

    string r = (string)size;
    int e = (int) (log((float)(size||1))/log(1024.0));

    if(`<=(1, e, 3))
      r = sprintf("%7.2f",(float)size/(float)(((int)pow(1024.0,(float)e))||1));
    return sprintf("%s %s", r, ([ 1:"Kb", 2:"Mb", 3:"Gb" ])[e]||"b ");
  }

  private string st_used(int used, mapping m)
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

  protected void create(string handle, Table table, function table_destroyed)
  {
    this::handle = handle;
    this::table = table;
    this::table_destroyed = table_destroyed;
  }
}


/*
 * The Yabu database object itself.
 *
 */
class DB
//! A Yabu database instance
{
  //! @ignore
  INHERIT_MUTEX;
  //! @endignore

  protected string dir, mode;
  protected mapping tables = ([]), table_refs = ([]);
  protected int write, id;
  protected ProcessLock lock_file;

  //! Sync all tables
  void sync()
  {
    LOCK();
    foreach(values(tables), Table o)
      if(o)
	o->sync();
    UNLOCK();
  }

  protected void _table_destroyed(string handle)
  {
    LOCK();
    table_refs[handle]--;
    if(!table_refs[handle]) {
      destruct(tables[handle]);
      m_delete(tables, handle);
      m_delete(table_refs, handle);
    }
    UNLOCK();
  }

  //! @decl Table table(string handle)
  //! Return the Table object for the named table

  _Table table(string handle)
  {
    LOCK();
    if(!tables[handle])
      tables[handle] = Table(combine_path(dir, handle), mode, lock_file);
    table_refs[handle]++;
    return _Table(handle, tables[handle], _table_destroyed);
    UNLOCK();
  }

  //! Equivalent to @[table]
  protected mixed `[](string handle)
  {
    return table(handle);
  }

  //! Return a list of all tables in the database
  array(string) list_tables()
  {
    LOCK();
    return Array.map(glob("*.chk", get_dir(dir)||({})),
		     lambda(string s) { return s[..<4]; });
    UNLOCK();
  }

  //! Return a list of all tables
  protected array _indices()
  {
    return list_tables();
  }

  //! Return all tables as an array
  protected array _values()
  {
    return map(_indices(), `[]);
  }

  /* Remove maximum one level of directories and files. */
  private void level2_rm(string f)
  {
    if(has_suffix(f, "/"))
      f = f[..<1];
    if(Stdio.is_dir(f))
      foreach(get_dir(f)||({}), string file)
	rm(combine_path(f, file));
    rm(f);
  }

  //! Delete the database.
  void purge()
  {
    LOCK();
    foreach(values(tables), Table o)
      if(o)
	destruct(o);
    level2_rm(dir);
    destruct(this);
    UNLOCK();
  }

  protected void destroy()
  {
    sync();
    foreach(values(tables), Table o)
      if(o)
	destruct(o);
    destruct(lock_file);
  }

  //! Close the database.
  void close()
  {
    destruct(this);
  }

  //! Call @[Table.reorganize] in all tables
  int reorganize(float|void ratio)
  {
    int r = 0;
    foreach(list_tables(), string name)
      r |= table(name)->reorganize(ratio);
    return r;
  }

  //! Return information about all tables
  mapping(string:int) statistics()
  {
    mapping m = ([]);
    foreach(list_tables(), string name)
      m[name] = table(name)->statistics();
    return m;
  }

  //! Return information about all tables in a human readable format
  string ascii_statistics()
  {
    string r = "";
    foreach(list_tables(), string name)
      r += table(name)->ascii_statistics()+"\n";
    return r;
  }

  //! Open a new or existing databse.
  //!
  //! The @[dir] is the directory the database should be stored in.
  //! It will be created if it does not exist.
  //!
  //! Only one database can be in any given directory.
  //!
  //! The @[mode] specifies the operation mode, and
  //! is a string made up of the desired modes, 'r'=read, 'w'=write
  //! and 'c'=create.
  //!
  //! To open an existing database in read only mode, use "r".
  //!
  //! To open an existing database in read/write mode, use "rw".
  //!
  //! To create a new database, or open an existing one in read write mode, use "rwc".
  protected void create(string dir, string mode)
  {
    atexit(close);

    this::dir = dir;
    this::mode = mode;

    if(search(mode, "w")+1) {
      write = 1;
      if(search(mode, "c")+1)
	Stdio.mkdirhier(dir);
    }
    lock_file = ProcessLock(combine_path(dir, "lock.pid"), mode);
  }
}

__deprecated__ DB db(string a,string b) {
  return DB(a,b);
}

/*
 * Special extra bonus. This database is optimized for lots of very small
 * data records.
 */
class LookupTable
{
  //! @ignore
  INHERIT_MUTEX;
  //! @endignore

  private int minx;
  private Table table;

  private string h(string s)
  {
    return (string)(hash(s) & minx);
  }

  void delete(string handle)
  {
    LOCK();
    mapping m = table->get(h(handle))||([]);
    m_delete(m,handle);
    table->set(h(handle), m);
    UNLOCK();
  }

  mixed get(string handle)
  {
    LOCK();
    return table->get(h(handle))[?handle];
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

  protected mixed `[]=(string handle, mixed x)
  {
    return set(handle, x);
  }

  protected mixed `[](string handle)
  {
    return get(handle);
  }

  protected mixed _m_delete(string handle)
  {
    mixed res = UNDEFINED;
    LOCK();
    if( mapping m = table->get(h(handle)) )
    {
      res = m_delete(m,handle);
      if( sizeof( m ) )
	table->set(h(handle), m);
      else
	table->delete(h(handle));
    }
    UNLOCK();
    return res;
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

  protected void create(string filename, string mode, int minx)
  {
    this::minx = minx;
    table = Table(filename, mode, 0);
  }
}


__deprecated__ LookupDB lookup(string a,string b,mapping|void c) {
  return LookupDB(a,b,c);
}

class LookupDB 
//! This database is optimized for lots of very small data records (a
//! few bytes each, usually), but the API is otherwise identical to
//! the normal @[DB] API.
//! 
//! It will perform badly if used with big records.
//! You also need to know in advance aproximately how many keys there
//! will be, the performance will degrade if way more than the
//! expected number of keys are present.
{
  inherit DB;
  private int minx;
  
  Table|LookupTable table(string handle)
  {
    LOCK();
    return (tables[handle] =
	    tables[handle]||LookupTable(combine_path(dir, handle), mode, minx));
    UNLOCK();
  }

  //! Construct a new lookup table.
  //!
  //! The @[options], if present, can be used to specify the index
  //! hash size.  The bigger that number is, the less memory will be
  //! used given a certain number of actual keys. In general, using
  //! the expected number of entries in the database divided by
  //! 100-1000 is reasonable.
  //!
  //! The supported options are
  //!
  //! @mapping
  //! @member int(1..) index_size
  //! @endmapping
  protected void create(string dir, string mode, mapping|void options)
  {
    ::create(dir, (mode-"t"));
    minx = options?->index_size || 0x7ff;
    string file = combine_path(dir, "hs");
    if(!sscanf(Stdio.read_bytes(file)||"", "%4c", minx))
      Stdio.write_file(file, sprintf("%4c", minx));
  }
}
