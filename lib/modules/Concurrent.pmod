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

  protected function(mixed:void) success_cb;
  protected array(mixed) success_ctx;
  protected function(mixed:void) failure_cb;
  protected array(mixed) failure_ctx;

  //! Wait for fullfillment and return the value.
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
  this_program on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    object key = mux->lock();
    success_cb = cb;
    success_ctx = extra;
    State s = state;
    mixed res = result;
    key = 0;
    if (s == STATE_FULFILLED) {
      call_out(cb, 0, res, @extra);
    }

    return this;
  }

  //! Register a callback that is to be called on failure.
  this_program on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
  {
    object key = mux->lock();
    failure_cb = cb;
    failure_ctx = extra;
    State s = state;
    mixed res = result;
    key = 0;
    if (s == STATE_REJECTED) {
      call_out(cb, 0, res, @extra);
    }
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

  //! Return a @[Future] that will be fulfilled with an
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

  //! Return a @[Future] that will be failed when @[seconds] seconds has
  //! passed unless it has already been fullfilled.
  this_program timeout(int|float seconds)
  {
    Promise p = Promise();
    on_failure(p->failure);
    on_success(p->success);
    call_out(p->maybe_failure, seconds, ({ "Timeout.\n", backtrace() }));
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
      function cb = success_cb;
      array(mixed) extra = success_ctx;
      cond->broadcast();
      if (cb) {
	call_out(cb, 0, value, @extra);
      }
    }
  }

  //! Fulfill the @[Future] value.
  void success(mixed value)
  {
    object key = mux->lock();
    unlocked_success(value);
    key = 0;
  }

  protected void unlocked_failure(mixed value)
  {
    state = STATE_REJECTED;
    result = value;
    function cb = failure_cb;
    array(mixed) extra = failure_ctx;
    cond->broadcast();
    if (cb) {
      call_out(cb, 0, value, @extra);
    }
  }

  //! Reject the @[Future] value.
  void failure(mixed value)
  {
    object key = mux->lock();
    unlocked_failure(value);
    key = 0;
  }

  //! Reject the @[Future] value unless it has already been fulfilled.
  void maybe_failure(mixed value)
  {
    object key = mux->lock();
    if (!state) return;
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
    futures->on_failure(got_failure);
    futures->on_success(got_success);
  }

  protected void got_failure(mixed err)
  {
    if (state) return;
    failure(err);
  }

  protected void got_success(mixed val)
  {
    if (state) return;
    success(val);
  }
}

//! Return a @[Future] that represents the first
//! of the @[futures] that completes.
Future first_completed(array(Future) futures)
{
  Promise p = FirstCompleted(futures);
  return p->future();
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
    if (state == STATE_REJECTED) return;
    results[i] = value;
    states[i] = STATE_FULFILLED;
    if (!state) {
      if (has_value(states, STATE_PENDING)) {
        return;
      }
    }
    success(results);
  }
}

//! Return a @[Future] that represents the array of all
//! the completed @[futures].
Future results(array(Future) futures)
{
  Promise p = Results(futures);
  return p->future();
}

//! Return a @[Future] that represents the array of mapping @[fun]
//! over the results of the competed @[futures].
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
    states[i] = STATE_FULFILLED;
    mixed err = catch {
	// FIXME: What if fun triggers a recursive call?
	accumulated = fun(val, accumulated, @ctx);
	if (!state) {
	  if (has_value(states, STATE_PENDING)) return;
	}
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
//!   @[fun] may be called in any order, and will only be called
//!   once for every @[Future] in @[futures].
Future fold(array(Future) futures,
	    mixed initial,
	    function(mixed, mixed, mixed ... : mixed) fun,
	    mixed ... extra)
{
  Promise p = Fold(futures, initial, fun, extra);
  return p->future();
}
