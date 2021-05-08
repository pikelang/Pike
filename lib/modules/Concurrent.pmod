#pike __REAL_VERSION__

// Enable CONCURRENT_DEBUG to get more meaningful broken-promise messages

// #define CONCURRENT_DEBUG

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

//! Value that will be provided asynchronously sometime in the
//! future. A Future object is typically produced from a @[Promise]
//! object by calling its @[future()] method.
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
  //!   @[get()], @[try_get()]
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
  //!   @[wait()], @[try_get()]
  mixed get()
  {
    wait();

    if (state >= STATE_REJECTED) {
      throw(result);
    }
    return result;
  }

  //! Return the value if available.
  //!
  //! @returns
  //!   Returns @[UNDEFINED] if the @[Future] is not yet fulfilled.
  //!
  //! @throws
  //!   Throws on rejection.
  //!
  //! @seealso
  //!   @[wait()]
  mixed try_get()
  {
    switch(state) {
    case ..STATE_PENDING:
      return UNDEFINED;
    case STATE_REJECTED..:
      throw(result);
    default:
      return result;
    }
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
    Thread.MutexKey key = mux->lock();
    switch (state) {
      case STATE_FULFILLED:
	key = 0;
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
    Thread.MutexKey key = mux->lock();
    switch (state) {
      case STATE_REJECTED:
	state = STATE_REJECTION_REPORTED;
	// FALL_THROUGH
      case STATE_REJECTION_REPORTED:
	key = 0;
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

    if (executor) {
      mixed err = catch(executor(success, failure, @extra));

      // This unfortunately does hide the real error in case of
      // double-finalization, i.e.
      //   fail("I reject!");
      //   error("I rejected.\n");
      // in the executor will leave our caller with an error about the
      // Promise already having been finalized, not giving anyone too much
      // information about where the double-finalization occured...
      if (err) {
        if (mixed err2 = catch(failure(err))) {
          string finalisation_error;

          if (catch(finalisation_error = describe_error(err2)))
            finalisation_error = sprintf("%O", err2);

          while (has_suffix(finalisation_error, "\n"))
            finalisation_error = finalisation_error[..<1];

          error("%O->create(): Got error \"%s\" while trying to finalise promise with %O thrown by executor.\n",
                this, finalisation_error, err);
        }
      }
    }
  }

  //! The future value that we promise.
  Future future()
  {
#ifdef CONCURRENT_DEBUG
    werror("Promise: %O\n", this_function);
#endif
    if (state == STATE_NO_FUTURE) state = STATE_PENDING;
    return Future::this;
  }

  protected this_program finalise(State newstate, mixed value, int try)
  {
    Thread.MutexKey key = mux->lock();
    if (state <= STATE_PENDING)
    {
      state = newstate;
      result = value;
      array(array(function(mixed, mixed ...: void)|array(mixed))) cbs;
      if (state == STATE_FULFILLED) {
	cbs = success_cbs;
      } else {
	cbs = failure_cbs;
      }
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
#ifdef CONCURRENT_DEBUG
    werror("Promise: %O\n", this_function);
#endif
    return finalise(STATE_FULFILLED, value, try);
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
#ifdef CONCURRENT_DEBUG
    werror("Promise: %O\n", this_function);
#endif
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
#ifdef CONCURRENT_DEBUG
    werror("Promise: %O\n", this_function);
#endif
    return
     finalise(STATE_REJECTED, value, try);
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
#ifdef CONCURRENT_DEBUG
    werror("Promise: %O\n", this_function);
#endif
    return (state > STATE_PENDING) ? this_program::this : failure(value, 1);
  }

  private string orig_backtrace =
    sprintf("%s\n------\n", describe_backtrace(backtrace()));

  protected void _destruct()
  {
    // NB: Don't complain about dropping STATE_NO_FUTURE on the floor.
    if (state == STATE_PENDING)
      try_failure(({ sprintf("%O: Promise broken.\n%s",
			     this, orig_backtrace),
		     backtrace() }));
    if ((state == STATE_REJECTED) && global_on_failure)
      call_callback(global_on_failure, result);
    result = UNDEFINED;
  }
}

//! Promise to provide an aggregated @[Future] value.
//!
//! Objects of this class are typically kept internal to the
//! code that provides the @[Future] value. The only thing
//! that is directly returned to the user is the return
//! value from @[future()].
//!
//! @note
//!   It is currently possible to use this class as a normal @[Promise]
//!   (ie without aggregation), but those functions may get removed
//!   in a future version of Pike. Functions to avoid include
//!   @[success()] and @[try_success()]. If you do not need aggregation
//!   use @[Promise].
//!
//! @seealso
//!   @[Future], @[future()], @[Promise], @[first_completed()],
//!   @[race()], @[results()], @[all()], @[fold()]
class AggregatedPromise
{
  inherit Promise;

  //! @array
  //!   @elem mapping(int:mixed) 0
  //!     Successful results.
  //!   @elem mapping(int:mixed) 1
  //!     Failed results.
  //! @endarray
  protected array(mapping(int:mixed)) dependency_results = ({ ([]), ([]) });

  protected int(0..) num_dependencies;
  protected int(1bit) started;
  protected int(0..) min_failure_threshold;
  protected int(-1..) max_failure_threshold;
  protected function(mixed, mixed, mixed ... : mixed) fold_fun;
  protected array(mixed) extra_fold_args;

  //! Callback used to aggregate the results from dependencies.
  //!
  //! @param value
  //!   Value received from the dependency.
  //!
  //! @param idx
  //!   Identification number for the dependency.
  //!
  //! @param results
  //!   Either of the mappings in @[dependency_results] depending on
  //!   whether this was a success or a failure callback.
  //!
  //! @note
  //!   The function may also be called with all arguments set to
  //!   @[UNDEFINED] in order to poll the current state. This is
  //!   typically done via a call to @[start()].
  //!
  //! @seealso
  //!   @[start()]
  protected void aggregate_cb(mixed value, int idx, mapping(int:mixed) results)
  {
    // Check if aggregation is to be performed.
    if (!num_dependencies) return;

    Thread.MutexKey key = mux->lock();
    mixed accumulator;
    if (state <= STATE_PENDING) {
      if (results) {
	results[idx] = value;
      }

      if (!started) {
	// Not started yet, so we don't know if all dependencies
	// have been added yet, or if the limits or fold function
	// are to be set.
	//
	// start() will call us again when it is called.
	destruct(key);
	return;
      }

      [int num_successes, int num_failures] =
	predef::map(dependency_results, sizeof);
      accumulator = result;
      destruct(key);

      // Note: Use try_{failure,success}() from this point forward,
      //       as we may race with a different thread.

      if (max_failure_threshold >= 0) {
	if (num_failures > max_failure_threshold) {
	  // Too many failures.
	  value = values(dependency_results[1])[0];
	  try_failure(value);
	  return;
	}
      }

      if ((num_successes + min_failure_threshold) > num_dependencies) {
	// Too many successes.
	// NB: num_failures in this case may very well be 0.
	try_failure(value);
	return;
      }

      if ((num_successes + num_failures) < num_dependencies) {
	// More results needed.
	return;
      }

      // All results have been received.
      if (num_failures < min_failure_threshold) {
	// Too few failures.
	// FIXME: Can this actually be reached?
	// FIXME: What is a suitable failure value?
	try_failure(value);
	return;
      }

      if (fold_fun) {
	foreach(values(dependency_results[0]), mixed val) {
	  // NB: The fold_fun may get called multiple times for
	  //     the same set of results. This is fine though,
	  //     as we retrieved the initial value for the
	  //     accumulator while we still held the lock.
	  mixed err = catch {
	      accumulator = fold_fun(accumulator, val, @extra_fold_args);
	    };
	  if (err) {
	    // Broken folding function, or the folding function
	    // wants to convert the success into a failure.
	    try_failure(err);
	    return;
	  }
	}
	try_success(accumulator);
      } else {
	// Convert the dependency_results[0] mapping into an array.
	//
	// NB: first_completed() (which sets num_dependencies to 1)
	//     also sets fold_fun, so num_dependencies will be
	//     correct here.
	try_success(predef::map(indices(allocate(num_dependencies)),
				dependency_results[0]));
      }
    }
  }

  //! Start the aggregation of values from dependencies.
  //!
  //! @note
  //!   This function is called from several functions. These
  //!   include @[on_success()], @[on_failure()], @[wait()],
  //!   @[get()], @[fold()] and @[first_completed()].
  //!
  //! @note
  //!   After this function has been called, several functions
  //!   may no longer be called. These include @[depend()],
  //!   @[fold()], @[first_completed()], @[max_failures()],
  //!   @[min_failures()], @[any_results()].
  protected void start()
  {
    if (!started) {
      started = 1;

      // Check if the results are already present (in the
      // aggregate results case).
      aggregate_cb(UNDEFINED, UNDEFINED, UNDEFINED);
    }
  }

  Future on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    start();
    return ::on_success(cb, @extra);
  }

  Future on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    start();
    return ::on_failure(cb, @extra);
  }

  Future wait()
  {
    start();
    return ::wait();
  }

  mixed get()
  {
    start();
    return ::get();
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
    if (started) {
      error("Sequencing error. Not possible to add more dependencies.\n");
    }
    if (sizeof(futures)) {
      foreach(futures, Future f) {
	int idx = num_dependencies++;
	f->on_failure(aggregate_cb, idx, dependency_results[1])->
	  on_success(aggregate_cb, idx, dependency_results[0]);
      }
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
    if (started) {
      error("Sequencing error. Not possible to add a folding function.\n");
    }
    result = initial;
    extra_fold_args = extra;
    fold_fun = fun;
    start();
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
    if (started) {
      error("Sequencing error. Not possible to switch to first completed.\n");
    }
    extra_fold_args = ({});
    fold_fun = lambda(mixed a, mixed b) { return b; };
    num_dependencies = 1;
    start();

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
    if (started) {
      error("Sequencing error. Not possible to set max failure threshold.\n");
    }
    max_failure_threshold  = max;
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
    if (started) {
      error("Sequencing error. Not possible to set min failure threshold.\n");
    }
    min_failure_threshold = min;
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
  return AggregatedPromise()->depend(futures)->first_completed()->future();
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

  return AggregatedPromise()->depend(futures)->future();
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
  return AggregatedPromise()->depend(futures)->fold(initial, fun, extra)->future();
}
