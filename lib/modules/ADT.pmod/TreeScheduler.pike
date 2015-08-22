// -*- encoding: utf-8; -*-

#pike __REAL_VERSION__

inherit .Scheduler;

//! This class implements an hierarchial quantized resource scheduler.
//!
//! It differs from @[Scheduler] by the [Consumer]s making
//! up a dependency tree.
//!
//! Active consumers closer to the root will receive the resource
//! before their children.
//!
//! Implements most of @rfc{7540:5.3@}.
//!
//! @seealso
//!   @[Scheduler]

//! A resource consumer.
//!
//! All consumers (both active and inactive) are nodes in
//! a dependency tree. This means that to avoid excessive
//! garbage @[detach()] must be called in consumers that
//! are no longer to be used.
//!
//! Active consumers are kept in a (min-)@[Heap].
class Consumer
{
  inherit ::this_program;

  //! @[Consumer] that we depend on.
  Consumer parent;

  //! @[Consumer]s that depend on us.
  array(Consumer) children = ({});

  // Total weight of all direct children.
  float children_weight = 0.0;

  // Number of parents to the root.
  // This is used to ensure that active parents get the resource before
  // their children.
  int(0..) depth;

  //! Update the cached quanta value.
  //!
  //! This function should be called whenever our @[weight]
  //! or that of our siblings has changed.
  void update_quanta()
  {
    if (!parent) {
      quanta = 1.0;
      return;
    }
    float old_quanta = quanta;
    // The quanta is the inverted normalized weight.
    quanta = parent->children_weight * parent->quanta / weight;
    children->update_quanta();
    pri += (quanta - old_quanta)/2.0;
    adjust();
  }

  void `weight=(float|int new_weight)
  {
    if (new_weight == weight_) return;
    parent->children_weight += new_weight - weight_;
    weight_ = new_weight;
    parent->children->update_quanta();
  }

  float|int `weight()
  {
    return weight_;
  }

  //!
  protected void create(int|float weight, mixed v, Consumer|void parent)
  {
    weight_ = weight;
    value = v;
    parent = this_program::parent = parent || root;
    if (parent) {
      parent->children += ({ this });
      parent->children_weight += weight;
      depth = parent->depth + 1;
      pri = parent->pri - parent->quanta/2.0;
      offset = parent->offset;
      // NB: The following also adjusts pri with our new quanta.
      parent->children->update_quanta();
    } else {
      // Root.
      quanta = 1.0;
      pri = 0.5;
    }
  }

  void set_depth(int new_depth)
  {
    if (depth == new_depth) return;
    depth = new_depth;
    foreach(children, Consumer c) {
      c->set_depth(depth + 1);
    }
    adjust();
  }

  void consume_down(float delta)
  {
    // NB: Update the children first to avoid unneeded reorderings.
    foreach(children, Consumer c) {
      c->consume_down(delta * c->weight / children_weight);
    }
    ::consume(delta);
  }

  void consume_up(float delta)
  {
    ::consume(delta);
    if (parent) {
      parent->consume_up(delta);
    }
  }

  void consume(float delta)
  {
    consume_down(delta);
    if (parent) {
      // NB: For us to have consumed all of our parents
      //     must have been inactive. Update their weights.
      parent->consume_up(delta);
    }
  }

  //! Change to a new parent.
  //!
  //! @param new_parent
  //!   @[Consumer] this object depends on. We will only
  //!   get returned by @[get()] when @[new_parent] is
  //!   inactive (ie @[remove]d).
  //!
  //! @param weight
  //!   New weight.
  //!
  //! @note
  //!   If @[new_parent] depends on us, it will be moved
  //!   to take our place in depending on our old parent.
  //!
  //! @note
  //!   To perform the exclusive mode reparent from @rfc{7540@}
  //!   figure 5, call @[reparent_siblings()] after this function.
  //!
  //! @seealso
  //!   @[detach()], @[remove()], @[create()], @[reparent_siblings()]
  void set_parent(Consumer new_parent, int|float weight)
  {
    if (!new_parent) new_parent = root;

    if (new_parent == parent) {
      this_program::weight = weight;
      return;
    }

    // If the new parent is a child to us, we
    // need to reparent it to our old parent.
    Consumer npp = new_parent;
    while (npp) {
      if (npp == this) {
	// The new parent is a child to us, so reparent
	// it to take our old place.
	new_parent->set_parent(parent, weight_);
	break;
      }
      npp = npp->parent;
    }

    // Detach from our old parent.
    parent->children_weight -= weight_;
    parent->children -= ({ this });
    parent->children->update_quanta();

    // Attach to the new parent.
    parent = new_parent;
    weight_ = weight;
    parent->children += ({ this });
    parent->children_weight += weight;

    // Update the depth.
    set_depth(parent->depth + 1);

    parent->children->update_quanta();

    // Check if we need to adjust our priority.
    // We should always have a priority value that
    // is larger or equal to that of our parent.
    float parent_pri = parent->pri + parent->offset - offset;
    if (parent_pri > pri) {
      // We need to adjust our priority.
      // Pretend to have consumed the difference.
      consume_down((parent_pri - pri) / quanta);
    }
  }

