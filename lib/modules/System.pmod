#pike __REAL_VERSION__
#pragma strict_types

// $Id: System.pmod,v 1.8 2003/04/27 02:22:26 nilsson Exp $
// This module is to allow the _system module to be called System.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;
