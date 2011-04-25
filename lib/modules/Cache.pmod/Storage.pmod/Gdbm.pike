/*
 * A GBM-based storage manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id$
 *
 * This storage manager provides the means to save data to memory.
 * In this manager I'll add reference documentation as comments to
 * interfaces. It will be organized later in a more comprehensive format
 *
 * Settings will be added later.
 * TODO: verify dependants' implementation.
 */

#pike __REAL_VERSION__

//after this many deletion ops, the databases will be compacted.
#define CLUTTERED 1000

#if constant(Gdbm.gdbm)
Gdbm.gdbm db, metadb;
int deletion_ops=0; //every 1000 deletion ops, we'll reorganize.
int have_dependants=0;

class Data {
  inherit Cache.Data;

  int _size=0;
  string _key=0;
  mixed _data=0;
  
  int size() {
    return _size; //it's guarranteed to be set. See set()
  }
  
  mixed data() {
    if (!_data)
      _data=decode_value(db[_key]);
    return _data;
  }
  
  private inline string metadata_dump () {
    return encode_value( (["size":_size,"atime":atime,
                           "ctime":ctime,"etime":etime,"cost":cost]) );
  }
  
  //dumps the metadata if necessary.
  void sync() {
    metadb[_key]=metadata_dump();
  }

  inline void touch() {
    atime=time(1);
    sync();
  }
  
  void create(string key, Gdbm.gdbm data_db, 
              Gdbm.gdbm metadata_db, string dumped_metadata) {
    mapping m=decode_value(dumped_metadata);
    _key=key;
    db=data_db;
    metadb=metadata_db;
    _size=m->size;
    atime=m->atime;
    ctime=m->ctime;
    etime=m->etime;
    cost=(float)(m->cost||1);
  }
  
}


//we could maybe use some kind of progressive approach: keep a few
//items in queue, then fetch more as we need them. This approach
//can be expensive in terms of memory
//Something I can't figure out a clean solution for: reorganizing
//the database. It would be cool to do that when we know it to be
//somewhat junky, but guessing that kind of information would be
//quite hard, especially if we consider caches surviving the process
//that created them
//Maybe we can put some heuristics: since almost only the policy manager
//uses first(), next() and delete(), we might count the deletion operations
//and reorganize when we reach some kind of threshold.
private string iter=0;
int(0..0)|string first() {
  return (iter=metadb->firstkey());
}

int(0..0)|string next() {
  return (iter=metadb->nextkey(iter));
}

// we set the data in the database directly here, since we have
// no need to create a Data object.
void set(string key, mixed value,
         void|int expire_time, void|float preciousness,
         void|multiset(string) dependants) {
  //should I refuse storing objects too?
  if (programp(value)||functionp(value)) {
    werror("can't store value\n"); //TODO: use crumbs
    return 0;
  }
  string tmp;
  int tm=time(1);
  mapping meta;
  tmp=encode_value(value);
  db[key]=tmp;
  meta=(["size":sizeof(tmp),"atime":tm,"ctime":tm]);
  if (expire_time) meta->etime=expire_time;
  if (preciousness||!zero_type(preciousness))
    meta->cost=preciousness;
  else
    meta->cost=1.0;
  if (dependants) {
    meta->dependants=dependants;
    have_dependants=1;
  }
  
  metadb[key]=encode_value(meta);
}

// we fetch at least the metadata here. If we delegated that to the
// Data class, we would waste quite a lot of resources while doing
// the undump operation.
int(0..0)|Cache.Data get(string key,void|int notouch) {
  string metadata=metadb[key];
  Data rv;
  if (!metadata) return 0;      // no such key in cache.
  rv = Data(key,db,metadb,metadata);
  if (!notouch) {
    rv->touch();
  }
  return rv;
}

//fetches some data from the cache asynchronously.
//the callback will get as first argument the key, and as second
//argument 0 (cache miss) or a Cache.Data object.
void aget(string key,
          function(string,int(0..0)|Cache.Data:void) callback) {
  callback(key,get(key));
}

void delete(string key, void|int(0..1) hard) {
  multiset(string) dependants=0;

  if (have_dependants) {
    string emeta=metadb->fetch(key);
    if (!emeta) return;         // no such key. Already deleted.
    dependants=decode_value(emeta)->dependants;
  }

  //werror("Deleteing %s\n",key);
  db->delete(key);
  metadb->delete(key);
  deletion_ops++;

  if (dependants) {
    foreach((array)dependants, string chain) {
      //werror("chain-deleteing %s\n",chain);
      delete(chain);            // recursively delete
    }
    //werror("Done chain-deleteing\n");
    return;                   // return so that reorg takes place at the end
  }
  
  if (deletion_ops > CLUTTERED) {
    //werror("Reorganizing database\n");
    db->reorganize();
    metadb->reorganize();
    deletion_ops=0;
  }
}

//A GDBM storage-manager must be hooked to a GDBM Database.
void create(string path) {
  db=Gdbm.gdbm(path+".db","rwcf");
  metadb=Gdbm.gdbm(path+"_meta.db","rwcf");
}

#endif // constant(Gdbm.gdbm)

/**************** thoughts and miscellanea ******************/
//maybe we should split the database into two databases, one for the data
//and one for the metadata.

//we should really use an in-memory cache for the objects. I delay that
//for now, since we don't have a decent footprint-constrained policy
//manager yet.
