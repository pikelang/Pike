/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: combine_path.h,v 1.15 2004/05/01 15:25:04 mast Exp $
*/

/*
 * Combine path template.
 *
 */

#undef IS_SEP
#undef IS_ANY_SEP
#undef IS_ABS
#undef IS_ROOT
#undef COMBINE_PATH
#undef APPEND_PATH
#undef CHAR_CURRENT
#undef CHAR_ROOT

#define COMBINE_PATH_DEBUG 0

#ifdef UNIX_COMBINE_PATH
#define IS_SEP(X) ( (X)=='/' )
#define IS_ABS(X) (IS_SEP( INDEX_PCHARP((X),0))?1:0)
#define APPEND_PATH append_path_unix
#define COMBINE_PATH combine_path_unix
#define CHAR_CURRENT '.'
#define CHAR_ROOT '/'
#endif /* UNIX_COMBINE_PATH */



#ifdef NT_COMBINE_PATH
#define IS_SEP(X) ( (X) == '/' || (X) == '\\' )

static int find_absolute(PCHARP s)
{
  int c0=INDEX_PCHARP(s,0);
  int c1=c0?INDEX_PCHARP(s,1):0;
  if(isalpha(c0) && c1==':' && IS_SEP(INDEX_PCHARP(s,2)))
    return 3;

  if(IS_SEP(c0) && IS_SEP(c1))
  {
    int l;
    for(l=2;INDEX_PCHARP(s,l) && !IS_SEP(INDEX_PCHARP(s,l));l++);
    return INDEX_PCHARP(s,l)? l+1:l;
  }

  return 0;
}
#define IS_ABS(X) find_absolute((X))
#define IS_ROOT(X) ( IS_SEP( INDEX_PCHARP((X),0) )?1:0)

#define APPEND_PATH append_path_nt
#define COMBINE_PATH combine_path_nt

#define CHAR_CURRENT '.'
#define CHAR_ROOT '/'

#endif /* NT_COMBINE_PATH */


#ifdef AMIGAOS_COMBINE_PATH
#define IS_SEP(X) ( (X)=='/' )
#define IS_ANY_SEP(X) ( (X) == '/' || (X) == ':' )
#define IS_ABS(X) find_absolute2((X))
#define IS_ROOT(X) ( ( INDEX_PCHARP((X),0) == CHAR_ROOT)?1:0)
#define APPEND_PATH append_path_amigaos
#define COMBINE_PATH combine_path_amigaos
#define CHAR_ROOT ':'

static int find_absolute2(PCHARP s)
{
  int r=0, p=0;
  int c;
  while((c=INDEX_PCHARP(s,p))) {
    ++p;
    if(c == CHAR_ROOT)
      r = p;
  }
  return r>1? r:0;
}

#endif /* AMIGAOS_COMBINE_PATH */


#ifndef IS_ANY_SEP
#define IS_ANY_SEP(X) IS_SEP(X)
#endif

