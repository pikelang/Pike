//! Module for handling multiple concurrent events.

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
}

//! Promise to provide a @[Future] value.
//!
//! @seealso
//!   @[Future]
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
