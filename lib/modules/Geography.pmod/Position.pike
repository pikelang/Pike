#pike __REAL_VERSION__

//! This class contains a geographical position,
//! ie a point on the earths surface. The resulting
//! position object implements comparision methods
//! (__hash, `==, `< and `>) so that you can compare
//! and sort positions as well as using them as index
//! in mappings. Comparision is made primary on latidue
//! and secondly on longitude. It does not currently
//! take the ellipsoid into account.
//!
//! It is possible to cast a position into an array, which
//! will yield ({ float latitude, float longitude }), as
//! well as into a string.


//! Latitude (N--S) of the position, in degrees.
//! Positive number is north, negative number is south.
float lat;

//! Longitude (W--E) of the position, in degrees.
//! Positive number is east, negativa number is west.
float long;

//! @decl void create(float lat, float long)
//! @decl void create(string lat, string long)
//! @decl void create(string both)
//!
//! Constructor for this class. If feeded with strings,
//! it will perform a dwim scan on the strings. If they
//! fails to be understood, there will be an exception.
//!
void create(int|float|string _lat,void|int|float|string _long)
{
   if (stringp(_lat))
   {
      if (zero_type(_long))
      {
	 string tmp;
	 if (sscanf(_lat,"%sN %s",tmp,_long)==2) _lat=tmp+"N";
	 else if (sscanf(_lat,"%sS %s",tmp,_long)==2) _lat=tmp+"S";
	 else if (sscanf(_lat,"%sW %s",tmp,_lat)==2) _long=tmp+"W";
	 else if (sscanf(_lat,"%sE %s",tmp,_lat)==2) _long=tmp+"N";
	 else if (sscanf(_lat,"%s %s",tmp,_long)==2) _lat=tmp;
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
   set_ellipsoid("WGS 84");
}

private float|string dwim(string what,string direction)
{
   float d,m,s;
   string dir=0;
   int neg=0;
#define DIV "%*[ \t\r\n'`°\":.]"

   if (sscanf(what,"-%s",what)) neg=1;

   what=upper_case(what);

   sscanf(what,"%f"DIV"%f"DIV"%f"DIV"%["+direction+"]",d,m,s,dir)==7 ||
      sscanf(what,"%f"DIV"%f"DIV       "%["+direction+"]",d,m,  dir)==5 ||
      sscanf(what,"%f"DIV              "%["+direction+"]",d,    dir);

   if (dir==direction[1..1]) neg=!neg;

   d+=m/60+s/3600;
   return neg?-d:d;
}

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

//! @decl string latitude(void|int n)
//! @decl string longitude(void|int n)
//!
//! Returns the nicely formatted latitude or longitude.
//!
//! @tt{
//!  n    format
//!  -    17°42.19'N       42°22.2'W
//!  1    17.703°N         42.37°W
//!  2    17°42.18'N       42°22.2'W
//!  3    17°42'10.4"N     42°22'12"W
//! -1    17.703           -42.37
//! @}

string latitude(void|int n)
{
   return prettyprint(lat,n,"NS");
}

string longitude(void|int n)
{
   return prettyprint(long,n,"EW");
}

//! Returns the standard map grid system
//! for the current position. Can either
//! be "UPS" or "UTM".
string standard_grid() {
  if(lat>84.0 || lat<-80.0) return "UPS";
  return "UTM";
}

//! The polar radius is how many meters the earth
//! radius is at the poles (north-south direction).
float polar_radius;

//! The equatorial radius is how many meters the earth
//! radius is at the equator (east-west direction).
float equatorial_radius;

//! Returns the flattening factor for the selected
//! earth approximation ellipsoid.
float flattening() {
  return (equatorial_radius - polar_radius) / equatorial_radius;
}

//! Returns the first eccentricity squared for the
//! selected earth approximation ellipsoid.
float eccentricity_squared() {
  float f = flattening();
  return 2*f - pow(f,2);
}

/*
     "Australian National" : ({ 6378160, 0.006694542 }),
     "Bessel 1841" : ({ 6377397, 0.006674372 }),
     "Bessel 1841 (Nambia)" : ({ 6377484, 0.006674372 }),
     "Helmert 1906" : ({ 6378200, 0.006693422 }),
     "Modified Airy" : ({ 6377340, 0.00667054 }),
     "Modified Everest" : ({ 6377304, 0.006637847 }),
     "Modified Fischer 1960" : ({ 6378155, 0.006693422 }),
*/

//! A mapping with reference ellipsoids, which can be fed to the
//! UTM converter. The mapping maps the name of the ellipsoid to
//! an array where the first element is a float describing the
//! equatorial radius and the second element is a float describing
//! the polar radius.
//!
constant ellipsoids = 
  ([ "Airy 1830" : ({ 6377563.396, 6356256.909237 }),
     "Bessel 1841" : ({ 6377397.155, 6356078.962818 }),
     "Clarke 1866" : ({ 6378206.4, 6356583.799999 }),
     "Clarke 1880" : ({ 6378249.145, 6356514.869550 }),
     "Everest 1830" : ({ 6377276.345, 6356075.413140 }),
     "Fisher 1960" : ({ 6378166.0, 6356784.283607 }),
     "Fisher 1968" : ({ 6378150.0, 6356768.337244 }),
     "G R S 1967" : ({ 6378160.0, 6356774.516091 }),
     "G R S 1975" : ({ 6378140.0, 6356755.288158 }),
     "G R S 1980" : ({ 6378137.0, 6356752.314140 }),
     "Hough 1956" : ({ 6378270.0, 6356794.343434 }),
     "International" : ({ 6378388.0, 6356911.946128 }),
     "Krassovsky 1940" : ({ 6378245.0, 6356863.018773 }),
     "South American 1969" : ({ 6378160.0, 6356774.719195 }),
     "WGS 60" : ({ 6378165.0, 6356783.286959 }),
     "WGS 66" : ({ 6378145.0, 6356759.769489 }),
     "WGS 72" : ({ 6378135.0, 6356750.520016 }),
     "WGS 84" : ({ 6378137.0, 6356752.3142 }),
  ]);

private constant ellipsoid_sym =
  ([ "airy" : "Airy 1830",
     "grs 1697" : "G R S 1967",
     "grs 1975" : "G R S 1975",
     "grs 1980" : "G R S 1980",
     "krassovsky" : "Krassovsky 1940",
     "mercury" : "Fisher 1960",
     "wgs-60" : "WGS 60",
     "wgs-66" : "WGS 66",
     "wgs-72" : "WGS 72",
     "wgs-84" : "WGS 84",
  ]);

//! @decl int(0..1) set_ellipsoid(string name)
//! @decl int(0..1) set_ellipsoid(float equatorial_radius, float polar_radius)
//!
//! Sets the equatorial and polar radius to the provided values.
//! A name can also be provided, in which case the radius will be looked
//! up in the ellipsoid mapping. The function returns 1 upon success, 0 on
//! failure. Warning: the longitude and lattitude are not converted to the
//! new ellipsoid.
int(0..1) set_ellipsoid(string|float er, float|void pr) {
  if(stringp(er)) {
    if(ellipsoid_sym[lower_case(er)])
      er = ellipsoid_sym[lower_case(er)];
    if(ellipsoids[er])
      [er,pr] = ellipsoids[er];
    else
      return 0;
  }
  equatorial_radius = er;
  polar_radius = pr;
  return 1;
}

// The following code for UTM conversion is base on code by
// Chuck Gants and equations from USGS Bulletin 1532.

//! Returns the UTM zone number for the current longitude, with
//! correction for the Svalbard deviations.
int UTM_zone_number() {
  int zone = (int)((long + 180)/6) + 1;

  if( lat >= 56.0 && lat < 64.0 && long >= 3.0 && long < 12.0 )
    zone = 32;

  // Special zones for Svalbard
  if( lat >= 72.0 && lat < 84.0 )
    {
      if(      long >= 0.0  && long <  9.0 ) zone = 31;
      else if( long >= 9.0  && long < 21.0 ) zone = 33;
      else if( long >= 21.0 && long < 33.0 ) zone = 35;
      else if( long >= 33.0 && long < 42.0 ) zone = 37;
    }

  return zone;
}

//! Returns the UTM letter designator for the current latitude.
//! Returns "Z" if latitude is outside the UTM limits of 84N to 80S.
string UTM_zone_designator()
{
  if(lat > 84) return "Z";
  int min = 72;
  foreach( "XWVUTSRQPNMLKJHGFEDC"/1, string code ) {
    if(lat >= min) return code;
    min -= 8;
  }
  return "Z";
}

//! Returns the offset within the present UTM cell.
//! The result will be returned in an array of floats,
//! containing easting and northing.
array(float) UTM_offset() {

  float k0 = 0.9996;
  float LatRad = lat * Math.pi/180;
  float LongRad = long * Math.pi/180;

  float LongOriginRad = ((UTM_zone_number() - 1)*6 - 180 + 3) * Math.pi/180;
  // +3 puts origin in middle of zone
  float ecc = eccentricity_squared();
  float eccPrime = ecc/(1-ecc);

  float N = equatorial_radius/sqrt(1-ecc*sin(LatRad)*sin(LatRad));
  float T = tan(LatRad)*tan(LatRad);
  float C = eccPrime*cos(LatRad)*cos(LatRad);
  float A = cos(LatRad)*(LongRad-LongOriginRad);

  float M = equatorial_radius *
    ((1 - ecc/4        - 3*ecc*ecc/64    - 5*ecc*ecc*ecc/256)*LatRad 
     - (3*ecc/8        + 3*ecc*ecc/32    + 45*ecc*ecc*ecc/1024)*sin(2*LatRad)
     + (15*ecc*ecc/256 + 45*ecc*ecc*ecc/1024)*sin(4*LatRad) 
     - (35*ecc*ecc*ecc/3072)*sin(6*LatRad));

  float UTME = (k0*N*(A+(1-T+C)*A*A*A/6
		      + (5-18*T+T*T+72*C-58*eccPrime)*A*A*A*A*A/120)
		+ 500000.0);

  float UTMN = (k0*(M+N*tan(LatRad)*
		    (A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
		     + (61-58*T+T*T+600*C-330*eccPrime)*A*A*A*A*A*A/720)));
  if(lat < 0)
    UTMN += 10000000.0; // 10000000 meter offset for southern hemisphere

  return ({ UTME, UTMN });
}

//! Returns the current UTM coordinates position.
//! An example output is
//! "32T 442063.562 5247479.500"
//! where the parts is zone number + zone designator,
//! easting and northing.
string UTM() {
  return sprintf("%d%s %f %f", UTM_zone_number(), UTM_zone_designator(),
		 @UTM_offset());
}

//! Sets the longitude and lattitude from the given
//! UTM coordinates.
void set_from_UTM(int zone_number, string zone_designator, float UTME, float UTMN) {

  float ecc = eccentricity_squared();
  float eccPrime = (ecc)/(1-ecc);

  float k0 = 0.9996;
  float e1 = (1-sqrt(1-ecc))/(1+sqrt(1-ecc));

  UTME -= 500000.0; // remove 500,000 meter offset for longitude
  if(zone_designator[0]-'N' < 0)
    UTMN -= 10000000.0; // remove 10,000,000 meter offset used for southern hemisphere

  float LongOrigin = (zone_number - 1)*6 - 180 + 3.0;  // +3 puts origin in middle of zone

  float M = UTMN / k0;
  float mu = M/(equatorial_radius*(1-ecc/4-3*ecc*ecc/64-5*ecc*ecc*ecc/256));

  float phi1Rad = mu + (3*e1/2-27*e1*e1*e1/32)*sin(2*mu) 
    + (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
    +(151*e1*e1*e1/96)*sin(6*mu);
  float phi1 = phi1Rad * 180/Math.pi;

  float N1 = equatorial_radius/sqrt(1-ecc*sin(phi1Rad)*sin(phi1Rad));
  float T1 = tan(phi1Rad)*tan(phi1Rad);
  float C1 = eccPrime*cos(phi1Rad)*cos(phi1Rad);
  float R1 = equatorial_radius*(1-ecc)/pow(1-ecc*sin(phi1Rad)*sin(phi1Rad), 1.5);
  float D = UTME/(N1*k0);

  lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrime)*D*D*D*D/24
                                        +(61+90*T1+298*C1+45*T1*T1-252*eccPrime-3*C1*C1)*D*D*D*D*D*D/720);
  lat = lat * 180/Math.pi;

  long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrime+24*T1*T1)
	  *D*D*D*D*D/120)/cos(phi1Rad);
  long = LongOrigin + long * 180/Math.pi;
}

