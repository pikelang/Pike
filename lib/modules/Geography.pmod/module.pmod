// ----------------------------------------------------------------

// convert between latitude-longitude and Swedish RT38 map coordinates
// RT38 is simpler, but differs 0-5 meters from the modern RT90

// code translated from Perl code by Aronsson from Python code by Kent

#define DEG2RAD(DEG) ((Math.pi/180.0)*(DEG))
#define RAD2DEG(RAD) ((RAD)*(180.0/Math.pi))


static private constant rt38_y0 = 1500000;
static private constant rt38_lng0 = DEG2RAD(15.80827778);
static private constant rt38_k0a = 6366742.5194;
static private constant rt38_beta1 = 0.00083522527;
static private constant rt38_beta2 = 0.000000756302;
static private constant rt38_beta3 = 0.000000001193;
static private constant rt38_delta1 = 0.000835225613;
static private constant rt38_delta2 = 0.000000058706;
static private constant rt38_delta3 = 0.000000000166;

//! @decl array(float) rt38_to_longlat(int|float|string x_n,int|float|string y_e)
//! @decl array(float) longlat_to_rt38(float lat, float long)
//!    convert lat/long coordinates to and from RT38, "Swedish grid".
//!    Note that RT38 differs a few meters from the current RT90!
//!    x is north, y is east.

array(float) longlat_to_rt38(float lat, float long)
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

array(float) rt38_to_longlat(int|float|string x_n,int|float|string y_e)
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

   return ({RAD2DEG(rlat),RAD2DEG(rlong)});
}

//! Create a Position object from a RT38 coordinate

Geography.Position PositionRT38(int|float|string x,int|float|string y)
{
   return Geography.Position(@rt38_to_longlat(x,y));
}

// ----------------------------------------------------------------
