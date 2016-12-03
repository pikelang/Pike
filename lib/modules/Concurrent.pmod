#pike __REAL_VERSION__

//! Module for handling multiple concurrent events.
//!
//! The @[Future] and @[Promise] API was inspired by
//! @url{https://github.com/couchdeveloper/FutureLib@}.

protected enum State {
  STATE_PENDING = 0,
  STATE_FULFILLED,
  STATE_REJECTED,
};

protected Thread.Mutex mux = Thread.Mutex();
protected Thread.Condition cond = Thread.Condition();

//! Value that will be provided asynchronously
//! sometime in the future.
//!
//! @seealso
//!   @[Promise]
class Future
{
  mixed result = UNDEFINED;
  State state;

  protected array(array(function(mixed, mixed ...: void)|array(mixed)))
    success_cbs = ({});
  protected array(array(function(mixed, mixed ...: void)|array(mixed)))
    failure_cbs = ({});

  //! Wait for fulfillment and return the value.
  //!
  //! @throws
  //!   Throws on rejection.
  mixed get()
  {
    State s = state;
    mixed res = result;
    if (!s) {
      object key = mux->lock();
      while (!state) {
	cond->wait(key);
      }

      s = state;
      res = result;
      key = 0;
    }

    if (s == STATE_REJECTED) {
      throw(res);
    }
    return res;
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
  //!   @[on_failure()]
  this_program on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    object key = mux->lock();

    if (state == STATE_FULFILLED) {
      call_out(cb, 0, result, @extra);
      key = 0;
      return this;
    }

    success_cbs += ({ ({ cb, extra }) });
    key = 0;

    return this;
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
  //!   @[on_success()]
  this_program on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    object key = mux->lock();

    if (state == STATE_REJECTED) {
      call_out(cb, 0, result, @extra);
      key = 0;
      return this;
    }

    failure_cbs += ({ ({ cb, extra }) });
    key = 0;

    return this;
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the result.
  protected void apply(mixed val, Promise p,
		       function(mixed, mixed ... : mixed) fun,
		       array(mixed) ctx)
  {
    mixed err = catch {
	p->success(fun(val, @ctx));
	return;
      };
    p->failure(err);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the eventual result.
  protected void apply_flat(mixed val, Promise p,
			    function(mixed, mixed ... : Future) fun,
			    array(mixed) ctx)
  {
    mixed err = catch {
	Future f = fun(val, @ctx);
	if (!objectp(f) || !f->on_failure || !f->on_success) {
	  error("Expected %O to return a Future. Got: %O.\n",
		fun, f);
	}
	f->on_failure(p->failure);
	f->on_success(p->success);
	return;
      };
    p->failure(err);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with the eventual result.
  protected void apply_smart(mixed val, Promise p,
			    function(mixed, mixed ... : mixed|Future) fun,
			    array(mixed) ctx)
  {
    mixed err = catch {
	mixed|Future f = fun(val, @ctx);
	if (!objectp(f)
         || !functionp(f->on_failure) || !functionp(f->on_success)) {
	  p->success(f);
	  return;
	}
	f->on_failure(p->failure);
	f->on_success(p->success);
	return;
      };
    p->failure(err);
  }

  //! Apply @[fun] with @[val] followed by the contents of @[ctx],
  //! and update @[p] with @[val] if @[fun] didn't return false.
  //! If @[fun] returned false fail @[p] with @[UNDEFINED].
  protected void apply_filter(mixed val, Promise p,
			      function(mixed, mixed ... : int(0..1)) fun,
			      array(mixed) ctx)
  {
    mixed err = catch {
	if (fun(val, @ctx)) {
	  p->success(val);
	} else {
	  p->failure(UNDEFINED);
	}
	return;
      };
    p->failure(err);
  }

  //! Return a @[Future] that will be fulfilled with the result
  //! of applying @[fun] with the fulfilled result of this @[Future]
  //! followed by @[extra].
  this_program map(function(mixed, mixed ... : mixed) fun, mixed ... extra)
  {
    Promise p = Promise();
    on_failure(p->failure);
    on_success(apply, p, fun, extra);
    return p->future();
  }

  //! Return a @[Future] that will be fulfilled with the fulfilled result
  //! of applying @[fun] with the fulfilled result of this @[Future]
  //! followed by @[extra].
  this_program flat_map(function(mixed, mixed ... : this_program) fun,
			mixed ... extra)
  {
    Promise p = Promise();
    on_failure(p->failure);
    on_success(apply_flat, p, fun, extra);
    return p->future();
  }

  //! Return a @[Future] that will be fulfilled with either
  //! the fulfilled result of this @[Future], or the result
  //! of applying @[fun] with the failed result followed
  //! by @[extra].
  this_program recover(function(mixed, mixed ... : mixed) fun,
		       mixed ... extra)
  {
    Promise p = Promise();
    on_success(p->success);
    on_failure(apply, p, fun, extra);
    return p->future();
  }

  //! Return a @[Future] that will be fulfilled with either
  //! the fulfilled result of this @[Future], or the fulfilled result
  //! of applying @[fun] with the failed result followed
  //! by @[extra].
  this_program recover_with(function(mixed, mixed ... : this_program) fun,
			    mixed ... extra)
  {
    Promise p = Promise();
    on_success(p->success);
    on_failure(apply_flat, p, fun, extra);
    return p->future();
  }

  //! Return a @[Future] that either will by fulfilled by the
  //! fulfilled result of this @[Future] if applying @[fun]
  //! with the result followed by @[extra] returns true,
  //! or will fail with @[UNDEFINED] if it returns false.
  this_program filter(function(mixed, mixed ... : int(0..1)) fun,
		      mixed ... extra)
  {
    Promise p = Promise();
    on_failure(p->failure);
    on_success(apply_filter, p, fun, extra);
    return p->future();
  }

  //! Return a @[Future] that will be fulfilled with either
  //! the result of applying @[success] with the fulfilled result
  //! followed by @[extra], or the result of applying @[failure]
  //! with the failed result followed by @[extra].
  //!
  //! @[failure] defaults to @[success].
  //!
  //! @seealso
  //!   @[map]
  this_program transform(function(mixed, mixed ... : mixed) success,
			 function(mixed, mixed ... : mixed)|void failure,
			 mixed ... extra)
  {
    Promise p = Promise();
    on_success(apply, p, success, extra);
    on_failure(apply, p, failure || success, extra);
    return p->future();
  }

  //! Return a @[Future] that will be fulfilled with either
  //! the fulfilled result of applying @[success] with the fulfilled result
  //! followed by @[extra], or the fulfilled result of applying @[failure]
  //! with the failed result followed by @[extra].
  //!
  //! @[failure] defaults to @[success].
  //!
  //! @seealso
  //!   @[flat_map]
  this_program transform_with(function(mixed, mixed ... : Future) success,
			      function(mixed, mixed ... : Future)|void failure,
			      mixed ... extra)
  {
    Promise p = Promise();
    on_success(apply_flat, p, success, extra);
    on_failure(apply_flat, p, failure || success, extra);
    return p->future();
  }

  //! @returns
  //! A @[Future] that will be fulfilled with an
  //! array of the fulfilled result of this object followed
  //! by the fulfilled results of @[others].
  //!
  //! @seealso
  //!   @[results()]
  this_program zip(this_program ... others)
  {
    if (!sizeof(others)) return this_program::this;
    return results(({ this_program::this }) + others);
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
    Promise p = Promise();
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

  //! Return a @[Future] that will either be fulfilled with the fulfilled
  //! result of this @[Future], or be failed after @[seconds] have expired.
  this_program timeout(int|float seconds)
  {
    Promise p = Promise();
    on_failure(p->failure);
    on_success(p->success);
    call_out(p->try_failure, seconds, ({ "Timeout.\n", backtrace() }));
    return p->future();
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
  //! driven method via @expr{executor(resolve, reject, extra ... )@}.
  //!
  //! @seealso
  //!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
  protected void create(void|
   function(function(mixed:void),
            function(mixed:void), mixed ...:void) executor, mixed ... extra) {
    if (executor)
      executor(success, failure, @extra);
  }

  //! The future value that we promise.
  Future future()
  {
    return Future::this;
  }

  protected void unlocked_success(mixed value)
  {
    if (state < STATE_REJECTED) {
      result = value;
      state = STATE_FULFILLED;
      cond->broadcast();
      foreach(success_cbs,
	      [function(mixed, mixed ...: void) cb,
	       array(mixed) extra]) {
	if (cb) {
	  call_out(cb, 0, value, @extra);
	}
      }
    }
  }

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
  void success(mixed value)
  {
    if (state) error("Promise has already been finalized.\n");
    object key = mux->lock();
    if (state) error("Promise has already been finalized.\n");
    unlocked_success(value);
    key = 0;
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
  void try_success(mixed value)
  {
    if (state) return;
    object key = mux->lock();
    if (state) return;
    unlocked_success(value);
    key = 0;
  }

  protected void unlocked_failure(mixed value)
  {
    state = STATE_REJECTED;
    result = value;
    cond->broadcast();
    foreach(failure_cbs,
	    [function(mixed, mixed ...: void) cb,
	     array(mixed) extra]) {
      if (cb) {
	call_out(cb, 0, value, @extra);
      }
    }
  }

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
  void failure(mixed value)
  {
    if (state) error("Promise has already been finalized.\n");
    object key = mux->lock();
    if (state) error("Promise has already been finalized.\n");
    unlocked_failure(value);
    key = 0;
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
  void try_failure(mixed value)
  {
    if (state) return;
    object key = mux->lock();
    if (state) return;
    unlocked_failure(value);
  }

  protected void destroy()
  {
    if (!state) {
      unlocked_failure(({ "Promise broken.\n", backtrace() }));
    }
  }
}

protected class FirstCompleted
{
  inherit Promise;

  protected void create(array(Future) futures)
  {
    if (!sizeof(futures)) {
      state = STATE_FULFILLED;
      return;
    }
    futures->on_failure(try_failure);
    futures->on_success(try_success);
  }
}

//! @returns
//! A @[Future] that represents the first
//! of the @expr{futures@} that completes.
//!
//! @seealso
//!   @[race()]
variant Future first_completed(array(Future) futures)
{
  Promise p = FirstCompleted(futures);
  return p->future();
}
variant inline Future first_completed(Future ... futures)
{
  return first_completed(futures);
}

//! JavaScript Promise API equivalent of @[first_completed()].
//!
//! @seealso
//!   @[first_completed()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
variant inline Future race(array(Future) futures)
{
  return first_completed(futures);
}
variant inline Future race(Future ... futures)
{
  return first_completed(futures);
}

protected class Results
{
  inherit Promise;

  protected void create(array(Future) futures)
  {
    if (!sizeof(futures)) {
      success(({}));
      return;
    }

    array(mixed) results = allocate(sizeof(futures), UNDEFINED);
    array(State) states  = allocate(sizeof(futures), STATE_PENDING);

    futures->on_failure(failure);

    foreach(futures; int i; Future f) {
      f->on_success(got_success, i, results, states);
    }
  }

  protected void got_success(mixed value, int i,
			     array(mixed) results, array(State) states)
  {
    if (state || states[i]) return;
    object key = mux->lock();
    if (state || states[i]) return;
    results[i] = value;
    states[i] = STATE_FULFILLED;
    if (has_value(states, STATE_PENDING)) {
      return;
    }
    key = 0;
    success(results);
  }
}

//! @returns
//! A @[Future] that represents the array of all the completed @expr{futures@}.
//!
//! @seealso
//!   @[all()]
variant Future results(array(Future) futures)
{
  Promise p = Results(futures);
  return p->future();
}
inline variant Future results(Future ... futures)
{
  return results(futures);
}

//! JavaScript Promise API equivalent of @[results()].
//!
//! @seealso
//!   @[results()]
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
//! @seealso
//!   @[Future.on_failure()], @[Promise.failure()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
Future reject(mixed reason)
{
  object p = Promise();
  p->failure(reason);
  return p->future();
}

//! @returns
//! A new @[Future] that has already been fulfilled with @expr{value@}
//! as result.  If @expr{value@} is an object which already
//! has @[on_failure] and @[on_success] methods, return it unchanged.
//!
//! @note
//! This function can be used to ensure values are futures.
//!
//! @seealso
//!   @[Future.on_success()], @[Promise.success()]
//!   @url{https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Global_Objects/Promise@}
Future resolve(mixed value)
{
  if (objectp(value) && value->on_failure && value->on_success)
    return value;
  object p = Promise();
  p->success(value);
  return p->future();
}

//! Return a @[Future] that represents the array of mapping @[fun]
//! over the results of the completed @[futures].
Future traverse(array(Future) futures,
		function(mixed, mixed ... : mixed) fun,
		mixed ... extra)
{
  return results(futures->map(fun, @extra));
}

protected class Fold
{
  inherit Promise;

  protected mixed accumulated;

  protected void create(array(Future) futures,
			mixed initial,
			function(mixed, mixed, mixed ... : mixed) fun,
			array(mixed) ctx)
  {
    if (!sizeof(futures)) {
      success(initial);
      return;
    }
    accumulated = initial;
    futures->on_failure(failure);
    foreach(futures; int i; Future f) {
      f->on_success(got_success, i, fun, ctx,
		    allocate(sizeof(futures), STATE_PENDING));
    }
  }

  protected void got_success(mixed val, int i,
			     function(mixed, mixed, mixed ... : mixed) fun,
			     array(mixed) ctx,
			     array(State) states)
  {
    if (state || states[i]) return;
    object key = mux->lock();
    if (state || states[i]) return;
    states[i] = STATE_FULFILLED;
    mixed err = catch {
	// FIXME: What if fun triggers a recursive call?
	accumulated = fun(val, accumulated, @ctx);
	if (has_value(states, STATE_PENDING)) return;
	success(accumulated);
	return;
      };
    failure(err);
  }
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
Future fold(array(Future) futures,
	    mixed initial,
	    function(mixed, mixed, mixed ... : mixed) fun,
	    mixed ... extra)
{
  Promise p = Fold(futures, initial, fun, extra);
  return p->future();
}
