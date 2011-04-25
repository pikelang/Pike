/*
 * $Id$
 */

#define PI 3.14159265358979
#define VOIDSYMBOL "\n"
#define SEP "\t"
#define UNICODE(TEXT,ENCODING) Locale.Charset.decoder(ENCODING)->feed(TEXT)->drain()

#define GETFONT(WHATFONT) ((diagram_data->WHATFONT) || diagram_data->font)

//#define BG_DEBUG 1


