/*
 * A generic cache front-end
 * by Francesco Chemolli <kinkie@roxen.com>
 *
 * $Id: cache.pike,v 1.11 2006/01/23 13:29:04 mast Exp $
 *
 */

//! This module serves as a front-end to different kinds of caching systems.
//! It uses two helper objects to actually store data, and to determine
//! expiration policies. Mechanisms to allow for distributed caching systems
//! will be added in time, or at least that is the plan.

#pike __REAL_VERSION__

#if constant(thread_create)
#define do_possibly_threaded_call thread_create
#else
#define do_possibly_threaded_call call_function
#endif

#define DEFAULT_CLEANUP_CYCLE 300


private int cleanup_cycle=DEFAULT_CLEANUP_CYCLE;
static object(Cache.Storage.Base) storage;
static object(Cache.Policy.Base) policy;

// TODO: check that Storage Managers return the appropriate zero_type
//! Looks in the cache for an element with the given key and, if available,
//! returns it. Returns 0 if the element is not available
mixed lookup(string key) {
  if (!stringp(key)) key=(string)key; // paranoia.
  object(Cache.Data) tmp=storage->get(key);
  return (tmp?tmp->data():UNDEFINED);
}

//structure: "key" -> (< ({function,args,0|timeout_call_out_id}) ...>)
// this is used in case multiple async-lookups are attempted
// against a single key, so that they can be collapsed into a
// single backend-request.
private mapping (string:multiset(array)) pending_requests=([]);

private void got_results(string key, int|Cache.Data value) {
  mixed data=UNDEFINED;
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

//! Asynchronously look the cache up.
//! The callback will be given as arguments the key, the value, and then
//! any user-supplied arguments.
//! If the timeout (in seconds) expires before any data could be retrieved,
//! the callback is called anyways, with 0 as value.
void alookup(string key,
              function(string,mixed,mixed...:void) callback,
              int|float timeout,
              mixed ... args) {
  if (!stringp(key)) key=(string)key; // paranoia
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

//! Sets some value in the cache. Notice that the actual set operation
//! might even not happen at all if the set data doesn't make sense. For
//! instance, storing an object or a program in an SQL-based backend
//! will not be done, and no error will be given about the operation not being
//! performed.
//!
//! Notice that while max_life will most likely be respected (objects will
//! be garbage-collected at pre-determined intervals anyways), the
//! preciousness . is to be seen as advisory only for the garbage collector
//! If some data was stored with the same key, it gets returned.
//! Also notice that max_life is @b{relative@} and in seconds.
//! dependants are not fully implemented yet. They are implemented after
//! a request by Martin Stjerrholm, and their purpose is to have some
//! weak form of referential integrity. Simply speaking, they are a list
//! of keys which (if present) will be deleted when the stored entry is
//! deleted (either forcibly or not). They must be handled by the storage 
//! manager.
void store(string key, mixed value, void|int max_life,
            void|float preciousness, void|multiset(string) dependants ) {
  if (!stringp(key)) key=(string)key; // paranoia
  multiset(string) rd=UNDEFINED;  // real-dependants, after string-check
  if (dependants) {
    rd=(<>);
    foreach((array)dependants,mixed d) {
      rd[(string)d]=1;
    }
  }
  storage->set(key,value,
               (max_life?time(1)+max_life:0),
               preciousness,rd);
}

//! Forcibly removes some key.
//! If the 'hard' parameter is supplied and true, deleted objects will also
//! have their @[lfun::destroy] method called upon removal by some
//! backends (i.e. memory)
void delete(string key, void|int(0..1)hard) {
  if (!stringp(key)) key=(string)key; // paranoia
  storage->delete(key,hard);
}


//notice. policy->expire is not guarranteed to be reentrant.
// Generally speaking, it will not be, but it would be very stupid to
// require so. The whole garbage collection architecture is heavily
// oriented to either single-pass or mark-and-sweep algos, it would be
// useless to require it be reentrant.

int cleanup_lock=0;

private void do_cleanup(function expiry_function, object storage) {
  // I know, generally speaking this would be racey. But this function will
  // be called at most every cleanup_cycle seconds, so no locking done here.
  if (cleanup_lock)
    return;
  cleanup_lock=1;
  expiry_function(storage);
  cleanup_lock=0;
}

static Thread.Thread cleanup_thread;

static void destroy()
{
  if (Thread.Thread t = cleanup_thread) {
    cleanup_thread = 0;
    t->wait();
  }
}

//!
void start_cleanup_cycle() {
  if (master()->asyncp()) { //we're asynchronous. Let's use call_outs
    call_out(async_cleanup_cache,cleanup_cycle);
    return;
  }
#if constant(thread_create)
  cleanup_thread = thread_create(threaded_cleanup_cycle);
#else
  call_out(async_cleanup_cache,cleanup_cycle); //let's hope we'll get async
                                               //sooner or later.
#endif
}

//!
void async_cleanup_cache() {
  mixed err=catch {
    do_possibly_threaded_call(do_cleanup,policy->expire,storage);
  };
  call_out(async_cleanup_cache,cleanup_cycle);
  if (err)
    throw (err);
}

//!
void threaded_cleanup_cycle() {
  while (1) {
    if (master()->asyncp()) {   // might as well use call_out if we're async
      call_out(async_cleanup_cache,0);
      return;
    }
    for (int wait = 0; wait < cleanup_cycle; wait++) {
      sleep (1);
      if (!cleanup_thread) return;
    }
    do_cleanup(policy->expire,storage);
  }
}

//! Creates a new cache object. Required are a storage manager, and an
//! expiration policy object.
void create(Cache.Storage.Base storage_mgr,
            Cache.Policy.Base policy_mgr,
            void|int cleanup_cycle_delay) {
  if (!storage_mgr || !policy_mgr)
    error ( "I need a storage manager and a policy manager\n" );
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
