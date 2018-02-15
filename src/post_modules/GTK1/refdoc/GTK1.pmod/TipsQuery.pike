//!

inherit GTK1.Label;

protected GTK1.TipsQuery create( );
//!

GTK1.Widget get_caller( );
//!

int get_in_query( );
//!

string get_label_inactive( );
//!

string get_label_no_tip( );
//!

GTK1.Widget get_last_crossed( );
//!

GTK1.TipsQuery set_caller( GTK1.Widget caller_widget );
//!

GTK1.TipsQuery set_labels( string label_inactive, string label_no_tip );
//!

GTK1.TipsQuery start_query( );
//!

GTK1.TipsQuery stop_query( );
//!
