#pike __REAL_VERSION__

constant inherit_list = __builtin.inherit_list;
constant inherits = __builtin.program_inherits;
constant implements = __builtin.program_implements;
constant defined = __builtin.program_defined;

//! @fixme
//!   Document this function.
array(program) all_inherits(program p)
{
  array(program) ret = inherit_list(p);
  for(int e=0;e<sizeof(ret);e++) ret+=inherit_list(ret[e]);
  return ret;
}

//! Recursively builds a inheritance tree by
//! fetching programs inheritance lists.
//!
//! @returns
//!  Returns an array with programs or arrays
//!  as elements.
//!
//! @example
//!  > class a{}
//!  > class b{}
//!  > class c{ inherit a; }
//!  > class d{ inherit b; inherit c; }
//!  > Program.inherit_tree(d);
//!  Result: ({ /* 3 elements */
//!              d,
//!              ({ /* 1 element */
//!                  program
//!              }),
//!              ({ /* 2 elements */
//!                  c,
//!                  ({ /* 1 element */
//!                      program
//!                  })
//!              })
//!          })
array inherit_tree(program p)
{
  return ({ p })+
    Array.map(inherit_list(p),inherit_tree);
}


