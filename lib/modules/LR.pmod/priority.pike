/*
 * $Id: priority.pike,v 1.1 1997/03/03 23:50:19 grubba Exp $
 *
 * Rule priority specification
 *
 * Henrik Grubbström 1996-12-05
 */

/* Priority value */
int value;

/* Associativity
 *
 * -1 - left
 *  0 - none
 * +1 - right
 */
int assoc;

void create(int p, int a)
{
  value = p;
  assoc = a;
}
