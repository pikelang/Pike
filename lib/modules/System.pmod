#pike __REAL_VERSION__
#pragma strict_types

// $Id$
// This module is to allow the _system module to be called System.

inherit _system;

constant Timer = __builtin.Timer;
constant Time  = __builtin.Time;
