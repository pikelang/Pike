/*
 * A GDBM-based storage manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Gdbm.pike,v 1.1 2000/07/02 20:15:42 kinkie Exp $
 *
 * This storage manager provides the means to save data to memory.
 * In this manager I'll add reference documentation as comments to
 * interfaces. It will be organized later in a more comprehensive format
 *
 * Settings will be added later.
 */

//after this many deletion ops, the databases will be compacted.
#define CLUTTERED 100


Gdbm.gdbm db, metadb;
int deletion_ops=0; //every 1000 deletion ops, we'll reorganize.

class Data {
  inherit Cache.Data;
  //metadata is kept around, data loaded on demand.

  int _size=0;
  string _key=0;
  mixed _data=0;
  private Gdbm.gdbm db, metadb;
  
  int size() {
    if (_size) return _size;
    _size=recursive_low_size(data());
    return _size;
  }
  
  mixed data() {
    if (!_data) 
      _data=decode_value(db->fetch(_key));
    return _data;
  }
  
  private inline string metadata_dump () {
    return encode_value( (["size":_size,"atime":atime,
                           "ctime":ctime,"etime":etime,"cost":cost]) );
  }
  
  //dumps the metadata if necessary.
  void sync() {
    metadb->store(_key,metadata_dump());
  }
  //restores a dumped object
  //basically a kind of second-stage constructor for objects retrieved
  //from the db.
  //the dumped value is passed for efficiency reasons, otherwise we
  //might have to perform two lookups to successfully retrieve an object
  Data undump( string dumped_value) {
    mapping m=decode_value(dumped_value);
    if (!m) throw ( ({"Can't decode dumped value",backtrace()}) );
    _size=m->size;
    atime=m->atime;
    ctime=m->ctime;
    etime=m->etime;
    cost=m->cost;
    return this_object();
  }
  
  inline void touch() {
    atime=time(1);
    sync();
  }
  
  //initializes a new object with a fresh value. It's used only
  //for the first instantiation, after that undump is to be used.
  //The data is not immediately dumped to the DB, as the destructor
  //will take care of that.
  Data init(mixed value, void|int expires, void|float 
            preciousness) {
    atime=ctime=time(1);
    if (expires) etime=expires;
    if (preciousness) cost=preciousness;
    sync();
    db->store(_key,encode_value(value));
    return this_object();
  }
  
  void create(string key, Gdbm.gdbm data_db, Gdbm.gdbm metadata_db) {
    _key=key;
    db=data_db;
    metadb=metadata_db;
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

void set(string key, mixed value,
         void|int expire_time, void|float preciousness) {
  //should I refuse storing objects too?
  if (programp(value)||functionp(value)) {
    werror("can't store value\n"); //TODO: use crumbs
    return 0;
  }
  Data(key,db,metadb)->init(value,expire_time,preciousness);
}

int(0..0)|Cache.Data get(string key,void|int notouch) {
  mixed tmp=metadb->fetch(key);
  if (tmp) {
    tmp=(Data(key,db,metadb)->undump(tmp));
    if (!notouch) {
      tmp->touch();
    }
  }
  return tmp;
}

//fetches some data from the cache asynchronously.
//the callback will get as first argument the key, and as second
//argument 0 (cache miss) or a Cache.Data object.
void aget(string key,
          function(string,int(0..0)|Cache.Data:void) callback) {
  callback(key,get(key));
}

Cache.Data|int(0..0) delete(string key, void|int(0..1) hard) {
  Data rv=(hard?0:get(key,1));
  db->delete(key);
  metadb->delete(key);
  deletion_ops++;
  if (deletion_ops > CLUTTERED) {
    werror("Reorganizing database\n");
    db->reorganize();
    metadb->reorganize();
    deletion_ops=0;
  }
  return rv;
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
