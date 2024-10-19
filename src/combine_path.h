/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
#define IS_ABS(X, L) ((((L) > 0) && IS_SEP(INDEX_PCHARP((X),0)))?1:0)
#define APPEND_PATH append_path_unix
#define COMBINE_PATH combine_path_unix
#define CHAR_CURRENT '.'
#define CHAR_ROOT '/'
#endif /* UNIX_COMBINE_PATH */



#ifdef NT_COMBINE_PATH
#define IS_SEP(X) ( (X) == '/' || (X) == '\\' )

static int find_absolute(PCHARP s, ptrdiff_t len)
{
  int c0=len ? INDEX_PCHARP(s,0) : 0;
  int c1=c0 && len>1 ? INDEX_PCHARP(s,1) : 0;
  /* The following used to use isalpha(c0), but it apparently can
   * index out-of-bound memory in the msvc 9.0 crt when given 16-bit
   * char values (known to occur with 0x20ac, at least). Besides, a
   * drive letter is limited to a..z, so this is faster and more
   * correct. */
  if(((c0 >= 'A' && c0 <= 'Z') ||
      (c0 >= 'a' && c0 <= 'z')) &&
     c1==':' && (len >= 3) && IS_SEP(INDEX_PCHARP(s,2)))
    return 3;

  if((len >= 2) && IS_SEP(c0) && IS_SEP(c1))
  {
    int l;
    for(l=2; l < len && INDEX_PCHARP(s,l) && !IS_SEP(INDEX_PCHARP(s,l));l++);
    return l < len && INDEX_PCHARP(s,l) ? l+1:l;
  }

  return 0;
}
#define IS_ABS(X, L) find_absolute((X), (L))
#define IS_ROOT(X, L) ( (((L) > 0) && IS_SEP( INDEX_PCHARP((X),0) ))?1:0)

#define APPEND_PATH append_path_nt
#define COMBINE_PATH combine_path_nt

#define CHAR_CURRENT '.'
#define CHAR_ROOT '/'

#endif /* NT_COMBINE_PATH */


#ifdef AMIGAOS_COMBINE_PATH
#define IS_SEP(X) ( (X)=='/' )
#define IS_ANY_SEP(X) ( (X) == '/' || (X) == ':' )
#define IS_ABS(X, L) find_absolute2((X), (L))
#define IS_ROOT(X, L) ( ((L) > 0) && ( INDEX_PCHARP((X),0) == CHAR_ROOT)?1:0)
#define APPEND_PATH append_path_amigaos
#define COMBINE_PATH combine_path_amigaos
#define CHAR_ROOT ':'

