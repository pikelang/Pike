#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

constant diff = __builtin.diff;
constant diff_longest_sequence = __builtin.diff_longest_sequence;
constant diff_compare_table = __builtin.diff_compare_table;
constant longest_ordered_sequence = __builtin.longest_ordered_sequence;
constant interleave_array = __builtin.interleave_array;

constant sort = __builtin.sort;
constant everynth = __builtin.everynth;
constant splice = __builtin.splice;
constant transpose = __builtin.transpose;

mixed map(mixed arr, mixed fun, mixed ... args)
{
  int e;
  mixed *ret;

  if(mappingp(arr))
    return mkmapping(indices(arr),map(values(arr),fun,@args));

  if(multisetp(arr))
    return mkmultiset(map(indices(arr,fun,@args)));

  switch(sprintf("%t",fun))
  {
  case "int":
    return arr(@args);

  case "string":
    return column(arr, fun)(@args);

  case "function":
  case "program":
  case "object":
    ret=allocate(sizeof(arr));
    for(e=0;e<sizeof(arr);e++)
      ret[e]=fun(arr[e],@args);
    return ret;

  default:
    error("Bad argument 2 to Array.map().\n");
  }
}

mixed filter(mixed arr, mixed fun, mixed ... args)
{
  int e;
  mixed *ret;

  if(mappingp(arr))
  {
    mixed *i, *v, r;
    i=indices(arr);
    ret=map(v=values(arr),fun,@args);
    r=([]);
    for(e=0;e<sizeof(ret);e++) if(ret[e]) r[i[e]]=v[e];

    return r;
  }
  if(multisetp(arr))
  {
    return mkmultiset(filter(indices(arr,fun,@args)));
  }
  else
  {
    int d;
    ret=map(arr,fun,@args);
    for(e=0;e<sizeof(arr);e++) if(ret[e]) ret[d++]=arr[e];
    
    return ret[..d-1];
  }
}

array shuffle(array arr)
{
  int i = sizeof(arr);

  while(i) {
    int j = random(i--);
    if (j != i) {
      mixed tmp = arr[i];
      arr[i] = arr[j];
      arr[j] = tmp;
    }
  }
  return(arr);
}

array permute(array a,int n)
{
   int q=sizeof(a);
   int i;
   a=a[..]; // copy
   
   while (n && q)
   {
      int x=n%q; 
      n/=q; 
      q--; 
      if (x) [a[i],a[i+x]]=({ a[i+x],a[i] });
      i++;
   }  
   
   return a;
}

int search_array(mixed *arr, mixed fun, mixed ... args)
{
  int e;

  if(stringp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(arr[e][fun](@args))
	return e;
    return -1;
  }
  else if(functionp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(fun(arr[e],@args))
	return e;
    return -1;
  }
  else if(intp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(arr[e](@args))
	return e;
    return -1;
  }
  else
  {
    error("Bad argument 2 to filter().\n");
  }
}

mixed *sum_arrays(function foo, mixed * ... args)
{
  mixed *ret;
  int e,d;
  ret=allocate(sizeof(args[0]));
  for(e=0;e<sizeof(args[0]);e++)
    ret[e]=foo(@ column(args, e));
  return ret;
}

mixed *sort_array(array foo,function|void cmp, mixed ... args)
{
  array bar,tmp;
  int len,start;
  int length;
  int foop, fooend, barp, barend;

  if(!cmp || cmp==`>)
  {
    foo+=({});
    sort(foo);
    return foo;
  }

  if(cmp == `<)
  {
    foo+=({});
    sort(foo);
    return reverse(foo);
  }

  length=sizeof(foo);

  foo+=({});
  bar=allocate(length);

  for(len=1;len<length;len*=2)
  {
    start=0;
    while(start+len < length)
    {
      foop=start;
      barp=start+len;
      fooend=barp;
      barend=barp+len;
      if(barend > length) barend=length;
      
      while(1)
      {
	if(cmp(foo[foop],foo[barp],@args) <= 0)
	{
	  bar[start++]=foo[foop++];
	  if(foop == fooend)
	  {
	    while(barp < barend) bar[start++]=foo[barp++];
	    break;
	  }
	}else{
	  bar[start++]=foo[barp++];
	  if(barp == barend)
	  {
	    while(foop < fooend) bar[start++]=foo[foop++];
	    break;
	  }
	}
      }
    }
    while(start < length) bar[start]=foo[start++];

    tmp=foo;
    foo=bar;
    bar=tmp;
  }

  return foo;
}

