/*
 * A GDBM-based storage manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Gdbm.pike,v 1.3 2000/09/26 18:59:12 hubbe Exp $
 *
 * This storage manager provides the means to save data to memory.
 * In this manager I'll add reference documentation as comments to
 * interfaces. It will be organized later in a more comprehensive format
 *
 * Settings will be added later.
 */

#pike __VERSION__

//after this many deletion ops, the databases will be compacted.
#define CLUTTERED 100


Gdbm.gdbm db, metadb;
int deletion_ops=0; //every 1000 deletion ops, we'll reorganize.

class Data {
  inherit Cache.Data;

  int _size=0;
  string _key=0;
  mixed _data=0;
  
  int size() {
    if (_size) return _size;
    _size=recursive_low_size(data());
    return _size;
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
    cost=m->cost;    
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
private ADT.Queue keys;
int(0..0)|string first() {
  string tmp;
  keys=ADT.Queue();
  tmp=metadb->firstkey();
  while (tmp) {
    keys->put(tmp);
    tmp=metadb->nextkey(tmp);
  }
  return keys->get();
}

int(0..0)|string next() {
  if (!keys) return 0;
  return keys->get();
}

// we set the data in the database directly here, since we have
// no need to create a Data object.
void set(string key, mixed value,
         void|int expire_time, void|float preciousness) {
  string tmp;
  int tm=time(1);
  mapping meta;
  //should I refuse storing objects too?
  if (programp(value)||functionp(value)) {
    werror("can't store value\n"); //TODO: use crumbs
    return 0;
  }
  tmp=encode_value(value);
  db[key]=tmp;
  meta=(["size":sizeof(tmp),"atime":tm,"ctime":tm]);
  if (expire_time) meta->etime=expire_time;
  if (preciousness||!zero_type(preciousness))
    meta->cost=preciousness;
  else
    meta->cost=1.0;
  
  metadb[key]=encode_value(meta);
}

// we fetch at least the metadata here. If we delegated that to the
// Data class, we would waste quite a lot of resources while doing
// the undump operation.
int(0..0)|Cache.Data get(string key,void|int notouch) {
  string metadata=metadb[key];
  mixed err;
  Data rv;
  if (!metadata) return 0;      // no such key in cache.
  err = catch (rv=(Data(key,db,metadb,metadata)));
  if (err) return 0;            // could not undump the metadata
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
  db->delete(key);
  metadb->delete(key);
  deletion_ops++;
  if (deletion_ops > CLUTTERED) {
    werror("Reorganizing database\n");
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


/**************** thoughts and miscellanea ******************/
//maybe we should split the database into two databases, one for the data
//and one for the metadata.

//we should really use an in-memory cache for the objects. I delay that
//for now, since we don't have a decent footprint-constrained policy
//manager yet.
