/*
 * A generic cache front-end
 * by Francesco Chemolli <kinkie@roxen.com>
 * (C) 2000 Roxen IS
 *
 * $Id: cache.pike,v 1.1 2000/07/02 20:14:27 kinkie Exp $
 *
 * This module serves as a front-end to different kinds of caching system
 * It uses two helper objects to actually store data, and to determine
 * expiration policies. Mechanisms to allow for distributed caching systems
 * will be added in time, or at least this is the plan.
 */

#if constant(thread_create)
#define do_possibly_threaded_call thread_create
#else
#define do_possibly_threaded_call call_function
#endif

#define DEFAULT_CLEANUP_CYCLE 300


private int cleanup_cycle=DEFAULT_CLEANUP_CYCLE;
private object(Cache.Storage.Base) storage;
private object(Cache.Policy.Base) policy;


//. Looks in the cache for an element with the given key and, if available,
//. returns it. Returns 0 if the element is not available
mixed lookup(string key) {
  object(Cache.Data) tmp=storage->get(key);
  return (tmp?tmp->data():0);
}

//structure: "key" -> (< ({function,args,0|timeout_call_out_id}) ...>)
private mapping (string:multiset(array)) pending_requests=([]);

private void got_results(string key, int|Cache.Data value) {
  mixed data=([])[0]; //undef
  if (pending_requests[key]) {
    if (value) {
      data=value->data();
    }
    foreach(indices(pending_requests[key]),array cb) {
      cb[0](key,value,@cb[1]);
      if (cb[2]) remove_call_out(cb[2]);
    }
    m_delete(pending_requests,key);
  }
  //pending requests have timed out. Let's just ignore this result.
}

//hooray for aliasing. This implementation relies _heavily_ on it
//for the "req" argument
private void no_results(string key, array req, mixed call_out_id) {
  pending_requests[key][req]=0; //remove the pending request
  req[0](key,0,@req[1]);        //invoke the callback with no data
}

//. asynchronously look the cache up.
//. The callback will be given as arguments the key, the value, and then
//. any user-supplied arguments.
//. If the timeout (in seconds) expires before any data could be retrieved,
//. the callback is called anyways, with 0 as value.
void alookup(string key,
              function(string,mixed,mixed...:void) callback,
              int|float timeout,
              mixed ... args) {
  array req = ({callback,args,0});
  if (!pending_requests[key]) { //FIXME: add double-indirection
    pending_requests[key]=(< req >);
    storage->aget(key,got_results);
  } else {
    pending_requests[key][req]=1;
    //no need to ask the storage manager, since a query is already pending
  }
  if (timeout)
    req[2]=call_out(no_results,timeout,key,req); //aliasing, gotta love it
}

//. Sets some value in the cache. Notice that the actual set operation
//. might even not happen at all if the set data doesn't make sense. For
//. instance, storing an object or a program in an SQL-based backend
//. will not be done, and no error will be given about the operation not being
//. performed.
//. Notice that while max_life will most likely be respected (objects will
//. be garbage-collected at pre-determined intervals anyways), the
//. preciousness . is to be seen as advisory only for the garbage collector
//. If some data was stored with the same key, it gets returned.
//. Also notice that max_life is _RELATIVE_ and in seconds.
void store(string key, mixed value, void|int max_life,
            void|float preciousness) {
  storage->set(key,value,
               (max_life?time(1)+max_life:0),
               preciousness);
}

//. Forcibly removes some key. If data was actually present under that key,
//. it is returned. Otherwise 0 is returned.
//. If the 'hard' parameter is supplied and true, deleted objects will also
//. be destruct()-ed upon removal.
mixed delete(string key, void|int(0..1)hard) {
  object(Cache.Data) tmp=storage->delete(key,hard);
  return (tmp?tmp->data():0);
}


object cleanup_thread=0;

void start_cleanup_cycle() {
  if (master()->asyncp()) { //we're asynchronous. Let's use call_outs
    call_out(async_cleanup_cache,cleanup_cycle);
    return;
  }
#if constant(thread_create)
  cleanup_thread=thread_create(threaded_cleanup_cycle);
#else
  call_out(async_cleanup_cache,cleanup_cycle); //let's hope we'll get async
                                               //sooner or later.
#endif
}

void async_cleanup_cache() {
  call_out(async_cleanup_cache,cleanup_cycle);
  do_possibly_threaded_call(policy->expire,storage);
}

void threaded_cleanup_cycle() {
  while (1) {
    if (master()->asyncp()) {
      call_out(async_cleanup_cache,0);
      return;
    }
    sleep(cleanup_cycle);
    policy->expire(storage);
  }
}

//. Creates a new cache object. Required are a storage manager, and an
//. expiration policy object.
void create(Cache.Storage.Base storage_mgr,
            Cache.Policy.Base policy_mgr,
            void|int cleanup_cycle_delay) {
  if (!storage_mgr || !policy_mgr)
    throw ( ({ "I need a storage manager and a policy manager",
               backtrace() }) );
  storage=storage_mgr;
  policy=policy_mgr;
  if (cleanup_cycle_delay) cleanup_cycle=cleanup_cycle_delay;
  start_cleanup_cycle();
}


/*
 * Miscellaneous thoughts.
 *
 * Some kind of settings-system will be needed, at least for the policy
 * manager. Maybe having a couple of pass-throught functions here might help.
 *
 * Data-objects should really be created by the Storage Manager, which can
 * then choose to use specialized forms (i.e. using some SQL tricks to
 * perform lazy work).
 *
 * I chose to go with call_outs for the cleanup cycle, and start a new thread
 * if possible when doing
 * cleanup. I have mixed feelings for this choice. On one side, it is quite
 * cheap and easily implemented. On the other side, it restricts us to
 * async mode, and creating a new thread can be not-so-cheap.
 *
 * It would be nice to have some statistics collection. But for some kind of
 * stats the storage manager has to be involved (if any kind of efficiency
 * is desired). However if we wish to do so, either we extend the storage
 * manager's API, or we're in trouble.
 */
