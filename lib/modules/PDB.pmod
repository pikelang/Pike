/*
 * $Id: PDB.pmod,v 1.2 1998/03/25 15:25:23 noring Exp $
 */

#if constant(thread_create)
#define THREAD_SAFE
#define LOCK() do { object key; catch(key=lock())
#define UNLOCK() key=0; } while(0)
#else
#undef THREAD_SAFE
#define LOCK() do {
#define UNLOCK() } while(0)
#endif

#define PDB_ERR(msg) (exceptions?throw(({ "(PDB) "+msg+"\n",backtrace() })):0)
#define PDB_WARN(msg) werror("(PDB Warning) "+msg+"\n")

class FileIO {

#ifdef THREAD_SAFE
  static inherit Thread.Mutex;
#endif

  static int exceptions, sov;

  static private object open(string f, string m)
  {
    object o = Stdio.File();
    if(!o->open(f,m))
      return PDB_ERR("Failed to open file. "+f+": "+strerror(o->errno()));
    return o;
  }
  
  static int safe_write(object o, string d, string ctx)
  {
    int warned_already=0;
    int n;
    for(;;) {
      n = o->write(d);
      if(!sov || n == sizeof(d) || (n<0 && !(<11,12,16,24,28,49>)[o->errno()]))
	break;
      d = d[n..];
      if(!warned_already) {
	if(n<0)
	  PDB_WARN(ctx+": "+strerror(o->errno())+" (sleeping)");
	else
	  PDB_WARN(ctx+": Disk seems to be full. (sleeping)");
	warned_already=1;
      }
      sleep(1);
    }
    if(warned_already)
      PDB_WARN(ctx+": I'm OK now.");
    return (n<0? n : (n==strlen(d)? 1 : 0));
  }

  static int write_file(string f, mixed d)
  {
    d = encode_value(d);
    catch {
      string q;
      object g;
      if (sizeof(indices(g=Gz))) {
	if(strlen(q=g->deflate()->deflate(d)) < strlen(d))
	  d=q;
      }
    };
    object o = open(f+".tmp","wct");
    int n = safe_write(o, d, "write_file("+f+")");
    o->close();
    if(n == 1)
      return mv(f+".tmp", f)?1:PDB_ERR("Cannot move file.");
    rm(f+".tmp");
    return PDB_ERR("Failed to write file. Disk full?");
  }
  
  static mixed read_file(string f)
  {
    object o = open(f,"r");
    string d = o->read();
    catch {
      object g;
      if (sizeof(indices(g=Gz))) {
	d=g->inflate()->inflate(d);
      }
    };
    return decode_value(d);
  }
}


class Bucket
{
  inherit FileIO;
  static object file=Stdio.File();
  static array free_blocks = ({});
  static string rf;
  static int last_block, dirty;
  static function db_log;
  int size;

  static int log(int subtype, mixed arg)
  {
    return db_log('B', subtype, size, arg);
  }

  static int write_at(int offset, string to)
  {
    if(file->seek(offset*size) < 0)
      return PDB_ERR("write_at: seek failed");

    if(safe_write(file, to, "write_at") == 1)
      return 1;

    return PDB_ERR("Failed to write file. Disk full?");
  }
  
  static string read_at(int offset)
  {
    if(file->seek(offset*size) == -1)
      return PDB_ERR("Failed to seek.");
    return file->read(size);
  }
  
  mixed get_entry(int offset)
  {
    LOCK();
    return read_at(offset);
    UNLOCK();
  }
  
  static void save_free_blocks()
  {
    if(write_file(rf+".free", ({last_block, free_blocks})))
      dirty = 0;
  }
  
  void free_entry(int offset)
  {
    LOCK();
    free_blocks += ({ offset });
    dirty = 1;
    log('F', offset);
    if(size<4)   write_at(offset,"F");
    else         write_at(offset,"FREE");
    UNLOCK();
  }

  int allocate_entry()
  {
    int b;
    LOCK();
    if(sizeof(free_blocks)) {
      b = free_blocks[0];
      free_blocks -= ({ b });
    } else
      b = last_block++;
    dirty = 1;
    if(!log('A', b))
      return -1;
    UNLOCK();
    return b;
  }

  int sync()
  {
    LOCK();
    if(dirty)
      save_free_blocks();
    return !dirty;
    UNLOCK();
  }

  int set_entry(int offset, string to)
  {
    if(strlen(to) > size) return 0;
    LOCK();
    return write_at(offset, to);
    UNLOCK();
  }

  void restore_from_log(array log)
  {
    multiset alloced = (< >);
    multiset freed = (< >);
    foreach(log, array entry)
      switch(entry[0]) {
      case 'A':
	alloced[entry[1]] = 1;
	freed[entry[1]] = 0;
	break;
      case 'F':
	freed[entry[1]] = 1;
	alloced[entry[1]] = 0;
	break;
      }
    foreach(sort(indices(alloced)), int a)
      if(a>=last_block) {
	int i;
	for(i=last_block; i<a; i++)
	  free_blocks += ({ i });
	last_block = a+1;
	dirty = 1;
      } else if(search(free_blocks, a)>=0) {
	free_blocks -= ({ a });
	dirty = 1;
      }
    array(int) fr = indices(freed) - free_blocks;
    if(sizeof(fr)) {
      free_blocks += fr;
      foreach(fr, int f)
	if(size<4)   write_at(f,"F");
	else         write_at(f,"FREE");
      dirty = 1;
    }
  }
  
