//!
//! module Geographical
//! class Position
//!	This class contains a geographical position,
//!	ie a point on the earths surface.
//!

//! variable float lat
//! variable float long
//!	Longitude (W--E) and latitude (N--S) of the position,
//!	float value in degrees.
//!	Positive number is north and east, respectively. 
//!	Negative number is south and west, respectively.


float lat;  // latitude in degrees  (N--S)
float long; // longitude in degrees (W--E)

//! method void create(float lat,float long)
//! method void create(string lat,string long)
//! method void create(string both)
//!	Constructor for this class.
//!	If feeded with strings, it will perform
//!	a dwim scan on the strings. If they
//!	fails to be understood, there will be an exception.

void create(int|float|string _lat,void|int|float|string _long)
{
   if (stringp(_lat))
   {
      if (zero_type(_long))
      {
	 string tmp;
	 if (sscanf(_lat,"%sN %s",tmp,_long)==2) _long=tmp+"N";
	 else if (sscanf(_lat,"%sS %s",tmp,_long)==2) _long=tmp+"N";
	 else if (sscanf(_lat,"%s %s",tmp,_long)==2) _long=tmp;
      }
      _lat=dwim(_lat,"NS");
      if (stringp(_lat))
	 error("Failed to understand latitude %O\n",lat);
   }
   if (stringp(_long))
   {
      _long=dwim(_long,"EW");
      if (stringp(_long))
	 error("Failed to understand longitude %O\n",long);
   }
   lat=(float)_lat;
   long=(float)_long;
}

float|string dwim(string what,string direction)
{
   float d,m,s;
   string dir=0;
   int neg=0;
#define DIV "%*[ \t\r\n'`°\"]"

   if (sscanf(what,"-%s",what)) neg=1;

   what=upper_case(what);

   sscanf(what,"%f"DIV"%f"DIV"%f"DIV"%["+direction+"]",d,m,s,dir)==7 ||
      sscanf(what,"%f"DIV"%f"DIV       "%["+direction+"]",d,m,  dir)==5 ||
      sscanf(what,"%f"DIV              "%["+direction+"]",d,    dir);

   if (dir==direction[1..1]) neg=!neg;

   d+=m/60+s/3600;
   return neg?-d:d;
}

//! method string latitude(void|int n)
//! method string longitude(void|int n)
//!	Returns the nicely formatted latitude
//!	or longitude. 
//!	<pre>
//!	n    format
//!	-    17°42.19'N       42°22.2'W
//!	1    17.703°N         42.37°W
//!	2    17°42.18'N       42°22.2'W
//!	3    17°42'10.4"N     42°22'12"W
//!	-1   17.703           -42.37
//!	</pre>

string prettyprint(float what,int n,string directions)
{
   if (what<0) what=-what,directions=directions[1..];
   else directions=directions[..0];

   switch (n)
   {
      case -1: return sprintf("%.5g",what);
      case 1:
	 return sprintf("%.5g°%s",what,directions);
      case 3:
	 return sprintf("%d°%d'%.3g\"%s",
			(int)floor(what),(int)floor(60*(what-floor(what))),
			3600*(what-floor(60*what)/60),
			directions);
      default:
	 return sprintf("%d°%.5g'%s",
			(int)floor(what),60*(what-floor(what)),
			directions);
   }
}

string latitude(void|int n)
{
   return prettyprint(lat,n,"NS");
}

string longitude(void|int n)
{
   return prettyprint(long,n,"EW");
}


//! method array cast("array")
//!	It is possible to cast the position to an array,
//!	<tt>({float lat,float long})</tt>.

string|array cast(string to)
{
   if (to[..4]=="array")
      return ({lat,long});

   if (to[..5]=="string")
      return latitude()+" "+longitude();

   error("can't cast to %O\n",to);
}

//! method int __hash()
//! method int `==(Position pos)
//! method int `<(Position pos)
//! method int `>(Position pos)
//!	These exist solely for purpose of detecting
//!	equal positions, for instance when used as keys in mappings.

int __hash()
{
   return (int)(lat*3600000+long*3600000);
}

int `==(object pos)
{
   return (pos->lat==lat && pos->long==long);
}

int `<(object pos)
{
   if (pos->lat>lat) return 1;
   else if (pos->lat==lat && pos->long>long) return 1;
   return 0;
}

int `>(object pos)
{
   if (pos->lat<lat) return 1;
   else if (pos->lat==lat && pos->long<long) return 1;
   return 0;
}

string _sprintf(int t)
{
   if (t=='O')
      return "Position("+longitude()+", "+latitude()+")";
   return 0;
}
