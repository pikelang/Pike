#pike 7.3

//
// $Id: Thread.pmod,v 1.2 2002/12/03 21:16:35 nilsson Exp $
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
    ::wait(m);
  }
}
