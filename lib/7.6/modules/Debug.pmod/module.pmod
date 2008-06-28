/*
 * $Id: module.pmod,v 1.2 2008/06/28 16:52:49 nilsson Exp $
 *
 * Debug API changes since Pike 7.6.
 *
 * Henrik Grubbström 2005-01-08
 */

#pike 7.6

//! @decl array(array(int|string)) describe_program(program p)
//! Debug function for showing the symbol table of a program.

constant describe_program = predef::_describe_program;
