#pike __REAL_VERSION__

// by Mirar 

inherit Calendar_I.Gregorian : christ;

class Year
{
   inherit Calendar_I.Gregorian.Year;

   int leap() // tack, hubbe... :-)
   {
      switch(y%900)
      {
        case 0:
        case 100:
        case 300:
        case 400:
        case 500:
        case 700:
        case 800: return 0;
        case 200:
        case 600: return 1;
        default: return !(y%4);
      }
   }
};
