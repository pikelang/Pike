static private int stage=0;
static private object defcal;
static private multiset protect=(<"YMD">);

#include "localization.h"

#if 1
mixed `[](string what)
{
   if ( protect[what] ) return ([])[0];
   if (!defcal)
   {
      if (stage) return ([])[0];
      stage++;
      defcal=master()->resolv("Calendar")[default_calendar];
   }
   if (what=="II") return 1; // Calendar.II
   return defcal[what];
}
mixed `-> = `[];
#endif
