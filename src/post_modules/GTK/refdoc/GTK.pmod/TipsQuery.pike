//!

inherit GTK.Label;

static GTK.TipsQuery create( );
//!

GTK.Widget get_caller( );
//!

int get_in_query( );
//!

string get_label_inactive( );
//!

string get_label_no_tip( );
//!

GTK.Widget get_last_crossed( );
//!

GTK.TipsQuery set_caller( GTK.Widget caller_widget );
//!

GTK.TipsQuery set_labels( string label_inactive, string label_no_tip );
//!

GTK.TipsQuery start_query( );
//!

GTK.TipsQuery stop_query( );
//!
