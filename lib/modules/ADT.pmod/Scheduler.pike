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
  for (i = 0; i < Heap::num_values; i++) {
    Heap::values[i]->pri -= 2.0;
  }
  normalization_offset += 2;
}

protected enum ConsumerState
{
  STATE_ACTIVE		= 1,
}

//! A resource consumer.
//!
//! Active consumers are kept in a (min-)@[Heap].
class Consumer {
  mixed value;
  //! The value set in @[create()].

  protected int|float weight_;

  //
  // We distribute quotas according to the Sainte-laguë method,
  // using an inverted and scaled quotient, and with fractional
  // actual quotas.
  //

  float pri;
  //! Accumulated deltas and initial priority.
  //!
  //! Typically in the range @expr{0.0 .. 2.0@}, but may temporarily
  //! be outside of the range.

  int offset;

  ConsumerState state;

  //!
  protected void create(int|float weight, mixed v)
  {
    weight_ = weight;
    value = v;

    // Initialize to half a quanta.
    pri = 1.0/(2.0 * weight);
    if (Heap::_sizeof()) {
      // Adjust the priority as if we've been active from the beginning.
      // Otherwise we'll get an unfair amount of the resource until
      // we reach this point. The element on the top of the heap is
      // representative of the accumulated consumption so far.
      Consumer c = Heap::peek();
      pri += c->pri - 1.0/(2.0 * c->weight);
    } else if (normalization_offset) {
      pri -= (float)normalization_offset;
    }
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
    float old_pri = pri;
    pri += delta / weight_;
    adjust();
    if (pri > 2.0) {
      renormalize_priorities();
    }
  }

  //! The weight of the consumer.
  void `weight=(int|float weight)
  {
    int|float old = weight_;
    weight_ = weight;
    pri += 1.0/(2.0 * weight_) - 1.0/(2.0 * old);
    adjust();
  }

  //! Get the weight of the consumer.
  int|float `weight() { return weight_; }

  int `<(object o) { return pri<o->pri; }
  int `>(object o) { return pri>o->pri; }

  protected string _sprintf(int c)
  {
    return sprintf("Consumer(%O [pri: %O], %O)",
		   weight_, pri, value);
  }
}

//! (Re-)activate a @[Consumer].
Consumer add(Consumer c)
{
  if (c->state & STATE_ACTIVE) {
    error("Attempt to add the same consumer twice.\n");
  }
  c->pri -= (float)(normalization_offset - c->offset);
  c->state |= STATE_ACTIVE;
  Heap::push(c);
  return c;
}

//! Push an element @[val] into the priority queue and assign a priority value
//! @[pri] to it. The priority queue will automatically sort itself so that
//! the element with the highest priority will be at the top.
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
    int pos = search(Heap::values, c);
    if (pos < 0) error("Active consumer not found in heap.\n");
    Heap::num_values--;
    if (Heap::num_values > pos) {
      Heap::values[pos] = Heap::values[Heap::num_values];
      Heap::adjust_down(pos);
    }
    Heap::values[Heap::num_values] = 0;

    if(Heap::num_values * 3 + 10 < sizeof(Heap::values))
      Heap::values = Heap::values[..num_values+10];

    c->offset = normalization_offset;
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
Consumer get() { return Heap::peek(); }
