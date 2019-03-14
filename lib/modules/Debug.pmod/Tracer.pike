#pike __REAL_VERSION__

//! A class that when instatiated will turn on trace, and when it's
//! destroyed will turn it off again.

void destroy()         { trace(0); }

//! Sets the level of debug trace to @[level].
void create(int level) { trace(level); }
