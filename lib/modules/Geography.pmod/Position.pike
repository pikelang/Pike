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
//! Positive number is east, negative number is west.
float long;

//! Altitud of the position, in meters. Positive numbers
//! is up. Zero is the shell of the current ellipsoid.
float alt;

//! @decl void create(float lat, float long, void|float alt)
//! @decl void create(string lat, string long)
//! @decl void create(string both)
//!
//! Constructor for this class. If fed with strings,
//! it will perform a dwim scan on the strings. If they
//! fails to be understood, there will be an exception.
//!
void create(void|int|float|string _lat,
	    void|int|float|string _long, void|float _alt)
{
   if(zero_type(_lat)) return;
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
   alt=(float)_alt;
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
	 return sprintf("%.3f°%s",what,directions);
      case 3:
	 return sprintf("%d°%d'%.1f\"%s",
			(int)floor(what),(int)floor(60*(what-floor(what))),
			3600*(what-floor(60*what)/60),
			directions);
      default:
	 return sprintf("%d°%.3f'%s",
			(int)floor(what),60*(what-floor(what)),
			directions);
   }
}

//! @decl string latitude(void|int n)
//! @decl string longitude(void|int n)
//!
//! Returns the nicely formatted latitude or longitude.
//!
//! @int
//!   @value 0
//!     "17°42.19'N" / "42°22.2'W"
//!   @value 1
//!     "17.703°N" / "42.37°W"
//!   @value 2
//!     "17°42.18'N" / "42°22.2'W"
//!   @value 3
//!     "17°42'10.4"N" / "42°22'12"W"
//!   @value -1
//!     "17.703" / "-42.37"
//! @endint

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