  void create(string d, int ms, int write, function logfun, int exceptions_in,
	      int sov_in)
  {
    string mode="r";
    size=ms;
    rf = d+ms;
    db_log = logfun;
    exceptions = exceptions_in;
    sov = sov_in;
    if(write) { mode="rwc"; }
    catch {
      array t = read_file(rf+".free");
      last_block = t[0];
      free_blocks = t[1];
    };
    if(!file->open(rf,mode)) destruct();
  }

  void destroy()
  {
    sync();
    if(file)
      destruct(file);
  }
};


class Table
{
  inherit FileIO;
  static mapping index = ([ ]);
  static int compress, write;
  static string dir, name;
  static function get_bucket;
  static int dirty;
  static function db_log;

  static int log(int subtype, mixed ... arg)
  {
    return db_log('T', subtype, name, @arg);
  }

  int sync()
  {
    LOCK();
    if(dirty) {
      if(write_file(dir+".INDEX", index))
	dirty = 0;
    }
    return !dirty;
    UNLOCK();
  }

  int find_nearest_2x(int num)
  {
    for(int b=4;b<32;b++) if((1<<b) >= num) return (1<<b);
  }

  function scheme = find_nearest_2x;


  void delete(string in)
  {
    if(!write) return;
    LOCK();
    array i;
    if(i=index[in]) {
      m_delete(index,in);
      dirty = 1;
      log('D', in);
      object bucket = get_bucket(i[0]);
      bucket->free_entry(i[1]);
    }
    UNLOCK();
  }

  mixed set(string in, mixed to)
  {
    if(!write) return 0;
    string ts = encode_value(to);
    if(compress)
      catch {
	string q;
	object g;
	if (sizeof(indices(g=Gz))) {
	  if(strlen(q=g->deflate()->deflate(ts)) < strlen(ts))
	    ts=q;
	}
      };
    LOCK();
    object bucket = get_bucket(scheme(strlen(ts)));
    int of = bucket->allocate_entry();
    if(of>=0 && bucket->set_entry(of, ts)) {
      array new_entry = ({ bucket->size, of });
      if(log('C', in, new_entry)) {
	delete(in);
	index[in]=new_entry;
	dirty = 1;
      } else
	to = 0;
    } else
      to = 0;
    UNLOCK();
    return to;
  }

  mixed `[]=(string in,mixed to) {
    return set(in,to);
  }
  
  mixed get(string in)
  {
    array i;
    mixed d;
    LOCK();
    if(!(i=index[in])) return 0;
    object bucket = get_bucket(i[0]);
    d = bucket->get_entry(i[1]);
    UNLOCK();
    catch {
      object g;
      if (sizeof(indices(g=Gz))) {
	d=g->inflate()->inflate(d);
      }
    };
    return decode_value( d );
  }

  mixed `[](string in) {
    return get(in);
  }

  array list_keys() {
//  LOCK();
    return indices(index);
//  UNLOCK();
  }
  
  array _indices() {
    return list_keys();
  }

  array match(string match)
  {
    return glob(match, _indices());
  }

  void purge()
  {
    // remove table...
    if(!write) return;
    LOCK();
    foreach(_indices(), string d)
      delete(d);
    rm(dir+".INDEX");
    index=([]);
    dirty = 0;
    log('P');
    UNLOCK();
  }

  void rehash()
  {
    // TBD ...
  }
  
  void restore_from_log(array log)
  {
    int purge = 0;
    foreach(log, array entry)
      switch(entry[0]) {
      case 'D':
	m_delete(index, entry[1]);
	break;
      case 'C':
	index[entry[1]] = entry[2];
	break;
      case 'P':
	index = ([]);
	purge = 1;
	break;
      }
    if(!write) return;
    if(purge) {
      rm(dir+".INDEX");
      dirty = sizeof(index)>0;
    } else
      dirty = sizeof(log)>0;
  }

  void create(string n, string d, int wp, int cp, function fn, function logfun, int exceptions_in, int sov_in)
  {
    name = n;
    get_bucket = fn;
    db_log = logfun;
    dir = d;
    exceptions = exceptions_in;
    sov = sov_in;
    if(sizeof(predef::indices(Gz)) && cp) compress=1;
    if(wp) write=1;
    catch { index = read_file(dir+".INDEX"); };
  }

  void destroy()
  {
    sync();
  }
};


class db
{
  inherit FileIO;
#ifdef THREAD_SAFE
  static inherit Thread.Mutex;
#endif

  static int write, compress;
  static string dir;
  static mapping (int:object(Bucket)) buckets = ([]);
  static mapping (string:object(Table)) tables = ([]);
  static object(Stdio.File) logfile;

