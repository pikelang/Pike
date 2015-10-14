#pike __REAL_VERSION__

//! The @tt{Web.Api@} has modules and classes for communicating with various
//! @tt{(RESTful)@} web api's such as @[Web.Api.Facebook],
//! @[Web.Api.Instagram], @[Web.Api.Twitter] etc.

//! Default user agent in HTTP calls
constant USER_AGENT = "Mozilla 4.0 (Pike/" + __REAL_MAJOR__ + "." +
                      __REAL_MINOR__ + "." + __REAL_BUILD__ + ")";
