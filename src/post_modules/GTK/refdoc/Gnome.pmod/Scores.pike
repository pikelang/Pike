//! This is a high-scores dialog box. The GNOME libraries also handle
//! loading/saving systemwide high scores in a secure way.
//! 
//!@code{ Gnome.Scores( 17, ({ "per" })*17, map((array(float))indices(allocate(17)),`*,42), map(indices(allocate(17)), `*, 10 ), 1 )@}
//!@xml{<image src='../images/gnome_scores.png'/>@}
//!
//! 
//!
//!
inherit GnomeDialog;

static GnomeScores create( int n_scores, array(string) names, array(float) scores, array(int) times, int clear )
//!

GnomeScores set_color( int pos, GDK.Color color )
//!

GnomeScores set_current_player( int index )
//!

GnomeScores set_def_color( GDK.Color color )
//!

GnomeScores set_logo_label_title( string txt )
//!

GnomeScores set_logo_pixmap( string logofile )
//!

GnomeScores set_logo_widget( GTK.Widget widget )
//!