array uniq(array a)
{
  return indices(mkmapping(a,a));
}

array columns(array x, array ind)
{
  array ret=allocate(sizeof(ind));
  for(int e=0;e<sizeof(ind);e++) ret[e]=column(x,ind[e]);
  return ret;
}

array transpose_old(array x)
{
   if (!sizeof(x)) return x;
   array ret=allocate(sizeof(x[0]));
   for(int e=0;e<sizeof(x[0]);e++) ret[e]=column(x,e);
   return ret;
}

// diff3, complement to diff

array(array(array)) diff3 (array a, array b, array c)
{
  // This does not necessarily produce the optimal sequence between
  // all three arrays. A diff_longest_sequence() that takes any number
  // of arrays would be nice.
  array(int) seq_ab = diff_longest_sequence (a, b);
  array(int) seq_bc = diff_longest_sequence (b, c);
  array(int) seq_ca = diff_longest_sequence (c, a);

  // A number bigger than any valid index servers as end of array marker.
  int eoa = max (sizeof (a), sizeof (b), sizeof (c));

  array(int) ab = allocate (sizeof (a) + 1, -1);
  array(int) ac = allocate (sizeof (a) + 1, -1);
  ab[sizeof (a)] = ac[sizeof (a)] = eoa;
  array(int) bc = allocate (sizeof (b) + 1, -1);
  array(int) ba = allocate (sizeof (b) + 1, -1);
  bc[sizeof (b)] = ba[sizeof (b)] = eoa;
  array(int) ca = allocate (sizeof (c) + 1, -1);
  array(int) cb = allocate (sizeof (c) + 1, -1);
  ca[sizeof (c)] = cb[sizeof (c)] = eoa;

  for (int i = 0, j = 0; j < sizeof (seq_ab); i++)
    if (equal (a[i], b[seq_ab[j]]))
      a[i] = b[seq_ab[j]], ab[i] = seq_ab[j], ba[seq_ab[j]] = i, j++;
  for (int i = 0, j = 0; j < sizeof (seq_bc); i++)
    if (equal (b[i], c[seq_bc[j]]))
      b[i] = c[seq_bc[j]], bc[i] = seq_bc[j], cb[seq_bc[j]] = i, j++;
  for (int i = 0, j = 0; j < sizeof (seq_ca); i++)
    if (equal (c[i], a[seq_ca[j]]))
      c[i] = a[seq_ca[j]], ca[i] = seq_ca[j], ac[seq_ca[j]] = i, j++;

  array(array) ares = ({}), bres = ({}), cres = ({});
  int ai = 0, bi = 0, ci = 0;
  int part = 8;			// Chunk partition bitfield.

  while (min (ac[ai], ab[ai], ba[bi], bc[bi], cb[ci], ca[ci]) != eoa) {
    int apart = (ac[ai] == -1 && 1) | (ab[ai] == -1 && 2);
    int bpart = (ba[bi] == -1 && 2) | (bc[bi] == -1 && 4);
    int cpart = (cb[ci] == -1 && 4) | (ca[ci] == -1 && 1);
    int newpart = apart | bpart | cpart;

    if ((apart ^ bpart ^ cpart) == 7 && !(apart & bpart & cpart) &&
	apart && bpart && cpart) {
      // Solve cyclically interlocking equivalences by arbitrary
      // breaking one of them.
      if (ac[ai] != -1) ca[ac[ai]] = -1, ac[ai] = -1;
      if (ab[ai] != -1) ba[ab[ai]] = -1, ab[ai] = -1;
      apart = 3;
    }

    if ((part & newpart) == newpart) {
      // If the previous block had the same equivalence partition or
      // was a three-part conflict, we should tack any singleton
      // equivalences we have onto it and mark them so they aren't
      // used again below.
      if (apart == 3) ares[-1] += ({a[ai++]}), apart = 8;
      if (bpart == 6) bres[-1] += ({b[bi++]}), bpart = 8;
      if (cpart == 5) cres[-1] += ({c[ci++]}), cpart = 8;
    }

    if (newpart == part) {
      // The previous block had exactly the same equivalence
      // partition, so tack anything else onto it too, but mask
      // singleton equivalences this time.
      if ((part & 3) == apart && apart != 3) ares[-1] += ({a[ai++]});
      if ((part & 6) == bpart && bpart != 6) bres[-1] += ({b[bi++]});
      if ((part & 5) == cpart && cpart != 5) cres[-1] += ({c[ci++]});
    }
    else {
      // Start a new block. Wait with singleton equivalences (this may
      // cause an extra iteration, but the necessary conditions to
      // prevent that are tricky).
      part = newpart;
      ares += ({(part & 3) == apart && apart != 3 ? ({a[ai++]}) : ({})});
      bres += ({(part & 6) == bpart && bpart != 6 ? ({b[bi++]}) : ({})});
      cres += ({(part & 5) == cpart && cpart != 5 ? ({c[ci++]}) : ({})});
    }
  }

  return ({ares, bres, cres});
}

