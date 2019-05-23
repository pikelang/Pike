#pike __REAL_VERSION__

//! Module for handling multiple concurrent events.
//!
//! The @[Future] and @[Promise] API was inspired by
//! @url{https://github.com/couchdeveloper/FutureLib@}.

local protected enum State {
  STATE_NO_FUTURE = -1,
  STATE_PENDING = 0,
  STATE_FULFILLED,
  STATE_REJECTED,
  STATE_REJECTION_REPORTED,
};

protected Thread.Mutex mux = Thread.Mutex();
protected Thread.Condition cond = Thread.Condition();

//! Global failure callback, called when a promise without failure
//! callback fails. This is useful to log exceptions, so they are not
//! just silently caught and ignored.
void on_failure(function(mixed : void) f)
{
  global_on_failure = f;
}
protected function(mixed : void) global_on_failure = master()->handle_error;

//! @param enable
//!   @int
//!     @value 0
//!       A @expr{false@} value causes all @[Concurrent] callbacks
//!       (except for timeouts) to default to being called directly,
//!       without using a backend.
//!     @value 1
//!       A @expr{true@} value causes callbacks to default to being
//!       called via @[Pike.DefaultBackend].
//!   @endint
//!
//! @note
//!   Be very careful about running in the backend disabled mode,
//!   as it may cause unlimited recursion and reentrancy issues.
//!
//! @note
//!   As long as the backend hasn't started, it will default to @expr{false@}.
//!   Upon startup of the backend, it will change to @expr{true@} unless you
//!   explicitly called @[use_backend()] before that.
//!
//! @note
//!   (Un)setting this typically alters the order in which some callbacks
//!   are called (depending on what happens in a callback).
//!
//! @seealso
//!   @[Future()->set_backend()], @[Future()->call_callback()]
final void use_backend(int enable)
{
  callout = enable ? call_out : callnow;
  remove_call_out(auto_use_backend);
}

private mixed
 callnow(function(mixed ...:void) f, int|float delay, mixed ... args)
{
  mixed err = catch (f(@args));
  if (err)
    master()->handle_error(err);
  return 0;
}

private void auto_use_backend()
{
  callout = call_out;
}

protected function(function(mixed ...:void), int|float, mixed ...:mixed)
  callout = ((callout = callnow), call_out(auto_use_backend, 0), callout);

//! Value that will be provided asynchronously
//! sometime in the future.
//!
//! @seealso
//!   @[Promise]
class Future
{
  mixed result;
  State state;

  protected array(array(function(mixed, mixed ...: void)|mixed))
    success_cbs = ({});
  protected array(array(function(mixed, mixed ...: void)|mixed))
    failure_cbs = ({});

  protected Pike.Backend backend;

  //! Set the backend to use for calling any callbacks.
  //!
  //! @note
  //!   This overides the mode set by @[use_backend()].
  //!
  //! @seealso
  //!   @[get_backend()], @[use_backend()]
  void set_backend(Pike.Backend backend)
  {
    this::backend = backend;
  }

  //! Get the backend (if any) used to call any callbacks.
  //!
  //! This returns the value set by @[set_backend()].
  //!
  //! @seealso
  //!   @[set_backend()]
  Pike.Backend get_backend()
  {
    return backend;
  }

  //! Create a new @[Promise] with the same base settings
  //! as the current object.
  //!
  //! Overload this function if you need to propagate more state
  //! to new @[Promise] objects.
  //!
  //! The default implementation copies the backend
  //! setting set with @[set_backend()] to the new @[Promise].
  //!
  //! @seealso
  //!   @[Promise], @[set_backend()]
  Promise promise_factory()
  {
    Promise res = Promise();

    if (backend) {
      res->set_backend(backend);
    }

    return res;
  }

  //! Call a callback function.
  //!
  //! @param cb
  //!   Callback function to call.
  //!
  //! @param args
  //!   Arguments to call @[cb] with.
  //!
  //! The default implementation calls @[cb] via the
  //! backend set via @[set_backend()] (if any), and
  //! otherwise falls back the the mode set by
  //! @[use_backend()].
  //!
  //! @seealso
  //!   @[set_backend()], @[use_backend()]
  protected void call_callback(function cb, mixed ... args)
  {
    (backend ? backend->call_out : callout)(cb, 0, @args);
  }

