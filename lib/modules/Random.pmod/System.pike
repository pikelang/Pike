#pike __REAL_VERSION__
#pragma strict_types

//! This is the default implementation of the @[random]
//! functions. This is the @[Random.Interface] combined with a system
//! random source. On Windows this is the default crypto service while
//! on Unix systems it is /dev/urandom.

inherit Builtin.RandomSystem;
