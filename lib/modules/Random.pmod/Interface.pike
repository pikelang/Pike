#pike __REAL_VERSION__
#pragma strict_types

//! This class implements the Pike @[predef::random] API on top of a
//! byte stream random source. This source should be implemented in
//! the @[random_string] method and will be called by random(int) for
//! random input. random(int) is in turned called by all the other
//! variants of random and applied to their use cases.
//!
//! While it is possible to overload the random variants, care must be
//! taken to not introduce any bias. The default implementation
//! gathers enough bits to completely reach the limit value, and
//! discards them if the result overshoots the limit.

//! @decl string(8bit) random_string(int length)
//! Prototype for the random source stream function. Must return
//! exactly @[length] bytes of random data.

//! @decl int(0..) random(int(0..) max)
//! @decl float random(float max)
//! @decl mixed random(array|multiset x)
//! @decl array random(mapping m)
//! @decl mixed random(object o)
//! Implementation of @[predef::random].

inherit Builtin.RandomInterface;
