/*
 * $Id: Priority.pike,v 1.1 2002/05/09 17:51:04 nilsson Exp $
 *
 * Rule priority specification
 *
 * Henrik Grubbström 1996-12-05
 */

#pike __REAL_VERSION__

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