  //! Wait for fulfillment.
  //!
  //! @seealso
  //!   @[get()]
  this_program wait()
  {
    if (state <= STATE_PENDING) {
      Thread.MutexKey key = mux->lock();
      while (state <= STATE_PENDING) {
	cond->wait(key);
      }
    }

    return this;
  }

  //! Wait for fulfillment and return the value.
  //!
  //! @throws
  //!   Throws on rejection.
  //!
  //! @seealso
  //!   @[wait()]
  mixed get()
  {
    wait();

    if (state >= STATE_REJECTED) {
      throw(result);
    }
    return result;
  }

  //! Register a callback that is to be called on fulfillment.
  //!
  //! @param cb
  //!   Function to be called. The first argument will be the
  //!   result of the @[Future].
  //!
  //! @param extra
  //!   Any extra context needed for @[cb]. They will be provided
  //!   as arguments two and onwards when @[cb] is called.
  //!
  //! @note
  //!   @[cb] will always be called from the main backend.
  //!
  //! @seealso
  //!   @[on_failure()], @[query_success_callbacks()]
  this_program on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    switch (state) {
      case STATE_FULFILLED:
        call_callback(cb, result, @extra);
        break;
      case STATE_NO_FUTURE:
      case STATE_PENDING:
        // Rely on interpreter lock to add to success_cbs before state changes
        // again
        success_cbs += ({ ({ cb, @extra }) });
    }
    return this_program::this;
  }

  //! Query the set of active success callbacks.
  //!
  //! @returns
  //!   Returns an array with callback functions.
  //!
  //! @seealso
  //!   @[on_success()], @[query_failure_callbacks()]
  array(function) query_success_callbacks()
  {
    return column(success_cbs, 0);
  }

  //! Register a callback that is to be called on failure.
  //!
  //! @param cb
  //!   Function to be called. The first argument will be the
  //!   failure result of the @[Future].
  //!
  //! @param extra
  //!   Any extra context needed for @[cb]. They will be provided
  //!   as arguments two and onwards when @[cb] is called.
  //!
  //! @note
  //!   @[cb] will always be called from the main backend.
  //!
  //! @seealso
  //!   @[on_success()], @[query_failure_callbacks()]
  this_program on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    switch (state) {
      case STATE_REJECTED:
	state = STATE_REJECTION_REPORTED;
	// FALL_THROUGH
      case STATE_REJECTION_REPORTED:
        call_callback(cb, result, @extra);
        break;
      case STATE_NO_FUTURE:
      case STATE_PENDING:
        // Rely on interpreter lock to add to failure_cbs before state changes
        // again
        failure_cbs += ({ ({ cb, @extra }) });
    }
    return this_program::this;
  }

  //! Query the set of active failure callbacks.
  //!
  //! @returns
  //!   Returns an array with callback functions.
  //!
  //! @seealso
  //!   @[on_failure()], @[query_success_callbacks()]
  array(function) query_failure_callbacks()
  {
    return column(failure_cbs, 0);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the result.
  protected void apply(mixed val, Promise p,
		       function(mixed, mixed ... : mixed) fun,
		       array(mixed) ctx)
  {
    mixed f;
    if (mixed err = catch (f = fun(val, @ctx)))
      p->failure(err);
    else
      p->success(f);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the eventual result.
  protected void apply_flat(mixed val, Promise p,
			    function(mixed, mixed ... : Future) fun,
			    array(mixed) ctx)
  {
    Future f;
    {
      if (mixed err = catch (f = fun(val, @ctx))) {
        p->failure(err);
        return;
      }
    }
    if (!objectp(f) || !f->on_failure || !f->on_success)
      error("Expected %O to return a Future. Got: %O.\n", fun, f);
    f->on_failure(p->failure)->on_success(p->success);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the eventual result.
  protected void apply_smart(mixed val, Promise p,
			    function(mixed, mixed ... : mixed|Future) fun,
			    array(mixed) ctx)
  {
    mixed|Future f;
    {
      if (mixed err = catch (f = fun(val, @ctx))) {
        p->failure(err);
        return;
      }
    }
    if (!objectp(f)
     || !functionp(f->on_failure) || !functionp(f->on_success))
      p->success(f);
    else
      f->on_failure(p->failure)->on_success(p->success);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with @[val] if @[fun] didn't return false.
  //! If @[fun] returned false, fail @[p] with @expr{0@} as result.
  protected void apply_filter(mixed val, Promise p,
			      function(mixed, mixed ... : int(0..1)) fun,
			      array(mixed) ctx)
  {
    int bool;
    mixed err;
    if (!(err = catch (bool = fun(val, @ctx))) && bool)
      p->success(val);
    else
      p->failure(err);
  }

  //! This specifies a callback that is only called on success, and
  //! allows you to alter the future.
  //!
  //! @param fun
  //!   Function to be called. The first argument will be the
  //!   @b{success@} result of @b{this@} @[Future].
  //!   The return value will be the success result of the new @[Future].
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{fun@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your @[fun] returns a regular value (i.e.
  //!   @b{not@} a @[Future]).
  //!
  //! @seealso
  //!  @[map_with()], @[transform()], @[recover()]
  this_program map(function(mixed, mixed ... : mixed) fun, mixed ... extra)
  {
    Promise p = promise_factory();
    on_failure(p->failure);
    on_success(apply, p, fun, extra);
    return p->future();
  }

  //! This specifies a callback that is only called on success, and
  //! allows you to alter the future.
  //!
  //! @param fun
  //!   Function to be called. The first argument will be the
  //!   @b{success@} result of @b{this@} @[Future].
  //!   The return value must be a @[Future] that promises
  //!   the new result.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{fun@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your @[fun] returns a @[Future] again.
  //!
  //! @seealso
  //!  @[map()], @[transform_with()], @[recover_with()], @[flat_map]
  this_program map_with(function(mixed, mixed ... : this_program) fun,
			mixed ... extra)
  {
    Promise p = promise_factory();
    on_failure(p->failure);
    on_success(apply_flat, p, fun, extra);
    return p->future();
  }

  //! This is an alias for @[map_with()].
  //!
  //! @seealso
  //!   @[map_with()]
  inline this_program flat_map(function(mixed, mixed ... : this_program) fun,
			       mixed ... extra)
  {
    return map_with(fun, @extra);
  }

  //! This specifies a callback that is only called on failure, and
  //! allows you to alter the future into a success.
  //!
  //! @param fun
  //!   Function to be called. The first argument will be the
  //!   @b{failure@} result of @b{this@} @[Future].
  //!   The return value will be the success result of the new @[Future].
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{fun@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your callbacks return a regular value (i.e.
  //!   @b{not@} a @[Future]).
  //!
  //! @seealso
  //!   @[recover_with()], @[map()], @[transform()]
  this_program recover(function(mixed, mixed ... : mixed) fun,
		       mixed ... extra)
  {
    Promise p = promise_factory();
    on_success(p->success);
    on_failure(apply, p, fun, extra);
    return p->future();
  }

  //! This specifies a callback that is only called on failure, and
  //! allows you to alter the future into a success.
  //!
  //! @param fun
  //!   Function to be called. The first argument will be the
  //!   @b{failure@} result of @b{this@} @[Future].
  //!   The return value must be a @[Future] that promises
  //!   the new success result.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{fun@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your callbacks return a @[Future] again.
  //!
  //! @seealso
  //!   @[recover()], @[map_with()], @[transform_with()]
  this_program recover_with(function(mixed, mixed ... : this_program) fun,
			    mixed ... extra)
  {
    Promise p = promise_factory();
    on_success(p->success);
    on_failure(apply_flat, p, fun, extra);
    return p->future();
  }

  //! This specifies a callback that is only called on success, and
  //! allows you to selectively alter the future into a failure.
  //!
  //! @param fun
  //!   Function to be called. The first argument will be the
  //!   @b{success@} result of @b{this@} @[Future].
  //!   If the return value is @expr{true@}, the future succeeds with
  //!   the original success result.
  //!   If the return value is @expr{false@}, the future fails with
  //!   an @[UNDEFINED] result.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{fun@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @seealso
  //!   @[transform()]
  this_program filter(function(mixed, mixed ... : int(0..1)) fun,
		      mixed ... extra)
  {
    Promise p = promise_factory();
    on_failure(p->failure);
    on_success(apply_filter, p, fun, extra);
    return p->future();
  }

  //! This specifies callbacks that allow you to alter the future.
  //!
  //! @param success
  //!   Function to be called. The first argument will be the
  //!   @b{success@} result of @b{this@} @[Future].
  //!   The return value will be the success result of the new @[Future].
  //!
  //! @param failure
  //!   Function to be called. The first argument will be the
  //!   @b{failure@} result of @b{this@} @[Future].
  //!   The return value will be the success result of the new @[Future].
  //!   If this callback is omitted, it will default to the same callback as
  //!   @expr{success@}.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{success@} and @expr{failure@}. They will be provided
  //!   as arguments two and onwards when the callbacks are called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your callbacks return a regular value (i.e.
  //!   @b{not@} a @[Future]).
  //!
  //! @seealso
  //!   @[transform_with()], @[map()], @[recover()]
  this_program transform(function(mixed, mixed ... : mixed) success,
			 function(mixed, mixed ... : mixed)|void failure,
			 mixed ... extra)
  {
    Promise p = promise_factory();
    on_success(apply, p, success, extra);
    on_failure(apply, p, failure || success, extra);
    return p->future();
  }

  //! This specifies callbacks that allow you to alter the future.
  //!
  //! @param success
  //!   Function to be called. The first argument will be the
  //!   @b{success@} result of @b{this@} @[Future].
  //!   The return value must be a @[Future] that promises
  //!   the new result.
  //!
  //! @param failure
  //!   Function to be called. The first argument will be the
  //!   @b{failure@} result of @b{this@} @[Future].
  //!   The return value must be a @[Future] that promises
  //!   the new success result.
  //!   If this callback is omitted, it will default to the same callback as
  //!   @expr{success@}.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{success@} and @expr{failure@}. They will be provided
  //!   as arguments two and onwards when the callbacks are called.
  //!
  //! @returns
  //!   The new @[Future].
  //!
  //! @note
  //!  This method is used if your callbacks return a @[Future] again.
  //!
  //! @seealso
  //!   @[transform()], @[map_with()], @[recover_with]
  this_program transform_with(function(mixed, mixed ... : this_program) success,
		         function(mixed, mixed ... : this_program)|void failure,
			      mixed ... extra)
  {
    Promise p = promise_factory();
    on_success(apply_flat, p, success, extra);
    on_failure(apply_flat, p, failure || success, extra);
    return p->future();
  }

  //! @param others
  //!  The other futures (results) you want to append.
  //!
  //! @returns
  //! A new @[Future] that will be fulfilled with an
  //! array of the fulfilled result of this object followed
  //! by the fulfilled results of other futures.
  //!
  //! @seealso
  //!   @[results()]
  this_program zip(array(this_program) others)
  {
    if (sizeof(others))
      return results(({ this_program::this }) + others);
    return this_program::this;
  }
  inline variant this_program zip(this_program ... others)
  {
    return zip(others);
  }

  //! JavaScript Promise API close but not identical equivalent
  //! of a combined @[transform()] and @[transform_with()].
  //!
  //! @param onfulfilled
  //!   Function to be called on fulfillment. The first argument will be the
  //!   result of @b{this@} @[Future].
  //!   The return value will be the result of the new @[Future].
  //!   If the return value already is a @[Future], pass it as-is.
  //!
  //! @param onrejected
  //!   Function to be called on failure. The first argument will be the
  //!   failure result of @b{this@} @[Future].
  //!   The return value will be the failure result of the new @[Future].
  //!   If the return value already is a @[Future], pass it as-is.
  //!
  //! @param extra
  //!   Any extra context needed for @expr{onfulfilled@} and
  //!   @expr{onrejected@}. They will be provided
  //!   as arguments two and onwards when the callbacks are called.
  //!
  //! @returns
  //! The new @[Future].
  //!
  //! @seealso
  //!   @[transform()], @[transform_with()], @[thencatch()],
  //!   @[on_success()], @[Promise.success()],
  //!   @[on_failure()], @[Promise.failure()],
  //!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
  this_program then(void|function(mixed, mixed ... : mixed) onfulfilled,
   void|function(mixed, mixed ... : mixed) onrejected,
   mixed ... extra) {
    Promise p = promise_factory();
    if (onfulfilled)
      on_success(apply_smart, p, onfulfilled, extra);
    else
      on_success(p->success);
    if (onrejected)
      on_failure(apply_smart, p, onrejected, extra);
    else
      on_failure(p->failure);
    return p->future();
  }

  //! JavaScript Promise API equivalent of a combination of @[recover()]
  //! and @[recover_with()].
  //!
  //! @param onrejected
  //!   Function to be called. The first argument will be the
  //!   failure result of @b{this@} @[Future].
  //!   The return value will the failure result of the new @[Future].
  //!   If the return value already is a @[Future], pass it as-is.
  //!
  //! @param extra
  //!   Any extra context needed for
  //!   @expr{onrejected@}. They will be provided
  //!   as arguments two and onwards when the callback is called.
  //!
  //! @returns
  //! The new @[Future].
  //!
  //! @seealso
  //!   @[recover()], @[recover_with()], @[then()], @[on_failure()],
  //!   @[Promise.failure()],
  //!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
  inline this_program thencatch(function(mixed, mixed ... : mixed) onrejected,
   mixed ... extra) {
    return then(0, onrejected, @extra);
  }

  private this_program setup_call_out(int|float seconds, void|int tout)
  {
    array call_out_handle;
    Promise p = promise_factory();
    void cancelcout(mixed value)
    {
      (backend ? backend->remove_call_out : remove_call_out)(call_out_handle);
      p->try_success(0);
    }
    /* NB: try_* variants as the original promise may get fulfilled
     *     after the timeout has occurred.
     */
    on_failure(cancelcout);
    call_out_handle = (backend ? backend->call_out : call_out)
      (p[tout ? "try_failure" : "try_success"], seconds,
       tout && ({ "Timeout.\n", backtrace() }));
    if (tout)
      on_success(cancelcout);
    return p->future();
  }

  //! Return a @[Future] that will either be fulfilled with the fulfilled
  //! result of this @[Future], or be failed after @[seconds] have expired.
  this_program timeout(int|float seconds)
  {
    return first_completed(
     ({ this_program::this, setup_call_out(seconds, 1) })
    );
  }

  //! Return a @[Future] that will be fulfilled with the fulfilled
  //! result of this @[Future], but not until at least @[seconds] have passed.
  this_program delay(int|float seconds)
  {
    return results(
     ({ this_program::this, setup_call_out(seconds) })
    )->map(`[], 0);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("%O(%s,%O)", this_program,
                             ([ STATE_NO_FUTURE : "no future",
			        STATE_PENDING : "pending",
                                STATE_REJECTED : "rejected",
				STATE_REJECTION_REPORTED : "rejection_reported",
                                STATE_FULFILLED : "fulfilled" ])[state],
                             result);
  }
}

protected class AggregateState
{
  private Promise promise;
  private int(0..) promises;
  private int(0..) succeeded, failed;
  final array(mixed) results;
  final int(0..) min_failures;
  final int(-1..) max_failures;
  final mixed accumulator;
  final function(mixed, mixed, mixed ... : mixed) fold_fun;
  final array(mixed) extra;

  protected void create(Promise p)
  {
    if (p->_materialised || p->_materialised++)
      error("Cannot materialise a Promise more than once.\n");
    promise = p;
  }

  final void materialise()
  {
    if (promise->_astate)
    {
      promise->_astate = 0;
      if (results)
      {
        promises = sizeof(results);
        array(Future) futures = results;
        if (fold_fun)
          results = 0;
        foreach(futures; int idx; Future f)
          f->on_failure(cb_failure, idx)->on_success(cb_success, idx);
      }
    }
  }

  private void fold_one(mixed val)
  {
    mixed err = catch (accumulator = fold_fun(val, accumulator, @extra));
    if (err && promise)
      promise->failure(err);
  }

  private void fold(function(mixed:void) failsucc)
  {
    failsucc(fold_fun ? accumulator : results);
    results = 0;			// Free memory
  }

  private void cb_failure(mixed value, int idx)
  {
    Promise p;				// Cache it, to cover a failure race
    if (p = promise)
    {
      Thread.MutexKey key = mux->lock();
      do
      {
        if (p->state <= STATE_PENDING)
        {
          ++failed;
          if (max_failures < failed && max_failures >= 0)
          {
            key = 0;
            p->try_failure(value);
            break;
          }
          int success = succeeded + failed == promises;
          key = 0;
          if (results)
            results[idx] = value;
          else
            fold_one(value);
          if (success)
          {
            fold(failed >= min_failures ? p->success : p->failure);
            break;
          }
        }
        return;
      } while (0);
      promise = 0;			// Free backreference
    }
  }

  private void cb_success(mixed value, int idx)
  {
    Promise p;				// Cache it, to cover a failure race
    if (p = promise)
    {
      Thread.MutexKey key = mux->lock();
      do
      {
        if (p->state <= STATE_PENDING)
        {
          ++succeeded;
          if (promises - min_failures < succeeded)
          {
            key = 0;
            p->try_failure(value);
            break;
          }
          int success = succeeded + failed == promises;
          key = 0;
          if (results)
            results[idx] = value;
          else
            fold_one(value);
          if (success)
          {
            fold(p->success);
            break;
          }
        }
        return;
      } while (0);
      promise = 0;			// Free backreference
    }
  }
}

//! Promise to provide a @[Future] value.
//!
//! Objects of this class are typically kept internal to the
//! code that provides the @[Future] value. The only thing
//! that is directly returned to the user is the return
//! value from @[future()].
//!
//! @seealso
//!   @[Future], @[future()]
class Promise
{
  inherit Future;

  final int _materialised;
  final AggregateState _astate;

  //! Creates a new promise, optionally initialised from a traditional callback
  //! driven method via @expr{executor(success, failure, @@extra)@}.
  //!
  //! @seealso
  //!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
  protected void create(void|
   function(function(mixed:void),
            function(mixed:void), mixed ...:void) executor, mixed ... extra)
  {
    state = STATE_NO_FUTURE;
    if (executor)
      executor(success, failure, @extra);
  }

  Future on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    if (_astate)
      _astate->materialise();
    return ::on_success(cb, @extra);
  }

  Future on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    if (_astate)
      _astate->materialise();
    return ::on_failure(cb, @extra);
  }

  Future wait()
  {
    if (_astate)
      _astate->materialise();
    return ::wait();
  }

  mixed get()
  {
    if (_astate)
      _astate->materialise();
    return ::get();
  }

  //! The future value that we promise.
  Future future()
  {
    if (state == STATE_NO_FUTURE) state = STATE_PENDING;
    return Future::this;
  }

  protected this_program finalise(State newstate, mixed value, int try,
    array(array(function(mixed, mixed ...: void)|array(mixed))) cbs)
  {
    Thread.MutexKey key = mux->lock();
    if (state <= STATE_PENDING)
    {
      state = newstate;
      result = value;
      key = 0;
      cond->broadcast();
      if (sizeof(cbs))
      {
        foreach(cbs; ; array cb)
          if (cb)
            call_callback(cb[0], value, @cb[1..]);
	if (newstate == STATE_REJECTED) {
	  state = STATE_REJECTION_REPORTED;
	}
      }
      failure_cbs = success_cbs = 0;		// Free memory and references
    }
    else
    {
      key = 0;
      if (!try)
        error("Promise has already been finalised.\n");
    }
    return this_program::this;
  }

  //! @decl this_program success(mixed value)
  //!
  //! Fulfill the @[Future].
  //!
  //! @param value
  //!   Result of the @[Future].
  //!
  //! @throws
  //!   Throws an error if the @[Future] already has been fulfilled
  //!   or failed.
  //!
  //! Mark the @[Future] as fulfilled, and schedule the @[on_success()]
  //! callbacks to be called as soon as possible.
  //!
  //! @seealso
  //!   @[try_success()], @[try_failure()], @[failure()], @[on_success()]
  this_program success(mixed value, void|int try)
  {
    return finalise(STATE_FULFILLED, value, try, success_cbs);
  }

  //! Fulfill the @[Future] if it hasn't been fulfilled or failed already.
  //!
  //! @param value
  //!   Result of the @[Future].
  //!
  //! Mark the @[Future] as fulfilled if it hasn't already been fulfilled
  //! or failed, and in that case schedule the @[on_success()] callbacks
  //! to be called as soon as possible.
  //!
  //! @seealso
  //!   @[success()], @[try_failure()], @[failure()], @[on_success()]
  inline this_program try_success(mixed value)
  {
    return (state > STATE_PENDING) ? this_program::this : success(value, 1);
  }

  //! @decl this_program failure(mixed value)
  //!
  //! Reject the @[Future] value.
  //!
  //! @param value
  //!   Failure result of the @[Future].
  //!
  //! @throws
  //!   Throws an error if the @[Future] already has been fulfilled
  //!   or failed.
  //!
  //! Mark the @[Future] as failed, and schedule the @[on_failure()]
  //! callbacks to be called as soon as possible.
  //!
  //! @seealso
  //!   @[try_failure()], @[success()], @[on_failure()]
  this_program failure(mixed value, void|int try)
  {
    return
     finalise(STATE_REJECTED, value, try, failure_cbs);
  }

  //! Maybe reject the @[Future] value.
  //!
  //! @param value
  //!   Failure result of the @[Future].
  //!
  //! Mark the @[Future] as failed if it hasn't already been fulfilled,
  //! and in that case schedule the @[on_failure()] callbacks to be
  //! called as soon as possible.
  //!
  //! @seealso
  //!   @[failure()], @[success()], @[on_failure()]
  inline this_program try_failure(mixed value)
  {
    return (state > STATE_PENDING) ? this_program::this : failure(value, 1);
  }

  inline private void fill_astate()
  {
    if (!_astate)
      _astate = AggregateState(this);
  }

  //! Add futures to the list of futures which the current object depends upon.
  //!
  //! If called without arguments it will produce a new @[Future]
  //! from a new @[Promise] which is implictly added to the dependency list.
  //!
  //! @param futures
  //!   The list of @expr{futures@} we want to add to the list we depend upon.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @note
  //!  Can be called multiple times to add more.
  //!
  //! @note
  //!  Once the promise has been materialised (when either @[on_success()],
  //!  @[on_failure()] or @[get()] has been called on this object), it is
  //!  not possible to call @[depend()] anymore.
  //!
  //! @seealso
  //!   @[fold()], @[first_completed()], @[max_failures()], @[min_failures()],
  //!   @[any_results()], @[Concurrent.results()], @[Concurrent.all()]
  this_program depend(array(Future) futures)
  {
    if (sizeof(futures)) {
      fill_astate();
      _astate->results += futures;
    }
    return this_program::this;
  }
  inline variant this_program depend(Future ... futures)
  {
    return depend(futures);
  }
  variant this_program depend()
  {
    Promise p = promise_factory();
    depend(p->future());
    return p;
  }

  //! @param initial
  //!   Initial value of the accumulator.
  //!
  //! @param fun
  //!   Function to apply. The first argument is the result of
  //!   one of the @[futures].  The second argument is the current value
  //!   of the accumulator.
  //!
  //! @param extra
  //!   Any extra context needed for @[fun]. They will be provided
  //!   as arguments three and onwards when @[fun] is called.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @note
  //!   If @[fun] throws an error it will fail the @[Future].
  //!
  //! @note
  //!   @[fun] may be called in any order, and will be called
  //!   once for every @[Future] it depends upon, unless one of the
  //!   calls fails in which case no further calls will be
  //!   performed.
  //!
  //! @seealso
  //!   @[depend()], @[Concurrent.fold()]
  this_program fold(mixed initial,
	            function(mixed, mixed, mixed ... : mixed) fun,
	            mixed ... extra)
  {
    if (_astate) {
      _astate->accumulator = initial;
      _astate->fold_fun = fun;
      _astate->extra = extra;
      _astate->materialise();
    } else
      success(initial);
    return this_program::this;
  }

  //! It evaluates to the first future that completes of the list
  //! of futures it depends upon.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @seealso
  //!   @[depend()], @[Concurrent.first_completed()]
  this_program first_completed()
  {
    if (_astate) {
      _astate->results->on_failure(try_failure)->on_success(try_success);
      _astate = 0;
    } else
      success(0);
    return this_program::this;
  }

  //! @param max
  //!   Specifies the maximum number of failures to be accepted in
  //!   the list of futures this promise depends upon.
  //!
  //!   @expr{-1@} means unlimited.
  //!
  //!   Defaults to @expr{0@}.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @seealso
  //!   @[depend()], @[min_failures()], @[any_results()]
  this_program max_failures(int(-1..) max)
  {
    fill_astate();
    _astate->max_failures = max;
    return this_program::this;
  }

  //! @param min
  //!   Specifies the minimum number of failures to be required in
  //!   the list of futures this promise depends upon.  Defaults
  //!   to @expr{0@}.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @seealso
  //!   @[depend()], @[max_failures()]
  this_program min_failures(int(0..) min)
  {
    fill_astate();
    _astate->min_failures = min;
    return this_program::this;
  }

  //! Sets the number of failures to be accepted in the list of futures
  //! this promise
  //! depends upon to unlimited.  It is equivalent to @expr{max_failures(-1)@}.
  //!
  //! @returns
  //! The new @[Promise].
  //!
  //! @seealso
  //!   @[depend()], @[max_failures()]
  this_program any_results()
  {
    return max_failures(-1);
  }

  protected void _destruct()
  {
    // NB: Don't complain about dropping STATE_NO_FUTURE on the floor.
    if (state == STATE_PENDING)
      try_failure(({ sprintf("%O: Promise broken.\n", this), backtrace() }));
    if ((state == STATE_REJECTED) && global_on_failure)
      call_callback(global_on_failure, result);
    result = UNDEFINED;
  }
}

//! @returns
//! A @[Future] that represents the first
//! of the @expr{futures@} that completes.
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
//!
//! @seealso
//!   @[race()], @[Promise.first_completed()]
variant Future first_completed(array(Future) futures)
{
  return Promise()->depend(futures)->first_completed()->future();
}
variant inline Future first_completed(Future ... futures)
{
  return first_completed(futures);
}

//! JavaScript Promise API equivalent of @[first_completed()].
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
//!
//! @seealso
//!   @[first_completed()], @[Promise.first_completed()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
variant inline Future race(array(Future) futures)
{
  return first_completed(futures);
}
variant inline Future race(Future ... futures)
{
  return first_completed(futures);
}

//! @returns
//! A @[Future] that represents the array of all the completed @expr{futures@}.
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
//!
//! @seealso
//!   @[all()], @[Promise.depend()]
variant Future results(array(Future) futures)
{
  if(!sizeof(futures))
    return resolve(({}));

  return Promise()->depend(futures)->future();
}
inline variant Future results(Future ... futures)
{
  return results(futures);
}

//! JavaScript Promise API equivalent of @[results()].
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
//!
//! @seealso
//!   @[results()], @[Promise.depend()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
inline variant Future all(array(Future) futures)
{
  return results(futures);
}
inline variant Future all(Future ... futures)
{
  return results(futures);
}

//! @returns
//! A new @[Future] that has already failed for the specified @expr{reason@}.
//!
//! @note
//!   The returned @[Future] does NOT have a backend set.
//!
//! @seealso
//!   @[Future.on_failure()], @[Promise.failure()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
Future reject(mixed reason)
{
  return Promise()->failure(reason)->future();
}

//! @returns
//! A new @[Future] that has already been fulfilled with @expr{value@}
//! as result.  If @expr{value@} is an object which already
//! has @[on_failure] and @[on_success] methods, return it unchanged.
//!
//! @note
//! This function can be used to ensure values are futures.
//!
//! @note
//!   The returned @[Future] does NOT have a backend set.
//!
//! @seealso
//!   @[Future.on_success()], @[Promise.success()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
Future resolve(mixed value)
{
  if (objectp(value) && value->on_failure && value->on_success)
    return value;
  return Promise()->success(value)->future();
}

//! Return a @[Future] that represents the array of mapping @[fun]
//! over the results of the completed @[futures].
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
Future traverse(array(Future) futures,
		function(mixed, mixed ... : mixed) fun,
		mixed ... extra)
{
  return results(futures->map(fun, @extra));
}

//! Return a @[Future] that represents the accumulated results of
//! applying @[fun] to the results of the @[futures] in turn.
//!
//! @param initial
//!   Initial value of the accumulator.
//!
//! @param fun
//!   Function to apply. The first argument is the result of
//!   one of the @[futures], the second the current accumulated
//!   value, and any further from @[extra].
//!
//! @note
//!   If @[fun] throws an error it will fail the @[Future].
//!
//! @note
//!   @[fun] may be called in any order, and will be called
//!   once for every @[Future] in @[futures], unless one of
//!   calls fails in which case no further calls will be
//!   performed.
//!
//! @note
//!   The returned @[Future] does NOT have any state (eg backend)
//!   propagated from the @[futures]. This must be done by hand.
Future fold(array(Future) futures,
	    mixed initial,
	    function(mixed, mixed, mixed ... : mixed) fun,
	    mixed ... extra)
{
  return Promise()->depend(futures)->fold(initial, fun, extra)->future();
}
