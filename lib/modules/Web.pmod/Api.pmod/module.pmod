//! The @tt{Web.Api@} has modules and classes for communicating with various
//! @tt{(RESTful)@} web api's such as @[Web.Api.Facebook],
//! @[Web.Api.Instagram], @[Web.Api.Twitter] etc.

//! Default user agent in HTTP calls
constant USER_AGENT = "Mozilla 4.0 (Pike/" + __REAL_MAJOR__ + "." +
                      __REAL_MINOR__ + "." + __REAL_BUILD__ + ")";

//! Human readable representation of @[timestamp].
//!
//! Examples are:
//!
//! @code
//!     0 .. 30 seconds:    Just now
//!     0 .. 120 seconds:   Just recently
//!   121 .. 3600 seconds:  x minutes ago
//!  3601 .. 86400 seconds: x hours ago
//!       .. and so on
//! @endcode
//!
//! @param timestamp
string time_elapsed(int timestamp)
{
  int diff = (int) time(timestamp);
  int t;

  switch (diff)
  {
    case      0 .. 30: return "Just now";
    case     31 .. 120: return "Just recently";
    case    121 .. 3600: return sprintf("%d minutes ago",(int)(diff/60.0));
    case   3601 .. 86400:
      t = (int)((diff/60.0)/60.0);
      return sprintf("%d hour%s ago", t, t > 1 ? "s" : "");

    case  86401 .. 604800:
      t = (int)(((diff/60.0)/60.0)/24);
      return sprintf("%d day%s ago", t, t > 1 ? "s" : "");

    case 604801 .. 31449600:
      t = (int)((((diff/60.0)/60.0)/24)/7);
      return sprintf("%d week%s ago", t, t > 1 ? "s" : "");
  }

  return "A long time ago";
}
