#pike 7.3

//
// Pike 7.2 backward compatibility layer.
//

#pragma no_deprecation_warnings

inherit Thread;

#if constant(thread_create)

class Condition
{
  inherit Thread::Condition;

  // Old-style wait().
  void wait(void|Mutex m)
  {
    if (!m) {
      m = Mutex();
      m->lock();
    }
    ::wait(m);
  }
}

#endif /* !constant(thread_create) */
