#pike __REAL_VERSION__

// $Id: System.pmod,v 1.7 2002/11/26 14:51:09 grubba Exp $
// this module is to allow the system module to be called system.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;
