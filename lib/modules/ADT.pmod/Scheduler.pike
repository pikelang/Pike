// -*- encoding: utf-8; -*-

#pike __REAL_VERSION__

protected inherit .Heap;

//! This class implements a quantized resource scheduler.
//!
//! Weighted consumers are added to the scheduler with @[add()],
//! which returns a @[Consumer] object.
//!
//! When there's some of the resource available to be consumed
//! the resource owner calls @[get()], which returns the
//! @[Consumer] that is to use the resource. @[Consumer()->consume()]
//! is then called with the fraction of the quanta that was consumed
//! (typically @expr{1.0@}). The amount of resources allocated to a
//! consumer is proportional to the weight of the consumer.
//!
//! A consumer may be temporarily deactivated (in which case it won't
//! be returned by @[get()], but still retain its share of the resource
//! which will be provided later by @[get()] when it has been reactivated. 

// This is an offset from the "true" priority value that is used to
// keep the priority values small enough to save on the mantissa.
protected int normalization_offset = 0;

protected void renormalize_priorities()
{
  int i;
  normalization_offset += 256;
  for (i = 0; i < Heap::num_values; i++) {
    Heap::values[i]->pri -= 256.0;
    Heap::values[i]->offset = normalization_offset;
  }
}

protected enum ConsumerState
{
  STATE_ACTIVE		= 1,
}

//! A resource consumer.
//!
//! Active consumers are kept in a (min-)@[Heap].
class Consumer {
  inherit Element;

  protected int|float weight_;

  //
  // We distribute quotas according to the Sainte-laguë method,
  // using an inverted and scaled quotient, and with fractional
  // actual quotas.
  //

  float pri = 0.0;
  //! Accumulated deltas and initial priority.
  //!
  //! Typically in the range @expr{0.0 .. 2.0@}, but may temporarily
  //! be outside of the range.

  int offset;

  ConsumerState state;

  // NB: Negative and zero quanta indicate that it should be recalculated.
  float quanta = 0.0;

  //!
  protected void create(int|float weight, mixed v)
  {
    weight_ = weight;
    value = v;
    quanta = 1.0/weight;

    // Initialize to half a quanta.
    pri = quanta/2.0;
    if (Heap::_sizeof()) {
      // Adjust the priority as if we've been active from the beginning.
      // Otherwise we'll get an unfair amount of the resource until
      // we reach this point. The element on the top of the heap is
      // representative of the accumulated consumption so far.
      Consumer c = Heap::peek();
      pri += c->pri - c->quanta/2.0;
    } else if (normalization_offset) {
      pri -= (float)normalization_offset;
    }

    offset = normalization_offset;
  }

  protected void adjust()
  {
    if (!(state & STATE_ACTIVE)) return;
    Heap::adjust(this);
  }

  //! Consume some of the resource.
  //!
  //! @param delta
  //!   Share of the resource quanta that was actually consumed.
  //!   Typically @expr{1.0@}, but other values are supported.
  //!
  //! This causes the consumer to be reprioritized.
  void consume(float delta)
  {
    pri += delta * quanta;
    adjust();
    if (pri > 256.0) {
      renormalize_priorities();
    }
  }

  //! The weight of the consumer.
  void `weight=(int|float weight)
  {
    int|float old_quanta = quanta;
    weight_ = weight;
    quanta = 1.0/weight;
    pri += (quanta - old_quanta)/2.0;
    adjust();
  }

  //! Get the weight of the consumer.
  int|float `weight() { return weight_; }

  protected int `<(object o) { return pri<o->pri; }
  protected int `>(object o) { return pri>o->pri; }

  protected string _sprintf(int c)
  {
    return sprintf("Consumer(%O [pri: %O, q: %O, s:%s], %O)",
		   weight_, pri, quanta, (state & STATE_ACTIVE)?"A":"", value);
  }
}

//! (Re-)activate a @[Consumer].
Consumer add(Consumer c)
{
  if (c->state & STATE_ACTIVE) {
    return c;
  }
  c->pri -= (float)(normalization_offset - c->offset);
  c->offset = normalization_offset;
  c->state |= STATE_ACTIVE;
  Heap::push(c);
  return c;
}

//! Create a @[Consumer] with the weight @[weight] for the value @[val],
//! and add it to the Scheduler.
variant Consumer add(int|float weight, mixed val)
{
  return add(Consumer(weight, val));
}

//! Adjust the weight value @[new_weight] of the @[Consumer] @[c] in the
//! scheduling table.
void adjust_weight(Consumer c, int new_weight)
{
  c->set_weight(new_weight);
}

//! Remove the @[Consumer] @[c] from the set of active consumers.
//!
//! The consumer may be reactivated by calling @[add()].
void remove(Consumer c)
{
  if (c->state & STATE_ACTIVE) {
    Heap::remove(c);
    c->state &= ~STATE_ACTIVE;
  }
}

//! Returns the next @[Consumer] to consume some of the resource.
//!
//! @returns
//!   Returns a @[Consumer] if there are any active @[Consumers]
//!   and @[UNDEFINED] otherwise.
//!
//! @note
//!   The same @[Consumer] will be returned until it has either
//!   consumed some of the resource, been removed or another
//!   @[Consumer] with lower priority has been added.
Consumer get()
{
  // FIXME: We know about internals in ADT.Heap.
  if (!num_values)
    return UNDEFINED;
  return values[0];
}
