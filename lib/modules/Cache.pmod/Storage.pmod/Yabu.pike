/*
 * A Yabu-based storage manager.
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: Yabu.pike,v 1.2 2000/07/05 21:38:56 kinkie Exp $
 *
 * Settings will be added later.
 */

#define CLUTTERED 200

Yabu.Table db, metadb;
Yabu.db yabudb;

int deletion_ops=0;

class Data {
  inherit Cache.Data;
  //metadata is kept around, data loaded on demand.

  int _size=0;
  string _key=0;
  mixed _data=0;
  private Yabu.table db, metadb;
  
  int size() {
    if (_size) return _size;
    _size=sizeof(encode_value(data()));
    return _size;
  }
  
  mixed data() {
    if (!_data) 
      _data=db->get(_key);
    return _data;
  }
  
  private inline mapping metadata_dump () {
    return (["size":_size,"atime":atime,
             "ctime":ctime,"etime":etime,"cost":cost]);
  }
  
  //dumps the metadata if necessary.
  void sync() {
    metadb->set(_key,metadata_dump());
  }

  //FIXME
  //restores a dumped object
  //basically a kind of second-stage constructor for objects retrieved
  //from the db.
  //the dumped value is passed for efficiency reasons, otherwise we
  //might have to perform two lookups to successfully retrieve an object
  Data undump(mapping dumped_value) {
     mapping m=dumped_value;
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
  
  //FIXME
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
  keys=ADT.Queue(@indices(metadb));
  return keys->get();
}

int(0..0)|string next() {
  if (!keys) return 0;
  return keys->get();
}

void set(string key, mixed value,
          void|int expire_time, void|float preciousness) {
  //problem: we can't store objects, functions or programs.
  //we check here for the actual type. BUT if some 'forbidden' type
  //is in a composite type's element, we'll mangle it along the way.
  //Checking for bad types in composite arguments would be very expensive
  //so I'd favour just stating the problem in the documentation and let
  //developers take care of this themselves.
   if (programp(value)||functionp(value)||objectp(value)) {
     werror("can't store value\n"); //TODO: use crumbs
     return 0;
   }
   int tm=time(1);
   mapping meta;
   db->set(key,value);
   //maybe we could lazy-ify this
   meta=(["size":sizeof(encode_value(value)), "atime":tm,"ctime":tm]);
   if (expire_time) meta->etime=expire_time;
   if (preciousness||!zero_type(preciousness))
     meta->cost=preciousness;
   else
     meta->cost=1.0;
   metadb->set(key,meta);
}

//FIXME
int(0..0)|Cache.Data get(string key,void|int notouch) {
  mixed tmp=metadb->get(key);
  if (tmp) {
    tmp=(Data(key,db,metadb))->undump(tmp);
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

void delete(string key, void|int(0..1) hard) {
  db->delete(key);
  metadb->delete(key);
  deletion_ops++;
  if (deletion_ops > CLUTTERED) {
    yabudb->reorganize();
    deletion_ops=0;
  }
}

void create(string path) {
  yabudb=Yabu.db(path+".yabu","wcSQ"); //let's hope I got the mode right.
  db=yabudb["data"];
  metadb=yabudb["metadata"];
}


/**************** thoughts and miscellanea ******************/
//maybe we should split the database into two databases, one for the data
//and one for the metadata.

//we should really use an in-memory cache for the objects. I delay that
//for now, since we don't have a decent footprint-constrained policy
//manager yet.
