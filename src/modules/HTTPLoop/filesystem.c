/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: filesystem.c,v 1.10 2004/03/10 16:31:41 nilsson Exp $
*/

#include "config.h"
#include <global.h>
#include <fdlib.h>

#ifdef _REENTRANT
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "accept_and_parse.h"
#include "filesystem.h"
#include "cache.h"
#include "util.h"


struct file_ret *aap_find_file( char *s, int len, 
                                char *ho, int hlen, 
                                struct filesystem *f )
{
  /* FIXME: Clobbering between s above and s below?? */
  PIKE_STAT_T s;
  char *fname = alloca( fs->base.len + len + hlen + 2);
  int res;


#ifdef FS_STATS
  fs->lookups++;
#endif
  MEMCPY( fname, fs->base.str, fs->base.len );
  MEMCPY( fname+fs->base.len, ho, hlen );
  MEMCPY( fname+fs->base.len+hlen, "/", 1 );
  MEMCPY( fname+fs->base.len+hlen+1, s, len );

  res = fd_stat( fname, &s );
    
  if(!res) /* ok. The file exists. */
  {
    if(S_ISREG( s.st_mode )) /* and it is a file... */
    {
      int fd;
      fd = open( fname, O_RDONLY, 0 );
      if(fd != -1)            /* and it can be opened... */
      {
        struct file_ret *r = new_file_ret(fd, s.st_size, s.st_mtime);
#ifdef FS_STATS
        fs->hits++;
#endif
        return r;
      }
#ifdef FS_STATS
      else
        fs->noperm++;
#endif
    }
#ifdef FS_STATS
    else
      fs->notfile++;
#endif
  }
#ifdef FS_STATS
  else
    fs->nofile++;
#endif
  return 0;
}
                    
#endif
