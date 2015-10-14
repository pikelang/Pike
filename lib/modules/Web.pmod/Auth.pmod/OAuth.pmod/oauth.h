#ifndef _OAUTH_H
#define _OAUTH_H

//#define OAUTH_DEBUG

#ifdef OAUTH_DEBUG
# define TRACE(X...) werror("%s:%d: %s",basename(__FILE__),__LINE__,sprintf(X))
#else
# define TRACE(X...) 0
#endif

#define STRURI          string|Standards.URI
#define ASSURE_URI(U)   U && objectp(U) && U || U && Standards.URI(U)
#define EMPTY(STR)	(!STR || !sizeof(STR))
#define NOT_NULL(STR)	STR = STR||""
#define CLASS_NAME(OBJ) sprintf("%O", object_program(OBJ))-"("-")"
#define ARG_ERROR(ARG, MSG...) \
  error("Argument exception (%s): %s\n", (ARG), sprintf(MSG))

#endif /* _OAUTH_H */
