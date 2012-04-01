//! An X-atom. You most likely want to use GDK1.Atom.atom_name
//! instead of GDK1._Atom(name).
//!
//!

protected GDK1._Atom create( string atom_name, int|void only_if_exists );
//! Create a new low-level atom. You should normally not call this
//! function directly. Use GDK1.Atom[name] instead of GDK1._Atom(name,0).
//!
//!

string get_name( );
//! Returns the name of the atom.
//!
//!
