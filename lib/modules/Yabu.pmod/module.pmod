// Yabu by Fredrik Noring
// $Id: module.pmod,v 1.4 1999/10/19 22:48:07 noring Exp $

#if constant(thread_create)
#define THREAD_SAFE
#define LOCK() do { object key; catch(key=lock())
#define UNLOCK() key=0; } while(0)
#else
#undef THREAD_SAFE
#define LOCK() do {
#define UNLOCK() } while(0)
#endif

#define ERR(msg) throw(({ "(Yabu) "+msg+"\n", backtrace() }))
#define WARN(msg) werror(msg)
#define DEB(msg) /* werror(msg) */
#define CHECKSUM(s) (hash(s) & 0xffffffff)

static private class FileIO {
  static private inherit Stdio.File:file;

  static private int mask = 0;

  static private array(int) fractionalise(int i)
  {
    if(!mask)
    {
      int size = 0;
      for(mask = 1; mask; size++)
	mask <<= 1;
      mask = ~(0xff << (size-8));
    }
    
    return ({ (i>>8) & mask, i & 0xff });
  }
  
  static private void seek(int offset)
  {
    if(offset < 0)
    {
      int fraction;

      [offset, fraction] = fractionalise(offset);
      if(file::seek(offset, 0x100) == -1 ||
	 (fraction && sizeof(file::read(fraction)) != fraction))
	ERR("seek failed");
    }
    else
      if(file::seek(offset) == -1)
	ERR("seek failed");
  }

  string read_at(int offset, int|void size)
  {
    seek(offset);
    mixed s = size?file::read(size):file::read();
    if(!stringp(s))
      ERR("read failed");
    return s;
  }

  void write_at(int offset, string s)
  {
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
  }

  void create(string filename, string mode)
  {
    file::create();
    if(!file::open(filename, mode))
      ERR(strerror(file::errno()));
  }
}

class Chunk {
  static private inherit FileIO:file;

  static private object parent;
  static private int magic, compress, write;
  static private string start_time, filename;

  static private int eof = 0;
  static private mapping frees = ([]), keys = ([]);

  static private string escape(string s)
  {
    return replace(s, ({ "\n", "%" }), ({ "%n", "%p" }));
  }

  static private string descape(string s)
  {
    return replace(s, ({ "%n", "%p" }), ({ "\n", "%" }));
  }

  static private string encode_num(int x)
  {
    return sprintf("%08x", x);
  }

  static private int decode_num(string s, int offset)
  {
    int x;
    if(sizeof(s) < offset+8)
      ERR("cunk short");
    if(sscanf(s[offset..offset+7], "%x", x) != 1)
      ERR("cunk number decode error");
    return x;
  }

  static private string encode_key(int offset, int type)
  {
    return encode_num(offset)+":"+encode_num(type);
  }

#define DECODE_KEY(key, offset, type) \
    { if(sizeof(key) != 17 || sscanf(key, "%x:%x", offset, type) != 2) \
         ERR("Key not valid"); }

  static private string next_magic()
  {
    return start_time + encode_num(magic++);
  }

  static private int find_nearest_2x(int num)
  {
    for(int b=4;b<32;b++) if((1<<b) >= num) return (1<<b);
  }

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

  static private mapping encode_chunk(mixed x)
  {
    string entry = encode_value(x);
#if constant(Gz.inflate)
    if(compress)
      catch { entry = Gz.deflate()->deflate(entry); };
#endif
    string entry = escape(entry);
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

    if(chunk[0] != '\n')
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
  int consistency()
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
    return indices(keys);
  }

  string set(mixed x)
  {
    mapping m = encode_chunk(x);
    int offset = allocate_chunk(m->type, m);
    string key = encode_key(offset, m->type);
    file::write_at(offset, m->entry);
    keys[key] = offset;
    return key;
  }

  mixed get(string key, void|int attributes)
  {
    if(!attributes && zero_type(keys[key]))
      ERR(sprintf("Unknown key '%O'", key));

    int offset, type;
    DECODE_KEY(key, offset, type);
    
    mapping m = decode_chunk(file::read_at(offset, type));
    if(m->size == 0)
      ERR(sprintf("Cannot decode free chunk! [consistency status: %s]",
                  consistency()?"#OK":"#FAILURE"));
    if(attributes)
      return m;
    return m->entry;
  }

  void free(string key)
  {
    if(zero_type(keys[key]))
      ERR(sprintf("Unknown key '%O'", key));

    int offset, type;
    DECODE_KEY(key, offset, type);

    m_delete(keys, key);
    file::write_at(offset, null_chunk(type)->entry);
    frees[type] = (frees[type]||({})) | ({ offset });
  }