//! A mapping with reference ellipsoids, which can be fed to the
//! UTM converter. The mapping maps the name of the ellipsoid to
//! an array where the first element is a float describing the
//! equatorial radius and the second element is a float describing
//! the polar radius.
//!
constant ellipsoids = 
  ([ "Airy 1830" : ({ 6377563.396, 6356256.91 }),
     "ATS77" : ({ 6378135.0, 6356750.304922 }),
     "Australian National" : ({ 6378160.0, 6356774.719 }),
     "Bessel 1841" : ({ 6377397.155, 6356078.962818 }),
     "Bessel 1841 Namibia" : ({ 6377483.865, 6356165.382966 }),
     "Clarke 1866" : ({ 6378206.4, 6356583.799999 }),
     "Clarke 1880" : ({ 6378249.145, 6356514.869550 }),
     "Everest" : ({ 6377298.556, 6356097.550301 }),
     "Everest 1830" : ({ 6377276.345, 6356075.4133 }),
     "Everest 1948" : ({ 6377304.063, 6356103.039 }),
     "Everest 1956" : ({ 6377301.243, 6356100.228368 }),
     "Everest 1969" : ({ 6377295.664, 6356094.667915 }),
     "Everest Pakistan" : ({ 6377309.613, 6356108.570542 }),
     "Fisher 1960" : ({ 6378166.0, 6356784.283666 }),
     "Fisher 1968" : ({ 6378150.0, 6356768.337303 }),
     "GEM 10C" : ({ 6378137.0, 6356752.0 }),
     "G R S 1967" : ({ 6378160.0, 6356774.516091 }),
     "G R S 1975" : ({ 6378140.0, 6356755.288158 }),
     "G R S 1980" : ({ 6378137.0, 6356752.314140 }),
     "Helmert 1906" : ({ 6378200.0, 6356818.169628 }),
     "Hough 1956" : ({ 6378270.0, 6356794.343479 }),
     "Indonesian 1974" : ({ 6378160.0, 6356774.504086 }),
     "International 1924" : ({ 6378388.0, 6356911.946130 }),
     "Krassovsky 1940" : ({ 6378245.0, 6356863.018773 }),
     "Modified Airy" : ({ 6377340.189, 6356034.448 }),
     "Modified Fisher 1960" : ({ 6378155.0, 6356773.3205 }),
     "New International 1967" : ({ 6378157.5,  6356772.2 }),
     "SGS 85" : ({ 6378136.0, 6356751.301569 }),
     "South American 1969" : ({ 6378160.0, 6356774.719195 }),
     "Sphere" : ({ 6370997.0, 6370997.0 }),
     "WGS 60" : ({ 6378165.0, 6356783.286959 }),
     "WGS 66" : ({ 6378145.0, 6356759.769356 }),
     "WGS 72" : ({ 6378135.0, 6356750.519915 }),
     "WGS 84" : ({ 6378137.0, 6356752.314245 }),
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
//! failure.
//!
//! @string name
//!   @value "Airy 1830"
//!   @value "ATS77"
//!   @value "Australian National"
//!   @value "Bessel 1841"
//!   @value "Bessel 1841 Namibia"
//!   @value "Clarke 1866"
//!   @value "Clarke 1880"
//!   @value "Everest"
//!   @value "Everest 1830"
//!   @value "Everest 1848"
//!   @value "Everest 1856"
//!   @value "Everest 1869"
//!   @value "Everest Pakistan"
//!   @value "Fisher 1960"
//!   @value "Fisher 1968"
//!   @value "G R S 1967"
//!   @value "G R S 1975"
//!   @value "G R S 1980"
//!   @value "Helmert 1906"
//!   @value "Hough 1956"
//!   @value "Indonesian 1974"
//!   @value "Krassovsky 1940"
//!   @value "Mercury"
//!   @value "Modified Airy"
//!   @value "Modified Fisher 1960"
//!   @value "New International 1967"
//!   @value "SGS 85"
//!   @value "South American 1969"
//!   @value "Sphere"
//!   @value "WGS 60"
//!   @value "WGS 66"
//!   @value "WGS 72"
//!   @value "WGS 84"
//! @endstring
//!
//! @note
//!  The longitude and lattitude are not converted to the new ellipsoid.
//!
int(0..1) set_ellipsoid(string|float er, float|void pr) {
  if(stringp(er)) {
    if(ellipsoid_sym[lower_case(er)])
      er = ellipsoid_sym[lower_case(er)];
    if(!ellipsoids[er])
      foreach(indices(ellipsoids), string ep)
	if(lower_case(ep)==lower_case(er)) er=ep;
    if(ellipsoids[er])
      [er,pr] = ellipsoids[er];
    else
      return 0;
  }
  equatorial_radius = er;
  polar_radius = pr;
  return 1;
}


// --- UTM code

// The following code for UTM conversion is base on code by
// Chuck Gantz and equations from USGS Bulletin 1532.

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
//! where the parts are zone number + zone designator,
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


// --- GEOREF code

//! Gives the full GEOREF position for the current position, e.g. "LDJA0511".
string GEOREF() {
  int x_square = (int)((180+long)/15);
  int y_square = (int)((90+lat)/15);
  int x_sub = (int)(180+long - x_square*15);
  int y_sub = (int)(90+lat - y_square*15);

  string pos = ("ABCDEFGHJKLMNPQRSTUVWXZY"/1)[ x_square ] +
    ("ABCDEFGHJKLM"/1)[ y_square ] +
    ("ABCDEFGHJKLMNPQ"/1)[ x_sub ] +
    ("ABCDEFGHJKLMNPQ"/1)[ y_sub ];

  return sprintf("%s%02d%02d", pos,
		 (int)floor(60*(long-floor(long))),
		 (int)floor(60*(lat-floor(lat))));
}

// FIXME: set_from_GEOREF


// --- RT 38 code

#define DEG2RAD(DEG) ((Math.pi/180.0)*(DEG))
#define RAD2DEG(RAD) ((RAD)*(180.0/Math.pi))

static constant rt38_y0 = 1500000;
static constant rt38_lng0 = DEG2RAD(15.80827778);
static constant rt38_k0a = 6366742.5194;
static constant rt38_beta1 = 0.00083522527;
static constant rt38_beta2 = 0.000000756302;
static constant rt38_beta3 = 0.000000001193;
static constant rt38_delta1 = 0.000835225613;
static constant rt38_delta2 = 0.000000058706;
static constant rt38_delta3 = 0.000000000166;

//!
array(float) RT38()
{
   float rlat=DEG2RAD(lat);
   float rlong=DEG2RAD(long);

   float rlat2 = rlat - sin(rlat) * cos(rlat)
      * DEG2RAD(1376.68809
		+ 7.64689 * pow(sin(rlat),2)
		+ 0.053 * pow(sin(rlat),4)
		+ 0.0004 * pow(sin(rlat),6)) /3600;
   float ksi = atan2(tan(rlat2) , cos(rlong - rt38_lng0));
   float eta = atanh(cos(rlat2) * sin(rlong - rt38_lng0));
   float x = rt38_k0a * (ksi + rt38_beta1 * sin(2 * ksi)
			 * cosh(2 * eta) + rt38_beta2
			 * sin(4 * ksi) * cosh(4 * eta)
			 + rt38_beta3 * sin(6 * ksi) * cosh(6 * eta));
   float y = rt38_y0 + rt38_k0a * (eta + rt38_beta1 * cos(2 * ksi)
				   * sinh(2 * eta) + rt38_beta2 * cos(4 * ksi)
				   * sinh(4 * eta) + rt38_beta3 * cos(6 * ksi)
				   * sinh(6 * eta));
   return ({x, y});
}

//! Sets the longitude and lattitude from the given
//! RT38 coordinates.
void set_from_RT38(int|float|string x_n,int|float|string y_e)
{
   if (stringp(x_n)) x_n=(float)((x_n+"0000000000")[..6]);
   if (stringp(y_e)) y_e=(float)((y_e+"0000000000")[..6]);

   float ksi = x_n / rt38_k0a;
   float eta = (y_e - rt38_y0) / rt38_k0a;
   float ksi2 = ksi - rt38_delta1 * sin(2 * ksi) * cosh(2 * eta)
      - rt38_delta2 * sin(4 * ksi) * cosh(4 * eta)
      - rt38_delta3 * sin(6 * ksi) * cosh(6 * eta);
   float eta2 = eta - rt38_delta1 * cos(2 * ksi) * sinh(2 * eta)
      - rt38_delta2 * cos(4 * ksi) * sinh(4 * eta)
      - rt38_delta3 * cos(6 * ksi) * sinh(6 * eta);
   float rlat2 = asin(sin(ksi2) / cosh(eta2));
   float rlong = atan2(sinh(eta2) , cos(ksi2)) + rt38_lng0;
   float rlat = rlat2 + sin(rlat2) * cos(rlat2)
      * DEG2RAD(1385.93836 - 10.89576 * pow(sin(rlat2),2)
		+ 0.11751 * pow(sin(rlat2),4)
		- 0.00139 * pow(sin(rlat2),6)) / 3600;

   lat = RAD2DEG(rlat);
   long = RAD2DEG(rlong);
}


// --- Height releated code

// Ten by Ten Degree WGS-84 Geoid Heights from -180 to +170 Degrees of Longitude.
// Defense Mapping Agency. 12 Jan 1987. GPS UE Relevant WGS-84 Data Base Package.
constant height_values = ({
  ({ 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13 }), // 90 deg N
  ({ 3,1,-2,-3,-3,-3,-1,3,1,5,9,11,19,27,31,34,33,34,33,34,28,23,17,13,9,4,4,1,-2,-2,0,2,3,2,1,1 }),
  ({ 2,2,1,-1,-3,-7,-14,-24,-27,-25,-19,3,24,37,47,60,61,58,51,43,29,20,12,5,-2,-10,-14,-12,-10,-14,-12,-6,-2,3,6,4 }),
  ({ 2,9,17,10,13,1,-14,-30,-39,-46,-42,-21,6,29,49,65,60,57,47,41,21,18,14,7,-3,-22,-29,-32,-32,-26,-15,-2,13,17,19,6 }),
  ({ -8,8,8,1,-11,-19,-16,-18,-22,-35,-40,-26,-12,24,45,63,62,59,47,48,42,28,12,-10,-19,-33,-43,-42,-43,-29,-2,17,23,22,6,2 }),
  ({ -12,-10,-13,-20,-31,-34,-21,-16,-26,-34,-33,-35,-26,2,33,59,52,51,52,48,35,40,33,-9,-28,-39,-48,-59,-50,-28,3,23,37,18,-1,-11 }),
  ({ -7,-5,-8,-15,-28,-40,-42,-29,-22,-26,-32,-51,-40,-17,17,31,34,44,36,28,29,17,12,-20,-15,-40,-33,-34,-34,-28,7,29,43,20,4,-6 }),
  ({ 5,10,7,-7,-23,-39,-47,-34,-9,-10,-20,-45,-48,-32,-9,17,25,31,31,26,15,6,1,-29,-44,-61,-67,-59,-36,-11,21,39,49,39,22,10 }),
  ({ 13,12,11,2,-11,-28,-38,-29,-10,3,1,-11,-41,-42,-16,3,17,33,22,23,2,-3,-7,-36,-59,-90,-95,-63,-24,12,53,60,58,46,36,26 }),

  ({ 22,16,17,13,1,-12,-23,-20,-14,-3,14,10,-15,-27,-18,3,12,20,18,12,-13,-9,-28,-49,-62,-89,-102,-63,-9,33,58,73,74,63,50,32 }),

  ({ 36,22,11,6,-1,-8,-10,-8,-11,-9,1,32,4,-18,-13,-9,4,14,12,13,-2,-14,-25,-32,-38,-60,-75,-63,-26,0,35,52,68,76,64,52 }),
  ({ 51,27,10,0,-9,-11,-5,-2,-3,-1,9,35,20,-5,-6,-5,0,13,17,23,21,8,-9,-10,-11,-20,-40,-47,-45,-25,5,23,45,58,57,63 }),
  ({ 46,22,5,-2,-8,-13,-10,-7,-4,1,9,32,16,4,-8,4,12,15,22,27,34,29,14,15,15,7,-9,-25,-37,-39,-23,-14,15,33,34,45 }),
  ({ 21,6,1,-7,-12,-12,-12,-10,-7,-1,8,23,15,-2,-6,6,21,24,18,26,31,33,39,41,30,24,13,-2,-20,-32,-33,-27,-14,-2,5,20 }),
  ({ -15,-18,-18,-16,-17,-15,-10,-10,-8,-2,6,14,13,3,3,10,20,27,25,26,34,39,45,45,38,39,28,13,-1,-15,-22,-22,-18,-15,-14,-10 }),
  ({ -45,-43,-37,-32,-30,-26,-23,-22,-16,-10,-2,10,20,20,21,24,22,17,16,19,25,30,35,35,33,30,27,10,-2,-14,-23,-30,-33,-29,-35,-43 }),
  ({ -61,-60,-61,-55,-49,-44,-38,-31,-25,-16,-6,1,4,5,4,2,6,12,16,16,17,21,20,26,26,22,16,10,-1,-16,-29,-36,-46,-55,-54,-59 }),
  ({ -53,-54,-55,-52,-48,-42,-38,-38,-29,-26,-26,-24,-23,-21,-19,-16,-12,-8,-4,-1,1,4,4,6,5,4,2,-6,-15,-24,-33,-40,-48,-50,-53,-52 }),
  ({ -30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30,-30 }) // 90 deg S
});

//! Returns a very crude approximation of where the ground level is
//! at the current position, compared against the ellipsoid shell.
//! WGS-84 is assumed, but the approximation is so bad that it doesn't
//! matter which of the standard ellipsoids is used.
float approx_height() {

  int this_lat = (int)(lat/10)+9;
  int next_lat = this_lat+1%18;
  int this_long = (int)(long/10)+18;
  int next_long = this_long+1%36;

  float lat_dev = lat-(int)lat;
  float long_dev = long-(int)long;

  return height_values[this_lat][this_long] * lat_dev * long_dev +
    height_values[this_lat][next_long] * lat_dev * (1-long_dev) +
    height_values[next_lat][this_long] * (1-lat_dev) * long_dev +
    height_values[next_lat][next_long] * (1-lat_dev) * (1-long_dev);
}

//! Returns the current position as Earth Centered Earth Fixed
//! Cartesian Coordinates.
//! @returns
//!  ({ X, Y, Z })
array(float) ECEF() {

  float N = equatorial_radius /
    sqrt(1-eccentricity_squared()*sin(lat)*sin(lat));

  constant torad=Math.pi/180;
  float X = (N + alt)*cos(lat*torad)*cos(long*torad);
  float Y = (N + alt)*cos(lat*torad)*sin(long*torad);
  float Z = (N*(1-eccentricity_squared())+alt) * sin(lat*torad);

  return ({ X, Y, Z });
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
   return (objectp(pos) && pos->lat==lat && pos->long==long);
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
  return t=='O' && sprintf("%O(%s, %s)", this_program,
			   latitude(), longitude());
}

//! Calculate the euclidian distance between two Geography.Position.
//! Result is in meter. This uses the ECEF function.
float euclidian_distance(this_program p)
{
   return sqrt(`+(@map(Array.sum_arrays(
			  `-,ECEF(),p->ECEF()),
		       lambda(float f) { return f*f; })));
}

// encoder
array(float) _encode() { return ({lat,long,alt}); }
void _decode(array(float) v) { create(@v); }
