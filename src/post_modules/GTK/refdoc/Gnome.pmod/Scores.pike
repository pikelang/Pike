//! This is a high-scores dialog box. The GNOME libraries also handle
//! loading/saving systemwide high scores in a secure way.
//! 
//!@code{ Gnome.Scores( 17, ({ "per" })*17, map((array(float))indices(allocate(17)),`*,42), map(indices(allocate(17)), `*, 10 ), 1 )@}
//!@xml{<image>../images/gnome_scores.png</image>@}
//!
//! 
//!
//!

inherit Gnome.Dialog;

static Gnome.Scores create( int n_scores, array names, array scores, array times, int clear );
//!

Gnome.Scores set_color( int pos, GDK.Color color );
//!

Gnome.Scores set_current_player( int index );
//!

Gnome.Scores set_def_color( GDK.Color color );
//!

Gnome.Scores set_logo_label_title( string txt );
//!

Gnome.Scores set_logo_pixmap( string logofile );
//!

Gnome.Scores set_logo_widget( GTK.Widget widget );
//!
