/*
 * $Id: priority.pike,v 1.2 1997/03/30 17:28:27 grubba Exp $
 *
 * Rule priority specification
 *
 * Henrik Grubbström 1996-12-05
 */

//.
//. File:	priority.pike
//. RCSID:	$Id: priority.pike,v 1.2 1997/03/30 17:28:27 grubba Exp $
//. Author:	Henrik Grubbström (grubba@infovav.se)
//.
//. Synopsis:	Rule priority specification.
//.
//. +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//.
//. Specifies the priority and associativity of a rule.
//.

//. + value
//.   Priority value
int value;

//. + assoc
//.   Associativity
//.
//.   -1 - left
//.    0 - none
//.   +1 - right
int assoc;

//. - create
//.   Create a new priority object.
//. > p
//.   Priority.
//. > a
//.   Associativity.
void create(int p, int a)
{
  value = p;
  assoc = a;
}
