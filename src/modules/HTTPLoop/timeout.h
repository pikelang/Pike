/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: timeout.h,v 1.3 2002/10/08 20:22:30 nilsson Exp $
\*/

void aap_init_timeouts(void);
void aap_exit_timeouts(void);
void aap_remove_timeout_thr(int *hmm);
int *aap_add_timeout_thr(THREAD_T thr, int secs);
extern MUTEX_T aap_timeout_mutex;
