/*
 * name = "BG: diagram.h";
 * doc = "Business Graphics common things. You must upgrade this component to use newer versions of BG.";
 *
 * string cvs_version="$Id: graph.h,v 1.1 1999/09/30 13:04:11 hedda Exp $";
 */


#define max(i, j) (((i)>(j)) ? (i) : (j))
#define min(i, j) (((i)<(j)) ? (i) : (j))
// #define abs(arg) ((arg)*(1-2*((arg)<0)))   // Don't do it this way! /grubba
#define abs(arg) (((arg)<0) ? -(arg) : (arg))

#define PI 3.14159265358979
#define VOIDSYMBOL "\n"
#define SEP "\t"
#define UNICODE(TEXT,ENCODING) Locale.Charset.decoder(ENCODING)->feed(TEXT)->drain()

private constant LITET = 1.0e-38;
private constant STORTLITET = 1.0e-30;
private constant STORT = 1.0e30;


//This is used in Roxen. Don't work with only Pike
//#define GETFONT(WHATFONT) object notext=resolve_font(diagram_data->WHATFONT||diagram_data->font);
#define GETFONT(WHATFONT) object notext=diagram_data->font;

//#define BG_DEBUG 1
#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

