#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

mixed map(mixed arr, mixed fun, mixed ... args)
{
  int e;
  mixed *ret;

  if(mappingp(arr))
    return mkmapping(indices(arr),map(values(arr),fun,@args));

  if(intp(fun))
    return arr(@args);
  
  if(stringp(fun))
    return column(arr, fun)(@args);

  if(functionp(fun))
  {
    ret=allocate(sizeof(arr));
    for(e=0;e<sizeof(arr);e++)
      ret[e]=fun(arr[e],@args);
    return ret;
  }

  error("Bad argument 2 to map_array().\n");
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

    return ret;
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

varargs mixed *sort_array(array foo,function cmp, mixed ... args)
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
    reverse(foo);
    return foo;
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


void create()
{
  add_constant("filter",filter);
  add_constant("map",map);
  add_constant("sum_arrays",sum_arrays);
  add_constant("search_array",search_array);
  add_constant("sort_array",sort_array);
  add_constant("uniq",uniq);
}
