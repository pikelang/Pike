/*
 * $Id$
 */

#define PI 3.14159265358979
#define VOIDSYMBOL "\n"
#define SEP "\t"
#define UNICODE(TEXT,ENCODING) Locale.Charset.decoder(ENCODING)->feed(TEXT)->drain()

private constant LITET = 1.0e-38;
private constant STORTLITET = 1.0e-30;
private constant STORT = 1.0e30;

#define GETFONT(WHATFONT) diagram_data->font;

//#define BG_DEBUG 1


