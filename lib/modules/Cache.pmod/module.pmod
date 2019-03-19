#pike __REAL_VERSION__

//! Common Caching implementation
//!
//! This module serves as a front-end to different kinds of caching
//! systems. It uses two helper objects to actually store data, and to
//! determine expiration policies.
//!
//! To create a new cache, do @[Cache.cache]( @[Cache.Storage.Base] storage_type, @[Cache.Policy.Base] expiration_policy )
//!
//!
//! The cache store instances of @[Cache.Data].
