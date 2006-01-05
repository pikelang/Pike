//! An X-atom. You most likely want to use GDK2.Atom.atom_name
//! instead of GDK2._Atom(name).
//!
//!

static GDK2._Atom create( string atom_name, int|void only_if_exists );
//! Create a new low-level atom. You should normally not call this
//! function directly. Use GDK2.Atom[name] instead of GDK2._Atom(name,0).
//!
//!

string get_name( );
//! Returns the name of the atom.
//!
//!
