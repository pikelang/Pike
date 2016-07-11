#pike __REAL_VERSION__
#pragma strict_types

//! This is the default implementation of the @[random]
//! functions. This is the @[Random.Interface] combined with a system
//! random source. On Windows this is the default crypto service while
//! on Unix systems it is /dev/urandom.
//!
//! In case of a process fork the generators in the two processes will
//! generate independent random values.

inherit Builtin.RandomSystem;