  static int log(int major, int minor, mixed ... args)
  {
    LOCK();
    if(logfile) {
      int rc;
      string entry = sprintf("%c%c%s\n", major, minor,
			     replace(encode_value(args),
				     ({ "\203", "\n" }),
				     ({ "\203\203", "\203n" })));

      if((rc = safe_write(logfile, entry, "log"))!=1)
	return PDB_ERR("Failed to write log: "+(rc<0? strerror(logfile->errno()):
						"Disk full (probably)"));
    }
    UNLOCK();
    return 1;
  }

  static object(Bucket) get_bucket(int s)
  {
    object bucket;
    LOCK();
    if(!(bucket = buckets[s]))
      buckets[s] = bucket = Bucket( dir+"Buckets/", s, write, log, exceptions, sov );
    UNLOCK();
    return bucket;
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

  object(Table) table(string tname)
  {
    tname -= "/";
    LOCK();
    if(tables[tname])
      return tables[tname];
    return tables[tname] =
      Table(tname, combine_path(dir, tname), write, compress, get_bucket, log, exceptions, sov );
    UNLOCK();
  }

  object(Table) `[](string t)
  {
    return table(t);
  }
  
  array(string) list_tables()
  {
    LOCK();
    return Array.map(glob("*.INDEX", get_dir(dir) || ({})),
		     lambda(string s)
		     { return s[..(sizeof(s)-1-sizeof(".INDEX"))]; });
    UNLOCK();
  }

  array(string) _indices()
  {
    return list_tables();
  }

  static void rotate_logs()
  {
    mv(dir+"log.1", dir+"log.2");
    logfile->open(dir+"log.1", "cwta");
  }

  int sync()
  {
    array b, t;
    int rc, ok=1;
    LOCK();
    rc = log('D', '1');
    b = values(buckets);
    t = values(tables);
    UNLOCK();
    if(rc) {
      foreach(b+t, object o)
	if(catch{if(!o->sync()) ok=0;}) ok=0;
      if(ok) {
	LOCK();
	if(log('D', '2'))
	  rotate_logs();
	else ok=0;
	UNLOCK();
      }
    } else ok=0;
    remove_call_out(sync);
    call_out(sync, 200);
    return ok;
  }

  // Remove maximum one level of directories and files
  static void level2_rm(string f)
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
    // remove whole db...
    LOCK();
    foreach(values(buckets)+values(tables)+({ logfile }), object o)
      destruct(o);
    buckets = 0;
    tables = 0;
    level2_rm(dir+"Buckets");
    level2_rm(dir);
    dir = 0;
    destruct();
    UNLOCK();
  }

  static void restore_logs()
  {
    string log = "\n"+(Stdio.read_file(dir+"log.2")||"")+
      (Stdio.read_file(dir+"log.1")||"")+"\n";
    int p=-1, d1_pos=-1, d2_pos=-1;
    while((p=search(log, "\nD", p+1))>=0)
      if(log[p+2]=='1')
	d1_pos = p;
      else if(log[p+2]=='2')
	d2_pos = d1_pos;
    if(d2_pos >= 0)
      log = log[d2_pos..];

    mapping(int:array(array)) bucket_log = ([]);
    mapping(string:array(array)) table_log = ([]);

    foreach(log/"\n", string entry) {
      int main, sub;
      array a;
      if(sscanf(entry, "%c%c%s", main, sub, entry)==3 &&
	 !catch(a = decode_value(replace(entry, ({ "\203\203", "\203n" }),
					 ({ "\203", "\n" })))))
	switch(main) {
	case 'D':
	  break;
	case 'T':
	  if(table_log[a[0]])
	    table_log[a[0]] += ({ ({ sub }) + a[1..] });
	  else
	    table_log[a[0]] = ({ ({ sub }) + a[1..] });
	  break;
	case 'B':
	  if(bucket_log[a[0]])
	    bucket_log[a[0]] += ({ ({ sub }) + a[1..] });
	  else
	    bucket_log[a[0]] = ({ ({ sub }) + a[1..] });
	  break;
	  break;
	}
    }
    log = 0;

    foreach(indices(bucket_log), int bucket)
      get_bucket(bucket)->restore_from_log(bucket_log[bucket]);
    foreach(indices(table_log), string t)
      table(t)->restore_from_log(table_log[t]);
  }
  
  void create(string d, string mode)
  {
    if(search(mode,"e")+1) exceptions=1;
    if(search(mode,"w")+1) write=1;
    if(search(mode,"C")+1) compress=1;
    if(search(mode,"s")+1) sov=1;
    if(search(mode,"c")+1) if(!file_stat(d))
    {
      mkdirhier(d+"/foo");
      mkdirhier(d+"/Buckets/foo");
    }
    dir = replace(d+"/","//","/");
    logfile = 0;
    restore_logs();
    if (write) {
      logfile = Stdio.File();
      logfile->open(dir+"log.1", "cwa");
      if(logfile->write("\n")!=1) {
	destruct(this_object());
	PDB_ERR("Failed to write a single byte to log.");
      }
      sync();
    }
  }
};
