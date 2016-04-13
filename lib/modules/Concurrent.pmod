//! Module for handling multiple concurrent events.

/*
 * Inspired by https://github.com/couchdeveloper/FutureLib
 */

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
  void on_success(function(mixed, mixed ... : void) cb, mixed ... extra)
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
  }

  //! Register a callback that is to be called on failure.
  void on_failure(function(mixed, mixed ... : void) cb, mixed ... extra)
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
  //! the fulfiled result of this @[Future], or the result
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
  //! the fulfiled result of this @[Future], or the fulfilled result
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

  //! Fulfill the @[Future] value.
  void success(mixed value)
  {
    object key = mux->lock();
    if (state < STATE_REJECTED) {
      result = value;
      state = STATE_FULFILLED;
      function cb = success_cb;
      array(mixed) extra = success_ctx;
      cond->broadcast();
      key = 0;
      if (cb) {
	call_out(cb, 0, value, @extra);
      }
    }
  }

  //! Reject the @[Future] value.
  void failure(mixed value)
  {
    object key = mux->lock();
    state = STATE_REJECTED;
    result = value;
    function cb = failure_cb;
    array(mixed) extra = failure_ctx;
    cond->broadcast();
    key = 0;
    if (cb) {
      call_out(cb, 0, value, @extra);
    }
  }

  protected void destroy()
  {
    if (!state) {
      failure(({ "Promise broken.\n", backtrace() }));
    }
  }
}
