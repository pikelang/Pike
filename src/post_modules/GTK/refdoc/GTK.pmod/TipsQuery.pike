//!

inherit Label;

static TipsQuery create( );
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

TipsQuery set_caller( GTK.Widget caller_widget );
//!

TipsQuery set_labels( string label_inactive, string label_no_tip );
//!

TipsQuery start_query( );
//!

TipsQuery stop_query( );
//!