static void APPEND_PATH(struct string_builder *s,
			PCHARP path,
			size_t len)
{
  size_t from=0;
  int tmp, abs;

  /* First, check if path is absolute, 
   * if so ignore anything already in 's'
   */
  abs=IS_ABS(MKPCHARP_STR(s->s));
  if((tmp=IS_ABS(path)))
  {
    s->s->len=0;
    s->known_shift=0;
    string_builder_append(s, path, tmp);
    from+=tmp;
    abs=tmp;
  }
#ifdef IS_ROOT
  else if((tmp=IS_ROOT(path)))
  {
    int tmp2;
    s->known_shift=0;
    if((tmp2=IS_ABS(MKPCHARP_STR(s->s))))
    {
      s->s->len=tmp2;
      abs=tmp2;
    }else{
      s->s->len=0;
      string_builder_append(s, path, tmp);
      abs=tmp;
    }
    from+=tmp;
  }
#endif

#define LAST_PUSHED() (s->s->len ? index_shared_string(s->s,s->s->len-1) : 0)
#define PUSH(X) string_builder_putchar(s,(X))

  /* Ensure s ends with a separator. */
  if(s->s->len && !IS_ANY_SEP(LAST_PUSHED()))
    PUSH('/');

  if (!len) return;

#ifdef CHAR_CURRENT
  /* Remove initial "./" if any. */
  if(s->s->len==2)
  {
    PCHARP to=MKPCHARP_STR(s->s);
    if(INDEX_PCHARP(to, 0) == CHAR_CURRENT)
    {
      s->s->len=0;
      s->known_shift=0;
    }
  }
#endif

  while(1)
  {
#if COMBINE_PATH_DEBUG > 1
    s->s->str[s->s->len]=0;
    fprintf(stderr, "combine_path(2),   TO: \"%s\"\n", s->s->str);
    fprintf(stderr, "combine_path(2), FROM (%d): \"%s\"\n",
	    from, path.ptr+from);
#endif
    if(IS_SEP(LAST_PUSHED()))
    {
#ifdef AMIGAOS_COMBINE_PATH
      if(from<len && INDEX_PCHARP(path, from) == '/' &&
	 s->s->len>1 && !IS_ANY_SEP(index_shared_string(s->s,s->s->len-2))) {
	/* Handle "//" */
	int tmp=s->s->len-2;
	while(tmp>0 && !IS_ANY_SEP(index_shared_string(s->s,tmp-1)))
	  --tmp;
	s->s->len=tmp;
	s->known_shift=0;
	from++;
	continue;
      }
#else /* !AMIGAOS_COMBINE_PATH */
      while(s->s->len && IS_SEP(LAST_PUSHED()))
	s->s->len--;
      PUSH('/');
      if(from<len && INDEX_PCHARP(path, from) == '.')
      {
	int c3;
#if COMBINE_PATH_DEBUG > 0
	s->s->str[s->s->len]=0;
	fprintf(stderr, "combine_path(0),   TO: \"%s\"\n", s->s->str);
	fprintf(stderr, "combine_path(0), FROM (%d): \"%s\"\n",
		from, path.ptr+from);
#endif

	switch(INDEX_PCHARP(path, from+1))
	{
	  case '.':
	    c3=INDEX_PCHARP(path, from+2);
	    if(IS_SEP(c3) || !c3)
	    {
	      /* Handle "..". */
	      int tmp=s->s->len-1;
	      if (tmp) {
		while(--tmp>=0)
		  if(IS_SEP(index_shared_string(s->s, tmp)))
		    break;
		tmp++;
	      } else if (IS_SEP(index_shared_string(s->s, 0))) {
		tmp++;
	      }
	      
	      if (tmp < abs)
		tmp = abs;
	      else
		if ((tmp+1 < s->s->len) &&
		    (index_shared_string(s->s,tmp)=='.') &&
		    (index_shared_string(s->s,tmp+1)=='.') && 
		    ( (tmp+2 == s->s->len) ||
		      IS_SEP(index_shared_string(s->s,tmp+2))))
		  break;

	      from+=(c3? 3:2);
	      s->s->len=tmp;
	      s->known_shift=0;

#if COMBINE_PATH_DEBUG > 0
	      s->s->str[s->s->len]=0;
	      fprintf(stderr,"combine_path(1),   TO: %s\n",s->s->str);
	      fprintf(stderr,"combine_path(1), FROM (%d): %s\n",from,path.ptr+from);
#endif
	      continue;
	    }
	    break;
	    
	  case 0:
	  case '/':
#ifdef NT_COMBINE_PATH
	  case '\\':
#endif
	    /* Handle ".". */
	    from++;
	    continue;
	}
      }
#endif /* !AMIGAOS_COMBINE_PATH */
    }

    if(from>=len) break;
    PUSH(INDEX_PCHARP(path, from++));
  }
  if((s->s->len > 1) && (s->s->len > abs) &&
     !IS_SEP(INDEX_PCHARP(path, from-1)) &&
     IS_SEP(LAST_PUSHED()))
    s->s->len--;
  
  if(!s->s->len)
  {
    if(abs)
    {
      PUSH(CHAR_ROOT);
#ifdef CHAR_CURRENT
    }else{
      PUSH(CHAR_CURRENT);
      if(IS_SEP(INDEX_PCHARP(path, from-1)))
	PUSH('/');
#endif
    }
  }
}

#define F_FUNC(X) PIKE_CONCAT(f_,X)

void F_FUNC(COMBINE_PATH)(INT32 args)
{
  int e;
  int root=0;
  struct string_builder ret;
  ONERROR tmp;

  check_all_args(DEFINETOSTR(COMBINE_PATH),args,
		 BIT_STRING, BIT_STRING | BIT_MANY | BIT_VOID, 0);

  init_string_builder(&ret, 0);
  SET_ONERROR(tmp, free_string_builder, &ret);

  for(e=args-1;e>root;e--)
  {
    if(IS_ABS(MKPCHARP_STR(Pike_sp[e-args].u.string)))
    {
      root=e;
      break;
    }
  }

  APPEND_PATH(&ret,
	      MKPCHARP_STR(Pike_sp[root-args].u.string),
	      Pike_sp[root-args].u.string->len);
  root++;

#ifdef IS_ROOT
  for(e=args-1;e>root;e--)
  {
    if(IS_ROOT(MKPCHARP_STR(Pike_sp[e-args].u.string)))
    {
      root=e;
      break;
    }
  }
#endif
  
  while(root<args)
  {
    APPEND_PATH(&ret,
		MKPCHARP_STR(Pike_sp[root-args].u.string),
		Pike_sp[root-args].u.string->len);
    root++;
  }
  UNSET_ONERROR(tmp);
  pop_n_elems(args);
  push_string(finish_string_builder(&ret));
}



#undef UNIX_COMBINE_PATH
#undef NT_COMBINE_PATH
#undef AMIGAOS_COMBINE_PATH