// diff3, complement to diff (alpha stage)

array(array(array(mixed))) diff3_old(array mid,array left,array right)
{
   array lmid,ldst;
   array rmid,rdst;

   [lmid,ldst]=diff(mid,left);
   [rmid,rdst]=diff(mid,right);

   int l=0,r=0,n;
   array res=({});
   int lpos=0,rpos=0;
   array eq=({});
   int x;

   for (n=0; ;)
   {
      while (l<sizeof(lmid) && lpos>=sizeof(lmid[l]))
      {
	 if (sizeof(ldst[l])>lpos)
	    res+=({({({}),ldst[l][lpos..],({})})});
	 l++;
	 lpos=0;
      }
      while (r<sizeof(rmid) && rpos>=sizeof(rmid[r])) 
      {
	 if (sizeof(rdst[r])>rpos)
	    res+=({({({}),({}),rdst[r][rpos..]})});
	 r++;
	 rpos=0;
      }

      if (n==sizeof(mid)) break;

      x=min(sizeof(lmid[l])-lpos,sizeof(rmid[r])-rpos);

      if (lmid[l]==ldst[l] && rmid[r]==rdst[r])
      {
	 eq=lmid[l][lpos..lpos+x-1];
	 res+=({({eq,eq,eq})});
      }
      else if (lmid[l]==ldst[l])
      {
	 eq=lmid[l][lpos..lpos+x-1];
	 res+=({({eq,eq,rdst[r][rpos..rpos+x-1]})});
      }
      else if (rmid[r]==rdst[r])
      {
	 eq=rmid[r][rpos..rpos+x-1];
	 res+=({({eq,ldst[l][lpos..lpos+x-1],eq})});
      }
      else 
      {
	 res+=({({lmid[l][lpos..lpos+x-1],
		  ldst[l][lpos..lpos+x-1],
		  rdst[r][rpos..rpos+x-1]})});
      }

//        werror(sprintf("-> %-5{%O%} %-5{%O%} %-5{%O%}"
//  		     " x=%d l=%d:%d r=%d:%d \n",@res[-1],x,l,lpos,r,rpos));
	 
      rpos+=x;
      lpos+=x;
      n+=x;
   }
   
   return transpose(res);
}
