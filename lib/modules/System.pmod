#pike __REAL_VERSION__

// $Id$
// this module is to allow the system module to be called system.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;