  //! Reparent all sibling @[Consumer]s, so that we become
  //! the only child of our parent.
  //!
  //! @seealso
  //!   @[set_parent()]
  void reparent_siblings()
  {
    // First the trivial case.
    if (sizeof(parent->children) == 1) return;

    float weight_factor = children_weight/weight;
    foreach(parent->children, Consumer c) {
      if (c == this) continue;
      c->set_parent(this, c->weight * weight_factor);
    }
  }

  //! Detach from the tree.
  //!
  //! Any children are moved to our parent and their
  //! weights adjusted to keep their priorities.
  //!
  //! @note
  //!   If the consumer was active it will be deactivated.
  void detach()
  {
    if (state & STATE_ACTIVE) {
      remove(this);
    }
    Consumer parent = this_program::parent;
    this_program::parent = UNDEFINED;

    parent->children -= ({ this });

    if (sizeof(children)) {
      // NB: This operation is priority and quanta-invariant, so normally
      //     the heap will not change. The only reason for it to change
      //     is due to depth changes for nodes with the same priority.
      parent->children += children;
      foreach(children, Consumer c) {
	c->parent = parent;
	c->weight *= weight/children_weight;
	parent->children_weight += c->weight;
      }
      children->set_depth(parent->depth + 1);
      children = ({});
    }
    parent->children_weight -= weight;
    parent->children->update_quanta();
  }

  protected void destroy()
  {
    if (parent) {
      detach();
    } else {
      // Already detached, or we are the root.
      foreach(children, Consumer c) {
	c->parent = UNDEFINED;
	c->set_depth(depth);
      }
    }
  }

  protected int `<(object o)
  {
    if (pri == o->pri) {
      // Make sure that children are considered after their parents.
      return depth < o->depth;
    }
    return pri<o->pri;
  }
  protected int `>(object o)
  {
    if (pri == o->pri) {
      // Make sure that children are considered after their parents.
      return depth > o->depth;
    }
    return pri>o->pri;
  }
}

//! Create a @[Consumer] depending on @[parent] with the weight @[weight]
//! for the value @[val], and add it to the Scheduler.
variant Consumer add(int|float weight, mixed val, Consumer parent)
{
  return add(Consumer(weight, val, parent));
}

//! The root of the @[Customer] dependency tree.
//!
//! @note
//!   Note that the root is never active (ie added to the Scheduler).
//!
//! @[Customer]s that don't have an explicit dependency depend on @[root].
Consumer root = Consumer(1.0, "root");

protected string _sprintf(int c)
{
  if (c != 'O') return UNDEFINED;
  Stdio.Buffer buf = Stdio.Buffer("ADT.TreeScheduler(\n");
  .Stack todo = .Stack();
  todo->push(0);	// End sentinel.
  todo->push(root);
  Consumer child;
  while (child = todo->pop()) {
    buf->sprintf("  %*s%O\n", child->depth * 2, "", child);
    foreach(reverse(child->children), Consumer cc) {
      todo->push(cc);
    }
  }
  buf->add(")");
  return (string)buf;
}

protected void destroy()
{
  // Zap the circular references in the dependency tree.
  .Stack todo = .Stack();
  todo->push(0);	// End sentinel.
  todo->push(root);
  root = UNDEFINED;
  Consumer child;
  while (child = todo->pop()) {
    foreach(child->children, Consumer cc) {
      todo->push(cc);
    }
    child->children = ({});
    child->parent = UNDEFINED;
    destruct(child);
  }
}
