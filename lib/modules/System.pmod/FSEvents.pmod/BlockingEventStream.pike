#pike __REAL_VERSION__
#require constant(System.FSEvents.EventStream)

//! A variation of @[EventStream] that provides a blocking interface.
//!
//! @note
//!   A quirk of the underlying IO subsystem in CoreFoundation is that
//!   there is exactly one runloop per thread. Because FSEvents uses
//!   CoreFoundation, this means that there's no meaningful way to
//!   specify which backend should process these events. Therefore,
//!   always make sure that the thread you create the EventStream
//!   object is the same one you read events from, otherwise
//!   @[read_event] will run not run the EventLoop that this
//!   EventStream is registered with, resulting in events never being
//!   delivered.

  inherit .EventStream;

  protected ADT.Queue received_events = ADT.Queue();

  protected void bfse_callback(string path, int flags, int event_id)
  {
    received_events->write((["path": path, "flags": flags, "event_id": event_id]));
  }

//!
  protected void create(array(string) paths, float latency, int|void since_when, int|void flags)
  {  
#if !constant(Pike.DefaultBackend.HAVE_CORE_FOUNDATION)
    throw(Error.Generic("Pike does not have support for Core Foundation. FSEvents will not function!\n"));
#endif
    ::create(paths, latency, since_when, flags);
    ::set_callback(bfse_callback);
  }

  void set_callback(function(:void) callback)
  {
    throw(Error.Generic("BlockingEventStream does not support set_callback().\n"));
  }

  //! wait for an event to be received, and return it.
  //! 
  //! @param timeout
  //!   an optional limit to the amount of time we're willing to wait
  mixed read_event(void|float timeout)
  {
    int|float res;
    float orig_timeout = timeout;
    if(!is_started())
    {
      Pike.DefaultBackend.enable_core_foundation(1);
      start();
    }

    if(received_events->is_empty())
    {
      do
      {
        res = Pike.DefaultBackend(timeout);
      } while(floatp(res) && received_events->is_empty() && orig_timeout && (timeout = (timeout-res)) > 0.0);
    }    

    if(received_events->is_empty())
      return 0;
    else
      return received_events->get();
  }
