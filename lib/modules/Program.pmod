#pike __REAL_VERSION__
#pragma strict_types

constant inherit_list = __builtin.inherit_list;
constant inherits = __builtin.program_inherits;
constant implements = __builtin.program_implements;
constant annotations = __builtin.direct_program_annotations;

// documented in the C-code.
string|zero defined(program x,string|void y)
{
    if( !y )
        return __builtin.program_defined(x);
    return __builtin.program_identifier_defined(x, [string]y);
}


//! Enumerate all programs this program inherits, directly or indirectly.
//! Similar to inherit_tree() but returns a flat array.
//!
//! @example
//!  > class a{}
//!  > class b{}
//!  > class c{ inherit a; }
//!  > class d{ inherit b; inherit c; }
//!  > Program.inherit_tree(d);
//!  Result: ({ /* 3 elements */
//!              b,
//!              c,
//!              a
//!          })
array(program) all_inherits(program p)
{
  array(program) ret = inherit_list(p);
  // Iterate over a mutated array to catch all inherits to infinite depth
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
