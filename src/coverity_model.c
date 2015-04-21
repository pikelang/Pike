/*
 * $Id$
 *
 * Coverity modeling file.
 *
 * Upload to https://scan.coverity.com/projects/1452?tab=analysis_settings .
 *
 * Notes:
 *   * This file may not include any header files, so any derived
 *     or symbolic types must be defined here.
 *
 *   * Full structs and unions are only needed if the fields are accessed.
 *
 *   * Uninitialized variables have any value.
 *
 * Henrik Grubbström 2015-04-21.
 */

/*
 * Typedefs and constants.
 */

#define NULL	0
#define INT32	int

typedef unsigned int socklen_t;

/*
 * STDC
 */

/*
 * POSIX
 */

/* This function doesn't have a model by default. */

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
  int may_fail;
  char first, last;
  __coverity_negative_sink__(size);
  if (!src)
    return NULL;
  if (may_fail)
    return NULL;
  dst[0] = first;
  dst[size-1] = first;
  __coverity_writeall__(dst);
  return dst;
}

