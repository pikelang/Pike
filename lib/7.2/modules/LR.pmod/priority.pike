/*
 * $Id: priority.pike,v 1.2 2002/12/04 14:00:12 grubba Exp $
 *
 * Rule priority specification
 *
 * Henrik Grubbström 1996-12-05
 */

#pike 7.2

//!
//! Specifies the priority and associativity of a rule.
//!

//!   Priority value
int value;

//! Associativity
//!
//! @int
//!   @value -1
//!    Left
//!   @value 0
//!    None
//!   @value 1
//!    Right
//! @endint
int assoc;

//! Create a new priority object.
//!
//! @param p
//!  Priority.
//! @param  a
//!  Associativity.
void create(int p, int a)
{
  value = p;
  assoc = a;
}