static int find_absolute2(PCHARP s, ptrdiff_t len)
{
  int r=0, p=0;
  int c;
  while((len > 0) && (c=INDEX_PCHARP(s,p))) {
    ++p;
    --len;
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
  abs=IS_ABS(MKPCHARP_STR(s->s), s->s->len);
  if((tmp=IS_ABS(path, len)))
  {
    s->s->len=0;
    s->known_shift=0;
    string_builder_append(s, path, tmp);
    from+=tmp;
    abs=tmp;
  }
#ifdef IS_ROOT
  else if((tmp=IS_ROOT(path, len)))
  {
    int tmp2;
    s->known_shift=0;
    if((tmp2=IS_ABS(MKPCHARP_STR(s->s), s->s->len)))
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
    /* s->s->str[s->s->len]=0; */
    fprintf(stderr, "combine_path(2),   TO: \"%s\"[%d]\n", s->s->str, s->s->len);
    fprintf(stderr, "combine_path(2), FROM (%d): \"%s\"[%d]\n",
	    from, path.ptr+from, len - from);
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
      do {
	/* Remove sequences of ending slashes and single dots
	 * from the result path.
	 */
	while(s->s->len && IS_SEP(LAST_PUSHED()))
	  s->s->len--;
	if (LAST_PUSHED() == '.') {
	  if (s->s->len == 1) {
	    /* Result path started with "./". */
	    s->s->len = 0;
	    break;
	  } else if (IS_SEP(index_shared_string(s->s, s->s->len-2))) {
	    /* Result path ended with "/./". */
	    s->s->len -= 2;
	    continue;
	  }
	}
	/* Restore a single slash as directory separator. */
	PUSH('/');
	break;
      } while (1);
      if(from<len && INDEX_PCHARP(path, from) == '.')
      {
	int c3;
#if COMBINE_PATH_DEBUG > 0
	/* s->s->str[s->s->len]=0; */
	fprintf(stderr, "combine_path(0),   TO: \"%s\"[%d]\n",
		s->s->str, s->s->len);
	fprintf(stderr, "combine_path(0), FROM (%d): \"%s\"[%d]\n",
		from, path.ptr+from, len - from);
#endif

	switch(INDEX_PCHARP(path, from+1))
	{
	  case '.':
	    c3=INDEX_PCHARP(path, from+2);
	    if(s->s->len && (IS_SEP(c3) || !c3))
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
	      else {
		if (tmp+1 < s->s->len && index_shared_string(s->s,tmp)=='.') {
		  if ((index_shared_string(s->s,tmp+1)=='.') &&
		      ( (tmp+2 == s->s->len) ||
			IS_SEP(index_shared_string(s->s,tmp+2)))) {
		    break;
		  }
		}
	      }

	      from+=(c3? 3:2);
	      s->s->len=tmp;
	      s->known_shift=0;

#if COMBINE_PATH_DEBUG > 0
	      /* s->s->str[s->s->len]=0; */
	      fprintf(stderr,"combine_path(1),   TO: %s[%d]\n",
		      s->s->str, s->s->len);
	      fprintf(stderr,"combine_path(1), FROM (%d): %s[%d]\n",
		      from, path.ptr+from, len-from);
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

  check_all_args(DEFINETOSTR(COMBINE_PATH), args,
		 BIT_STRING, BIT_STRING | BIT_MANY | BIT_VOID, 0);

  init_string_builder(&ret, 0);
  SET_ONERROR(tmp, free_string_builder, &ret);

#if COMBINE_PATH_DEBUG > 0
  for(e=0; e < args; e++) {
    fprintf(stderr, "combine_path(), Arg #%d: \"%s\"[%d bytes]\n",
	    e, Pike_sp[e-args].u.string->str, Pike_sp[e-args].u.string->len);
  }
#endif
#if COMBINE_PATH_DEBUG > 5
  fprintf(stderr, "combine_path(),  Empty: \"%s\"[%d bytes]\n",
	  ret.s->str, ret.s->len);
  fprintf(stderr, "combine_path(),  Empty+1: \"%s\"\n",
	  ret.s->str+1);
#endif

  for(e=args-1;e>root;e--)
  {
    if(IS_ABS(MKPCHARP_STR(Pike_sp[e-args].u.string),
	      Pike_sp[e-args].u.string->len))
    {
      root=e;
      break;
    }
  }

  APPEND_PATH(&ret,
	      MKPCHARP_STR(Pike_sp[root-args].u.string),
	      Pike_sp[root-args].u.string->len);
  root++;

#if COMBINE_PATH_DEBUG > 1
  fprintf(stderr, "combine_path(),  ret: \"%s\"[%d] (root: %d)\n",
	  ret.s->str, ret.s->len, root);
#endif

#ifdef IS_ROOT
  for(e=args-1;e>root;e--)
  {
    if (TYPEOF(Pike_sp[e-args]) != T_STRING) continue;
    if(IS_ROOT(MKPCHARP_STR(Pike_sp[e-args].u.string),
	       Pike_sp[e-args].u.string->len))
    {
      root=e;
      break;
    }
  }
#if COMBINE_PATH_DEBUG > 1
  fprintf(stderr, "combine_path(),  root: %d\n", root);
#endif
#endif

  while(root<args)
  {
    if (TYPEOF(Pike_sp[root-args]) != T_STRING) continue;
#if COMBINE_PATH_DEBUG > 0
    fprintf(stderr, "combine_path(), Appending \"%s\"[%d] root: %d\n",
	    Pike_sp[root-args].u.string->str,
	    Pike_sp[root-args].u.string->len,
	    root);
#endif
    APPEND_PATH(&ret,
		MKPCHARP_STR(Pike_sp[root-args].u.string),
		Pike_sp[root-args].u.string->len);
    root++;
  }
  UNSET_ONERROR(tmp);
  pop_n_elems(args);
  push_string(finish_string_builder(&ret));
}


#undef COMBINE_PATH_DEBUG
#undef UNIX_COMBINE_PATH
#undef NT_COMBINE_PATH
#undef AMIGAOS_COMBINE_PATH
