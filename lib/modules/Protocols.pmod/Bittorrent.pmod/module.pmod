// KLUDGE!!!1!¡
// this is to bootstrap; Protocols.Bittorrent.Port doesn't survive
// being loaded first:

// Torrent.pike:406:Too many arguments to clone call.
// Torrent.pike:406:Expected: function( : object(is 337450))
// Torrent.pike:406:Got     : function(object(implements 337451) : void | mixed)
// Torrent.pike:553:Must return a value for a non-void function.
// Torrent.pike:687:Must return a value for a non-void function.
// Torrent.pike:901:Must return a value for a non-void function.
// Torrent.pike:939:Must return a value for a non-void function.
// Torrent.pike:947:Must return a value for a non-void function.
// Torrent.pike:950:Must return a value for a non-void function.
// Torrent.pike:1018:Must return a value for a non-void function.
// Torrent.pike:1093:Class definition failed.
// Torrent.pike:1103:Must return a value for a non-void function.
// Torrent.pike:1156:Must return a value for a non-void function.
// Port.pike:6:Error looking up 'Torrent' in module '.'.
// Port.pike:27:Must return a value for a non-void function.
// test: failed to peek at "Protocols.Bittorrent.Port": Compilation failed.

#pike __REAL_VERSION__
#if constant(.Torrent)
static private function dummy=.Torrent;
#else /* !constant(.Torrent) */

constant this_program_does_not_exist=1;

#endif /* constant(.Torrent) */