  mapping state(array|void almost_free)
  {
    mapping m_frees = copy_value(frees);
    mapping m_keys = copy_value(keys);
    foreach(almost_free||({}), string key) {
      int offset, type;
      DECODE_KEY(key, offset, type);
      m_frees[type] = (m_frees[type]||({})) | ({ offset });
      m_delete(m_keys, key);
    }
    return copy_value(([ "eof":eof, "keys":m_keys, "frees":m_frees ]));
  }

  void purge()
  {
    if(!write)
      ERR("Cannot purge in read mode");
    rm(filename);
    destruct(this_object());
  }

  void destroy()
  {
    if(parent)
      destruct(parent);
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
	  mapping m;
	  if(catch(m = get(key, 1))) {
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

  void create(object table_in, int id_in, object keep_ref_in)
  {
    table = table_in;
    id = id_in;
    keep_ref = keep_ref_in;
  }
}

class Table {
#ifdef THREAD_SAFE
  static inherit Thread.Mutex;
#endif
  static private object index, db;

  static private string filename;
  static private mapping handles, new, changes;
  static private mapping t_start, t_changes, t_handles, t_deleted;
  static private int sync_timeout, write, dirty, magic, id = 0x314159;

  void sync()
  {
    if(!write || !dirty) return;

    LOCK();
    new = ([]);
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

    dirty = sizeof(working);
    UNLOCK();
  }

  static private int next_magic()
  {
    return magic++;
  }

  static private void modified()
  {
    dirty++;
    if(sync_timeout && dirty >= sync_timeout)
      sync();
  }

  static private mixed _set(string handle, mixed x,
			    mapping handles, mapping new)
  {
    if(!write) ERR("Cannot set in read mode");

    string key = handles[handle];
    if(key && new[key]) {
      m_delete(new, key);
      db->free(key);
    }
    handles[handle] = key = db->set(([ "handle":handle, "entry":x ]));
    new[key] = 1;
    modified();
    return x;
  }

  static private mixed _get(string handle, mapping handles)
  {
    if(!handles[handle])
      return 0;
    return db->get(handles[handle])->entry;
  }

  void delete(string handle)
  {
    if(!write) ERR("Cannot delete in read mode");

    LOCK();
    if(!handles[handle]) ERR(sprintf("Unknown handle '%O'", handle));

    string key = handles[handle];
    m_delete(handles, handle);
    if(changes)
      changes[handle] = next_magic();
    // m_delete(changes, handle);
    if(new[key]) {
      m_delete(new, key);
      db->free(key);
    }
    modified();
    UNLOCK();
  }

  mixed set(string handle, mixed x)
  {
    LOCK();
    if(changes)
      changes[handle] = next_magic();
    return _set(handle, x, handles, new);
    UNLOCK();
  }

  mixed get(string handle)
  {
    LOCK();
    return _get(handle, handles);
    UNLOCK();
  }

  //
  // Transactions
  //
  mixed t_set(int id, string handle, mixed x)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    t_changes[id][handle] = t_start[id];
    mapping t_new = mkmapping(values(t_handles[id]), indices(t_handles[id]));
    return _set(handle, x, t_handles[id], t_new);
    UNLOCK();
  }

  mixed t_get(int id, string handle)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    t_changes[id][handle] = t_start[id];
    if(t_deleted[id][handle])
      return 0;
    return _get(handle, t_handles[id][handle]?t_handles[id]:handles);
    UNLOCK();
  }

