#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

constant diff = __builtin.diff;
constant diff_longest_sequence = __builtin.diff_longest_sequence;
constant diff_compare_table = __builtin.diff_compare_table;
constant longest_ordered_sequence = __builtin.longest_ordered_sequence;

constant sort = __builtin.sort;

mixed map(mixed arr, mixed fun, mixed ... args)
{
  int e;
  mixed *ret;

  if(mappingp(arr))
    return mkmapping(indices(arr),map(values(arr),fun,@args));

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
    error("Bad argument 2 to map_array().\n");
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
  }else{
    int d;
    ret=map(arr,fun,@args);
    for(e=0;e<sizeof(arr);e++) if(ret[e]) ret[d++]=arr[e];
    
    return ret[..d-1];
  }
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

array transpose(array x)
{
  array ret=allocate(sizeof(x[0]));
  for(int e=0;e<sizeof(x[0]);e++) ret[e]=column(x,e);
  return ret;
}

// diff3, complement to diff (alpha stage)

array(array(array(mixed))) diff3(array mid,array left,array right)
{
   array lmid,ldst;
   array rmid,rdst;

   [lmid,ldst]=diff(mid,left);
   [rmid,rdst]=diff(mid,right);

   werror(sprintf("%O\n",({rmid,rdst})));

   array res=({});
   int lpos=0,rpos=0;
   int l=0,r=0,n;
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
