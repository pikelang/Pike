/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: filesystem.h,v 1.2 2002/10/08 20:22:29 nilsson Exp $
\*/

struct file_ret *aap_find_file( char *s, int len,
                                char *ho, int hlen, 
                                struct filesystem *f );

void set_filesystem( struct args *a, struct pstring loc )
{
}