  void t_delete(int id, string handle)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    t_deleted[id][handle] = 1;
    t_changes[id][handle] = t_start[id];
    if(t_handles[id][handle]) {
      db->free(t_handles[id][handle]);
      m_delete(t_handles[id], handle);
    }
    UNLOCK();
  }

  void t_commit(int id)
  {
    if(!write) ERR("Cannot commit in read mode");
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    foreach(indices(t_changes[id]), string handle)
      if(t_changes[id][handle] < changes[handle])
	ERR("Transaction conflict");

    mapping tmp_t_changes = copy_value(t_changes);
    mapping tmp_t_handles = copy_value(t_handles);
    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);

    array obsolete = ({});
    foreach(indices(tmp_t_changes[id]), string handle) {
      if(new[handles[handle]])
	obsolete += ({ handles[handle] });
      changes[handle] = next_magic();
      handles[handle] = tmp_t_handles[id][handle];
    }

    foreach(obsolete, string key) {
      m_delete(new, key);
      db->free(key);
    }
    UNLOCK();
  }

  void t_rollback(int id)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    array keys = values(t_handles[id]);
    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);
    foreach(keys, string key)
      db->free(key);
    UNLOCK();
  }

  array t_list_keys(int id)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    LOCK();
    return Array.uniq(indices(handles) + indices(t_handles[id]));
    UNLOCK();
  }

  void t_destroy(int id)
  {
    if(!t_handles[id]) ERR("Unknown transaction id");

    t_rollback(id);
    LOCK();
    m_delete(t_start, id);
    m_delete(t_changes, id);
    m_delete(t_handles, id);
    m_delete(t_deleted, id);
    UNLOCK();
  }

  object transaction(object|void keep_ref)
  {
    if(!changes) ERR("Transactions are not enabled");

    LOCK();
    id++;
    t_start[id] = next_magic();
    t_changes[id] = ([]);
    t_handles[id] = ([]);
    t_deleted[id] = ([]);
    UNLOCK();
    return Transaction(this_object(), id, keep_ref);
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
  // Interface functions
  //

  void sync_schedule()
  {
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
  }

  void _destroy()
  {
    write = 0;
    destruct(this_object());
  }

  void close()
  {
    sync();
    _destroy();
  }

  void purge()
  {
    if(!write) ERR("Cannot purge in read mode");

    LOCK();
    write = 0;
    rm(filename+".inx");
    rm(filename+".chk");
    destruct(this_object());
    UNLOCK();
  }

  array _indices()
  {
    return list_keys();
  }
  
  void create(string filename_in, string mode)
  {
    filename = filename_in;

    if(search(mode, "w")+1)
      write = 1;
    if(search(mode, "t")+1) {
      changes = ([]);
      t_start = ([]);
      t_changes = ([]);
      t_handles = ([]);
      t_deleted = ([]);
    }

    index = Chunk(filename+".inx", mode, this_object());
    mapping m = ([]);
    if(sizeof(index->list_keys()))
      m = index->get(sort(index->list_keys())[-1]);
    handles = m->handles||([]);
    new = ([]);
    db = Chunk(filename+".chk", mode, this_object(), m->db||([]));

    if(write && search(mode, "s")+1) {
      sync_timeout = 32;
      sync_schedule();
    }
  }
}

class _Table {
  static string handle;
  static object table, parent;

  void sync() { table->sync(); }
  void delete(string handle) { table->delete(handle); }
  mixed set(string handle, mixed x) { return table->set(handle, x); }
  mixed get(string handle) { return table->get(handle); }
  array list_keys() { return table->list_keys(); }
  object transaction() { return table->transaction(this_object()); }
  void close() { destruct(this_object()); }
  void purge() { table->purge(); }

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
  
  void destroy()
  {
    if(parent)
      parent->_table_destroyed(handle);
  }
  
  void create(string _handle, object _table, object _parent)
  {
    handle = _handle;
    table = _table;
    parent = _parent;
  }
}

class db {
#ifdef THREAD_SAFE
  static inherit Thread.Mutex;
#endif

  static string dir, mode;
  static mapping tables = ([]), table_refs = ([]);
  static int write, id;

  void sync()
  {
    LOCK();
    foreach(values(tables), object o)
      if(o)
	o->sync();
    UNLOCK();
  }
  
  object table(string handle)
  {
    LOCK();
    if(!tables[handle])
      tables[handle] = Table(dir+"/"+handle, mode);
    table_refs[handle]++;
    // DEB(sprintf("### refs[%s]++ = %d\n", handle, table_refs[handle]));
    DEB(sprintf("# tables '%s': %d (%s)\n",
		reverse(reverse(dir/"/")[0..1])*"/", sizeof(tables), handle));
    return _Table(handle, tables[handle], this_object());
    UNLOCK();
  }

  void _table_destroyed(string handle)
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

  // Remove maximum one level of directories and files
  static private void level2_rm(string f)
  {
    if(sizeof(f) > 1 && f[-1] == '/')
      f = f[0..sizeof(f)-2];  // remove /'s
    if((file_stat(f)||({0,0}))[1] == -2)  // directory
      foreach(get_dir(f)||({}), string file)
	rm(f+"/"+file);  // delete file
    rm(f);  // delete file/directory
  }

  void purge()
  {
    LOCK();
    foreach(values(tables), object o)
      if(o)
	destruct(o);
    level2_rm(dir);
    destruct(this_object());
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

  void create(string dir_in, string mode_in)
  {
    dir = dir_in;
    mode = mode_in;
    
    if(search(mode, "w")+1) {
      write = 1;
      mkdirhier(dir+"/");
    }
  }
}

class LookupTable {
#ifdef THREAD_SAFE
  static inherit Thread.Mutex;
#endif
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
    table = Table(filename, mode);
  }
}

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