string GEOREF() {
  int x_square = (int)((90+lat)/15);
  int y_square = (int)((180+long)/15);
  int x_sub = (int)(90+lat - x_square*15);
  int y_sub = (int)(180+long - y_square*15);

  string pos = ("ABCDEFGHJKLM"/1)[ y_square ] + 
    ("ABCDEFGHJKLMNPQRSTUVWXZY"/1)[ x_square ] +
    ("ABCDEFGHJKLMNPQ"/1)[ y_sub ] +
    ("ABCDEFGHJKLMNPQ"/1)[ x_sub ];

  return pos + (int)floor(60*(long-floor(long))) + (int)floor(60*(lat-floor(lat)));
}


// --- "Technical" methods --------------

string|array cast(string to)
{
   if (to[..4]=="array")
      return ({lat,long});

   if (to[..5]=="string")
      return latitude()+" "+longitude();

   error("can't cast to %O\n",to);
}

//!
int __hash()
{
   return (int)(lat*3600000+long*3600000);
}

//!
int `==(object pos)
{
   return (pos->lat==lat && pos->long==long);
}

//!
int `<(object pos)
{
   if (pos->lat>lat) return 1;
   else if (pos->lat==lat && pos->long>long) return 1;
   return 0;
}

//!
int `>(object pos)
{
   if (pos->lat<lat) return 1;
   else if (pos->lat==lat && pos->long<long) return 1;
   return 0;
}

//!
string _sprintf(int|void t)
{
  switch(t)
  {
    case 't': return "Geography.Position";
    case 'O': return "Position("+latitude()+", "+longitude()+")";
  }
  return 0;
}
