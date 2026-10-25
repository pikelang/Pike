#pike 9.0

//! Pike 8.0 compatibility for Concurrent aggregation diagnostics.

inherit Concurrent : pre;

//! Compatibility name for the state used by aggregate futures.
//!
//! In Pike 9.0 aggregation state is part of @[pre::AggregatedPromise].
//! This class retains the extension point for Pike 8.0 applications that
//! inherit Concurrent and override AggregateState for diagnostics.
//! It does not restore the old implementation's internal fields.
class AggregateState
{
  inherit pre::AggregatedPromise;
}

protected pre::AggregatedPromise aggregate_promise_factory()
{
  return AggregateState();
}
