#pike 8.1

inherit Thread : pre;

//! Create a new thread.
//!
//! @deprecated predef::Thread.Thread
optional Thread `()( mixed f, mixed ... args )
{
  return thread_create( f, @args );
}
