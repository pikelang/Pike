#pike 7.3

//
// $Id: Thread.pmod,v 1.1 2002/09/30 18:45:20 grubba Exp $
//
// Pike 7.2 backward compatibility layer.
//

inherit Thread;

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
    ::wait();
  }
}
