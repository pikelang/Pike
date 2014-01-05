#pike __REAL_VERSION__

inherit Web.Crawler.Queue;

//! Virtual base class for the @[Search] crawler state.

//! The queue stage levels.
enum Stage {
  STAGE_WAITING = 0,
  STAGE_FETCHING,
  STAGE_FETCHED,
  STAGE_FILTERED,
  STAGE_INDEXED,
  STAGE_COMPLETED,
  STAGE_ERROR,
};

//! Add an URI to be crawled.
void add_uri( Standards.URI uri, int recurse, string template, void|int force );

//! Set the content MD5 for an URI.
void set_md5( Standards.URI uri, string md5 );

//! Set the recurse mode for an URI.
void set_recurse( Standards.URI uri, int recurse );

//! @returns
//!   Returns a mapping with the current state for the URI.
//!   @mapping
//!     @member string "md5"
//!     @member string|int "recurse"
//!     @member string|int "stage"
//!     @member string "template"
//!   @endmapping
//!
//! @fixme
//!   Currently this function always returns a
//!   @expr{mapping(string:string)@}, but this
//!   may change to the above in the future.
mapping get_extra( Standards.URI uri );

//!
int|Standards.URI get();

//! @returns
//!   Returns all URIs if no @[stage] is specified,
//!   otherwise returns the URIs at the specified @[stage].
//!
//! @fixme
//!   State @expr{0@} (zero) is a special case,
//!   and returns all URIs. This may change in
//!   the future.
array(Standards.URI) get_uris(void|int stage);

//! Add one or multiple URIs to the queue.
//!
//! All the URIs will be added with recursion enabled
//! and an empty template.
void put(string|array(string)|Standards.URI|array(Standards.URI) uri);

//! Clear and empty the entire queue.
void clear();

//! Reset all URIs at the specified stage to stage @expr{0@} (zero).
void clear_stage( int ... stages );

//! Remove all URIs at the specified stage.
void remove_stage (int stage);

//! Clear the content MD5 for all URIs at the specified stages.
void clear_md5( int ... stages );

//! @returns
//!   Returns the number of URIs at the specified stage(s).
int num_with_stage( int ... stage );

//! Set the stage for a single URI.
void set_stage( Standards.URI uri, int stage );

//! Remove an URI from the queue.
void remove_uri(string|Standards.URI uri);

//! Remove all URIs with the specified prefix from the queue.
void remove_uri_prefix(string|Standards.URI uri);

//! Clear any RAM caches.
void clear_cache();
