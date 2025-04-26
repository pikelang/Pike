#charset iso-8859-1
#pike __REAL_VERSION__

#pragma strict_types

//! General functions to operate on arrays.

constant diff = __builtin.diff;
constant diff_longest_sequence = __builtin.diff_longest_sequence;
constant diff_compare_table = __builtin.diff_compare_table;
constant longest_ordered_sequence = __builtin.longest_ordered_sequence;
constant interleave_array = __builtin.interleave_array;

constant diff_dyn_longest_sequence = __builtin.diff_dyn_longest_sequence;

constant sort = predef::sort;
constant everynth = __builtin.everynth;
constant splice = __builtin.splice;
constant transpose = __builtin.transpose;
constant uniq = __builtin.uniq_array;

constant filter=predef::filter;
constant map=predef::map;
constant permute = __builtin.permute;
constant enumerate = predef::enumerate;
constant Iterator = __builtin.array_iterator;

//! @[reduce()] sends the first two elements in @[arr] to @[fun],
//! then the result and the next element in @[arr] to @[fun] and
//! so on. Then it returns the result. The function will return
//! @[zero] if @[arr] is the empty array. If @[arr] has
//! only one element, that element will be returned.
//!
//! @seealso
//!   @[rreduce()]
//!
mixed reduce(function fun, array arr, mixed|void zero)
{
  if(sizeof(arr))
    zero = arr[0];
  for(int i=1; i<sizeof(arr); i++)
    zero = ([function(mixed,mixed:mixed)]fun)(zero, arr[i]);
  return zero;
}

//! @[rreduce()] sends the last two elements in @[arr] to @[fun],
//! then the third last element in @[arr] and the result to @[fun] and
//! so on. Then it returns the result. The function will return
//! @[zero] if @[arr] is the empty array. If @[arr] has
//! only one element, that element will be returned.
//!
//! @seealso
//!   @[reduce()]
//!
mixed rreduce(function fun, array arr, mixed|void zero)
{
  if(sizeof(arr))
    zero = arr[-1];
  for(int i=sizeof(arr)-2; i>=0; --i)
    zero = ([function(mixed,mixed:mixed)]fun)(arr[i], zero);
  return zero;
}

//! @[shuffle()] gives back the same elements, but in random order.
//! The array is modified destructively.
//!
//! @seealso
//!   @[permute()]
//!
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
  return arr;
}

//! Returns an array of all combinations of length @[len] of
//! elements from @[arr].
//!
//! @seealso
//!   @[permute()]
array(array) combinations(array arr, int(0..) len)
{
  if (len > sizeof(arr)) return ({});
  if (len == sizeof(arr)) return ({arr+({})});
  if (!len) return ({({})});
  if (len < 0) error("Negative length.\n");
  array(int) stack = allocate(len+1);
  array selection = allocate(len);
  array(array) res = allocate(Math.choose(sizeof(arr), len));
  int depth;
  int pos;
  stack[0] = -1;
  for (pos = 0; pos < sizeof(res); pos++) {
    selection[depth] = arr[stack[depth+1]];
    for(depth++;depth < len; depth++) {
      selection[depth] = arr[stack[depth+1] = stack[depth]+1];
    }
    res[pos] = selection + ({});
    do {
      stack[depth--]++;
    } while (depth && (stack[depth+1]+len == sizeof(arr)+depth+1));
  }
  return res;
}

//! @[search_array()] works like @[map()], only it returns the index
//! of the first call that returnes true instead.
//!
//! If no call returns true, -1 is returned.
//!
//! @seealso
//!   @[sum()], @[map()]
//!
int search_array(array arr, string|function|int fun, mixed ... args)
{
  int e;

  if(stringp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(([function(mixed...:mixed)]([array(object)]arr)[e][fun])(@args))
	return e;
    return -1;
  }
  else if(functionp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(([function(mixed,mixed...:mixed)]fun)(arr[e],@args))
	return e;
    return -1;
  }
  else if(intp(fun))
  {
    for(e=0;e<sizeof(arr);e++)
      if(([array(function(mixed...:mixed))]arr)[e](@args))
	return e;
    return -1;
  }

  error("Bad argument 2 to search_array().\n");
}

//! Applies the function @[sum] columnwise on the elements in the
//! provided arrays. E.g. @expr{sum_arrays(`+,a,b,c)@} does the same
//! as @expr{`+(a[*],b[*],c[*])@}.
array sum_arrays(function(int(0..0) ...:mixed) sum, array ... args)
{
  // FIXME: int(0..0) in the function prototype above is a kludge.
  // See the FIXME in sort_array.
  array ret = allocate(sizeof(args[0]));
  for(int e=0; e<sizeof(args[0]); e++)
    ret[e] = sum( @column(args, e) );
  return ret;
}

int(0..1) is_sorted(array arr,
                    function(__unknown__,__unknown__,__unknown__ ...:int)|void cmp,
                    mixed ... args)
{
  if (sizeof(arr) < 2) return 1;

  int i = sizeof(arr)-1;
  mixed n = arr[i];
  while (i--) {
    mixed p = arr[i];
    if (!([function(mixed,mixed,mixed...:int)]cmp)(n, p, @args)) return 0;
    n = p;
  }

  return 1;
}

//! @decl array sort_array(array arr, function|void cmp, mixed ... args)
//!
//! This function sorts the array @[arr] after a compare-function
//! @[cmp] which takes two arguments and should return @expr{1@} if the
//! first argument is larger then the second. Returns the sorted array
//! - @[arr] is not sorted destructively.
//!
//! The remaining arguments @[args] will be sent as 3rd, 4th etc. argument
//! to @[cmp].
//!
//! If @[cmp] is omitted, @[`>()] is used instead.
//!
//! @seealso
//! @[map()], @[sort()], @[`>()], @[dwim_sort_func], @[lyskom_sort_func],
//! @[oid_sort_func]
//!
array sort_array(array arr, function(__unknown__, __unknown__, __unknown__ ...:int)|void cmp,
                 mixed ... args)
{
  // FIXME: The two int(0..0) in the function prototype above are
  // kludges to avoid strict_type warnings on correctly typed cmp
  // functions. The correct way to fix it would be to infer the real
  // type from the array elements in arr.

  int len,start;
  int(0..) length;
  int foop, fooend, barp, barend;

  arr+=({});

  if(!cmp || cmp==`>)
  {
    sort(arr);

    cmp = `>;
  } else if(cmp == `<)
  {
    sort(arr);
    arr = reverse(arr);
  }

  // Short-cut if the array is already sorted or got properly
  // sorted by predef::sort() above. Note that predef::sort()
  // does NOT interleave values of different types (eg ints
  // and floats).
  if (is_sorted(arr, cmp, @args)) {
    return arr;
  }

  // Bottom-up Merge Sort.
  length=sizeof(arr);

  array bar = allocate(length);

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
	if(([function(mixed,mixed,mixed...:int)]cmp)(arr[foop],arr[barp],@args)
	    <= 0)
	{
	  bar[start++]=arr[foop++];
	  if(foop == fooend)
	  {
	    while(barp < barend) bar[start++]=arr[barp++];
	    break;
	  }
	}else{
	  bar[start++]=arr[barp++];
	  if(barp == barend)
	  {
	    while(foop < fooend) bar[start++]=arr[foop++];
	    break;
	  }
	}
      }
    }
    while(start < length) bar[start]=arr[start++];

    array tmp = arr;
    arr=bar;
    bar=tmp;
  }

  return arr;
}

//! Get multiple columns from an array.
//!
//! This function is equivalent to
//! @pre{
//!   map(ind, lambda(mixed i) { return column(x, i); })
//! @}
//!
//! @seealso
//!   @[column()]
array(array) columns(array x, array ind)
{
  array(array) ret=allocate(sizeof(ind));
  for(int e=0;e<sizeof(ind);e++) ret[e]=column(x,ind[e]);
  return ret;
}

// diff3, complement to diff

//! Return the three-way difference between the arrays.
//!
//! @seealso
//!   @[Array.diff()], @[Array.diff_longest_sequence()]
array(array(array)) diff3 (array a, array b, array c)
{
  // This does not necessarily produce the optimal sequence between
  // all three arrays. A diff_longest_sequence() that takes any number
  // of arrays would be nice.
  array(int) seq_ab = diff_longest_sequence (a, b);
  array(int) seq_bc = diff_longest_sequence (b, c);
  array(int) seq_ca = diff_longest_sequence (c, a);

  array(int) aeq = allocate (sizeof (a) + 1);
  array(int) beq = allocate (sizeof (b) + 1);
  array(int) ceq = allocate (sizeof (c) + 1);
  aeq[sizeof (a)] = beq[sizeof (b)] = ceq[sizeof (c)] = 7;

  for (int i = 0, j = 0; j < sizeof (seq_ab); i++)
    if (a[i] == b[seq_ab[j]]) aeq[i] |= 2, beq[seq_ab[j]] |= 1, j++;
  for (int i = 0, j = 0; j < sizeof (seq_bc); i++)
    if (b[i] == c[seq_bc[j]]) beq[i] |= 2, ceq[seq_bc[j]] |= 1, j++;
  for (int i = 0, j = 0; j < sizeof (seq_ca); i++)
    if (c[i] == a[seq_ca[j]]) ceq[i] |= 2, aeq[seq_ca[j]] |= 1, j++;

  //werror ("%O\n", ({aeq, beq, ceq}));

  array(array) ares = ({}), bres = ({}), cres = ({});
  int ai = 0, bi = 0, ci = 0;
  int prevodd = -2;

  while (!(aeq[ai] & beq[bi] & ceq[ci] & 4)) {
    //werror ("aeq[%d]=%d beq[%d]=%d ceq[%d]=%d prevodd=%d\n",
    //    ai, aeq[ai], bi, beq[bi], ci, ceq[ci], prevodd);
    array empty = ({}), apart = empty, bpart = empty, cpart = empty;
    int side = aeq[ai] & beq[bi] & ceq[ci];

    if ((<1, 2>)[side]) {
      // Got cyclically interlocking equivalences. Have to break one
      // of them. Prefer the shortest.
      int which, merge, inv_side = side ^ 3, i, oi;
      array(int)|zero eq, oeq;
      array|zero arr, oarr;
      int atest = side == 1 ? ceq[ci] != 3 : beq[bi] != 3;
      int btest = side == 1 ? aeq[ai] != 3 : ceq[ci] != 3;
      int ctest = side == 1 ? beq[bi] != 3 : aeq[ai] != 3;

      for (i = 0;; i++) {
	int abreak = atest && aeq[ai] != aeq[ai + i];
	int bbreak = btest && beq[bi] != beq[bi + i];
	int cbreak = ctest && ceq[ci] != ceq[ci + i];

	if (abreak + bbreak + cbreak > 1) {
	  // More than one shortest sequence. Avoid breaking one that
	  // could give an all-three match later.
	  if (side == 1) {
	    if (!atest) cbreak = 0;
	    if (!btest) abreak = 0;
	    if (!ctest) bbreak = 0;
	  }
	  else {
	    if (!atest) bbreak = 0;
	    if (!btest) cbreak = 0;
	    if (!ctest) abreak = 0;
	  }
	  // Prefer breaking one that can be joined with the previous
	  // diff part.
	  switch (prevodd) {
	    case 0: if (abreak) bbreak = cbreak = 0; break;
	    case 1: if (bbreak) cbreak = abreak = 0; break;
	    case 2: if (cbreak) abreak = bbreak = 0; break;
	  }
	}

	if (abreak) {
	  which = 0, merge = (<0, -1>)[prevodd];
	  i = ai, eq = aeq, arr = a;
	  if (inv_side == 1) oi = bi, oeq = beq, oarr = b;
	  else oi = ci, oeq = ceq, oarr = c;
	  break;
	}
	if (bbreak) {
	  which = 1, merge = (<1, -1>)[prevodd];
	  i = bi, eq = beq, arr = b;
	  if (inv_side == 1) oi = ci, oeq = ceq, oarr = c;
	  else oi = ai, oeq = aeq, oarr = a;
	  break;
	}
	if (cbreak) {
	  which = 2, merge = (<2, -1>)[prevodd];
	  i = ci, eq = ceq, arr = c;
	  if (inv_side == 1) oi = ai, oeq = aeq, oarr = a;
	  else oi = bi, oeq = beq, oarr = b;
	  break;
	}
      }
      //werror ("  which=%d merge=%d inv_side=%d i=%d oi=%d\n",
      //      which, merge, inv_side, i, oi);

      int s = i, mask = eq[i];
      do {
	eq[i++] &= inv_side;
	while (!(oeq[oi] & inv_side)) oi++;
	oeq[oi] &= side;
      }
      while (eq[i] == mask);

      if (merge && !eq[s]) {
	array part = ({});
	do part += ({arr[s++]}); while (!eq[s]);
	switch (which) {
	  case 0: ai = s; ares[-1] += part; break;
	  case 1: bi = s; bres[-1] += part; break;
	  case 2: ci = s; cres[-1] += part; break;
	}
      }
    }
    //werror ("aeq[%d]=%d beq[%d]=%d ceq[%d]=%d prevodd=%d\n",
    //    ai, aeq[ai], bi, beq[bi], ci, ceq[ci], prevodd);

    if (aeq[ai] == 2 && beq[bi] == 1) { // a and b are equal.
      do apart += ({a[ai++]}), bi++; while (aeq[ai] == 2 && beq[bi] == 1);
      bpart = apart;
      while (!ceq[ci]) cpart += ({c[ci++]});
      prevodd = 2;
    }
    else if (beq[bi] == 2 && ceq[ci] == 1) { // b and c are equal.
      do bpart += ({b[bi++]}), ci++; while (beq[bi] == 2 && ceq[ci] == 1);
      cpart = bpart;
      while (!aeq[ai]) apart += ({a[ai++]});
      prevodd = 0;
    }
    else if (ceq[ci] == 2 && aeq[ai] == 1) { // c and a are equal.
      do cpart += ({c[ci++]}), ai++; while (ceq[ci] == 2 && aeq[ai] == 1);
      apart = cpart;
      while (!beq[bi]) bpart += ({b[bi++]});
      prevodd = 1;
    }

    else if ((<1*2*3, 3*3*3>)[aeq[ai] * beq[bi] * ceq[ci]]) { // All are equal.
      // Got to match both when all three are 3 and when they are 1, 2
      // and 3 in that order modulo rotation (might get such sequences
      // after breaking the cyclic equivalences above).
      do apart += ({a[ai++]}), bi++, ci++;
      while ((<0333, 0123, 0312, 0231>)[aeq[ai] << 6 | beq[bi] << 3 | ceq[ci]]);
      cpart = bpart = apart;
      prevodd = -2;
    }

    else {
      // Haven't got any equivalences in this block. Avoid adjacent
      // complementary blocks (e.g. ({({"foo"}),({}),({})}) next to
      // ({({}),({"bar"}),({"bar"})})). Besides that, leave the
      // odd-one-out sequence empty in a block where two are equal.
      switch (prevodd) {
	case 0: apart = ares[-1], ares[-1] = ({}); break;
	case 1: bpart = bres[-1], bres[-1] = ({}); break;
	case 2: cpart = cres[-1], cres[-1] = ({}); break;
      }
      prevodd = -1;
      while (!aeq[ai]) apart += ({a[ai++]});
      while (!beq[bi]) bpart += ({b[bi++]});
      while (!ceq[ci]) cpart += ({c[ci++]});
    }

    //werror ("%O\n", ({apart, bpart, cpart}));
    ares += ({apart}), bres += ({bpart}), cres += ({cpart});
  }

  return ({ares, bres, cres});
}

#if 0
array(array(array)) compact_diff3 (array a, array b, array old)
//! Given three arrays like those returned from @ref{diff3@}, this
//! function "compacts" the diff3 result by removing all differences
//! where @tt{a@} and @tt{b@} agrees against @tt{old@}. The result is
//! on the same form as the result from @ref{diff@}, and doesn't
//! include the sequence from @tt{old@}.
{
//   a = a + ({}), b = b + ({});

//   if (sizeof (a) && a[0] == b[0] && !sizeof (a[0]))
//     a[0] = b[0] = 0;
//   int prev = 0;
//   for (int i = 1; i < sizeof (a); i++)
//     if (a[i] == b[i])
//       if (!sizeof (a[i])) {
// 	a[i] = b[i] = 0;
//       }
//       else if (prev != i - 1) {
// 	int joined = 0;
// 	if (!sizeof (a[i])) {
// 	  if (!sizeof (a[prev])) b[prev] +=
// 	}
//       }
}
#endif

//! Sort strings containing numbers with respect to their values
//! rather than according to their formatting (this most notably
//! causes leading zeroes to be ignored/unnecessary).
//!
//! @example
//!   "foo7" will be sorted before "foo27", which will be before
//!   "foo100".
int(-1..1) dwim_sort_func(string a, string b)
{
  if( a==b ) return 0;

  while(1)
  {
    sscanf(a, "%[0-9]%[^0-9]%s", string a_int, string a_str, a);
    sscanf(b, "%[0-9]%[^0-9]%s", string b_int, string b_str, b);

    // Need only be done first iteration
    if( !sizeof(a_int) ^ !sizeof(b_int) )
      return sizeof(a_int) ? -1 : 1;

    if( a_int != b_int )
    {
      int ai = (int)a_int;
      int bi = (int)b_int;
      if( ai!=bi )
        return ai<bi ? -1 : 1;
    }

    if( a_str != b_str )
      return a_str<b_str ? -1 : 1;

    if( !sizeof(a) || !sizeof(b) )
    {
      if( sizeof(a) ) return 1;
      if( sizeof(b) ) return -1;
      return 0;
    }
  }
}

//! Sort comparison function that does not care about case, nor about
//! the contents of any parts of the string enclosed with '()'
//!
//! Example: "Foo (bar)" is given the same weight as "foo (really!)"
int(-1..1) lyskom_sort_func(string a,string b)
{
   string a0=a,b0=b;
   a=replace(lower_case(a),"][\\}{|"/1,"������"/1);
   b=replace(lower_case(b),"][\\}{|"/1,"������"/1);

   while (sscanf(a0=a,"%*[ \t](%*[^)])%*[ \t]%s",a)==4 && a0!=a);
   while (sscanf(b0=b,"%*[ \t](%*[^)])%*[ \t]%s",b)==4 && b0!=b);
   a0=b0="";
   sscanf(a,"%[^ \t]%*[ \t](%*[^)])%*[ \t]%s",a,a0);
   sscanf(b,"%[^ \t]%*[ \t](%*[^)])%*[ \t]%s",b,b0);
   if (a>b) return 1;
   if (a<b) return -1;
   if (a0==b0) return 0;
   return lyskom_sort_func(a0,b0);
}

//! Flatten a multi-dimensional array to a one-dimensional array.
//! @note
//!   Prior to Pike 7.5.7 it was not safe to call this function
//!   with cyclic data-structures.
array flatten(array a, mapping(array:array)|void state)
{
  if (state && state[a]) return [array]state[a];
  if (!state) state = ([a:({})]);
  else state[a] = ({});
  array res = allocate(sizeof(a));
  foreach(a; int i; mixed b) {
    res[i] = arrayp(b)?flatten([array]b, state):({b});
  }
  return state[a] = [array](res*({}));
}

//! Sum the elements of an array using `+. The empty array
//! results in 0.
mixed sum(array a)
{
  if(a==({})) return 0;
  // 1000 is a safe stack limit
   if (sizeof(a)<1000)
      return `+(@a);
   else
   {
      int|float|array|mapping|multiset|object|string mem =
	[int|float|array|mapping|multiset|object|string]`+(@a[..999]);
      int j=1000;
      array v = ({});
      while (sizeof(v=a[j..j+999])) {
	mem = [int|float|array|mapping|multiset|object|string]`+(mem,@v);
	j+=1000;
      }
      return mem;
   }
}

//! Perform the same action as the Unix uniq command on an array,
//! that is, fold consecutive occurrences of the same element into
//! a single element of the result array:
//!
//! aabbbcaababb -> abcabab.
//!
//! See also the @[uniq] function.
array uniq2(array a)
{
   if (!sizeof(a)) return ({});
   mixed last;
   array res = ({ last = a[0] });
   foreach (a,mixed v)
      if (v!=last) last=v,res+=({v});
   return res;
}

//! Make an array of the argument, if it isn't already. An undefined
//! argument gives the empty array. This is useful when something is
//! either an array or a basic datatype, for instance in headers from
//! the MIME module or Protocols.HTTP.Server.
//! @param x
//!   Result depends of the argument type:
//!   @dl
//!     @item arrayp(x)
//!       arrayify(x) => x
//!     @item undefinedp(x)
//!       arrayify(x) => ({})
//!     @item otherwise
//!       arrayify(x) => ({ x })
//!   @enddl
array arrayify(void|array|mixed x)
{
   if(undefinedp(x)) return ({});
   if(arrayp(x)) return [array]x;
   return ({ x });
}

//! Sort with care of numerical sort for OID values, e.g.
//! "1.2.1" before "1.11.1".
//! @returns
//!   @int
//!     @value -1
//!       @expr{a<b@}
//!     @value 0
//!       @expr{a==b@}
//!     @value 1
//!       @expr{a>b@}
//!   @endint
//! @note
//!   In Pike 7.6 and older this function returned @expr{0@} both when
//!   @expr{a<b@} and @expr{a==b@}.
//! @seealso
//!   @[sort_array]
int(-1..1) oid_sort_func(string a, string b)
{
    int a1, b1;
    sscanf(a, "%d.%[0-9.]", a1, string a_rest);
    sscanf(b, "%d.%[0-9.]", b1, string b_rest);
    if (a1>b1) return 1;
    if (a1<b1) return -1;
    if (!a_rest || a_rest == "") a_rest = "0";
    if (!b_rest || b_rest == "") b_rest = "0";
    if (a_rest == b_rest) return 0;
    return oid_sort_func(a_rest, b_rest);
}

protected array(array(array)) low_greedy_diff(array(array) d1, array(array) d2)
{
  array(array) r1 = ({}), r2 = ({});
  int at, last, seen;
  while(-1 != (at = search(d1, ({}), last)))
  {
    last = at + 1;
    if(at < 2) continue;
    array b = d2[at-1];
    array yb = d2[at];
out:if(sizeof(yb) > sizeof(b))
    {
      int i = sizeof(b), j = sizeof(yb);
      while(i)
	if(b[--i] != yb[--j])
	  break out; // past five lines implement an if(has_suffix(yb, b))
      array x = d2[at-2];
      array y = yb[..sizeof(yb)-sizeof(b)-1];
      if(at+1 <= sizeof(d1))
      {
	array c = d2[at+1];
	array bc = b+c;
	r1 += d1[seen..at-2] + ({ bc });
	r2 += d2[seen..at-3] + ({ x+b+y }) + ({ bc });
      }
      else
      {
	// At last chunk. There is no C.
	r1 += d1[seen..at-2] + ({ b });
	r2 += d2[seen..at-3] + ({ x+b+y }) + ({ b });
      }
      seen = at + 5;
    }
  }
  if(!seen)
    return ({ d1, d2 });	// No change.
  return ({ r1 + d1[seen..], r2 + d2[seen..] });
}

//! Like @[Array.diff], but tries to generate bigger continuous chunks of the
//! differences, instead of maximizing the number of difference chunks. More
//! specifically, @[greedy_diff] optimizes the cases where @[Array.diff] returns
//! @expr{({ ..., A, Z, B, ({}), C, ... })@}
//! @expr{({ ..., A, X, B,  Y+B, C, ... })@}
//! into the somewhat shorter diff arrays
//! @expr{({ ..., A, Z,     B+C, ... })@}
//! @expr{({ ..., A, X+B+Y, B+C, ... })@}
array(array(array)) greedy_diff(array from, array to)
{
  [array(array) d1, array(array) d2] = diff(from, to);
  [d2, d1] = low_greedy_diff(d2, d1);
  return low_greedy_diff(d1, d2);
}

//! @decl int count(array|mapping|multiset haystack, mixed needle)
//! @decl mapping(mixed:int) count(array|mapping|multiset haystack)
//!   Returns the number of occurrences of @[needle] in @[haystack].
//!   If the optional @[needle] argument is omitted, @[count] instead
//!   works similar to the unix command @tt{sort|uniq -c@}, returning
//!   a mapping with the number of occurrences of each element in
//!   @[haystack]. For array or mapping @[haystack]s, it's the values
//!   that are counted, for multisets the indices, as you'd expect.
//! @seealso
//!   @[String.count], @[search], @[has_value]
int|mapping(mixed:int) count(array|mapping|multiset haystack,
			     mixed|void needle)
{
  if(undefinedp(needle))
  {
    mapping(mixed:int) res = ([]);
    if(mappingp(haystack))
      haystack = values([mapping]haystack);
    foreach((array)haystack, mixed what)
      res[what]++;
    return res;
  }
  return sizeof(filter(haystack, `==, needle));
}

//! Find the longest common prefix from an array of arrays.
//! @seealso
//!   @[String.common_prefix]
array common_prefix(array(array) arrs)
{
  if(!sizeof(arrs))
    return ({});

  array arrs0 = arrs[0];
  int n, i;

  catch
  {
    for(n = 0; n < sizeof(arrs0); n++)
      for(i = 1; i < sizeof(arrs); i++)
	if(!equal(arrs[i][n],arrs0[n]))
	  return arrs0[0..n-1];
  };

  return arrs0[0..n-1];
}

//! Returns 1 if all of the elements in @[a] fulfills the requirement
//! @[predicate]( @[a][@i{i@}], @@@[extra_args] ), otherwise 0. The
//! predicate should return non-zero for an element that meets the
//! requirements and zero for those that do not.
//! @example
//!   Array.all( ({ 2, 4, 6, 8 }), `<, 17 )
//! @seealso
//!   @[any], @[has_value]
int(0..1) all( array a, function(int(0..0), mixed ...:mixed) predicate,
	       mixed ... extra_args )
{
  // FIXME: int(0..0) in the function prototype above is a kludge.
  // See the FIXME in sort_array.
  foreach( a, mixed elem )
    if( !predicate( [int(0..0)] elem, @extra_args ) )
      return 0;
  return 1;
}

//! Returns 1 if any of the elements in @[a] fulfills the requirement
//! @[predicate]( @[a][@i{i@}], @@@[extra_args] ), otherwise 0. The
//! predicate should return non-zero for an element that meets the
//! requirements and zero for those that do not.
//! @example
//!   Array.any( ({ 2, 4, 6, 8 }), `>, 5 )
//! @seealso
//!   @[all], @[has_value]
int(0..1) any( array a, function(__unknown__, __unknown__ ...:mixed) predicate,
	       mixed ... extra_args )
{
  // FIXME: int(0..0) in the function prototype above is a kludge.
  // See the FIXME in sort_array.
  foreach( a, mixed elem )
    if( ([function(mixed...:mixed)]predicate)( elem, @extra_args ) )
      return 1;
  return 0;
}

//! Splits an array in two, according to an arbitration function
//! @[arbiter]. The elements in @[a] who return non-zero for the
//! expression @[arbiter]( @[a][@i{i@}], @@@[extra_args] ) end up in
//! the first sub-array, the others in the second. The order is
//! preserved from the original array.
//! @example
//!   Array.partition( enumerate( 9 ), lambda(int n) { return n>3 && n<7; } );
//!   > ({ ({ 4, 5, 6 }), ({ 0, 1, 2, 3, 7, 8 }) })
//! @seealso
//!   @[filter], @[`/], @[`%]
array(array) partition( array a, function(int(0..0), mixed ...:mixed) arbiter,
			mixed ... extra_args )
{
  // FIXME: int(0..0) in the function prototype above is a kludge.
  // See the FIXME in sort_array.
  array first = ({}), second = ({});
  foreach( a, mixed elem )
    if( arbiter( [int(0..0)] elem, @extra_args ) )
      first += ({ elem });
    else
      second += ({ elem });
  return ({ first, second });
}

//! Threats an Array as a stack and pushes the element onto the
//! end.
//! @example
//!   Array.push(({ "a", "b", "c", "d" }), "e");
//!   > ({ "a", "b", "c", "d", "e" })
//! @seealso
//!   @[ADT.Stack], @[ADT.Stack.push]
array push(array list, mixed element) {
  return list + ({ element });
}

//! Pops and returns the last value of the array, shortening the
//! array by one element.
//! If there are no elements in the array then 0 is returned otherwise
//! an array is returned where the first returned element is the popped
//! value, and the second element is the modified array.
//! @example
//!   Array.pop(({ "a", "b", "c", "d" }));
//!   > ({ "d", ({ "a", "b", "c" }) })
//! @seealso
//!   @[ADT.Stack], @[ADT.Stack.pop], @[ADT.Stack.quick_pop]
array pop(array list) {
  if (sizeof(list) == 1)
    return ({ list[0], ({}) });
  else if (sizeof(list) > 1) {
    mixed elem = list[sizeof(list)-1];
    list = list[..<1];
    return ({ elem, list });
  }
}

//! Shifts the first value of the array off and returns it, shortening
//! the array by 1 and moving everything down.  If there are no elements
//! in the array it returns 0.
//! Returns an array where the first element is the shifted value and the
//! second element is the modified array.
//! @example
//!   Array.shift(({ "a", "b", "c", "d"}));
//!   > ({ "a", ({ "b", "c", "d" }) })
//! @seealso
//!   @[ADT.Stack]
array|zero shift(array list) {
  if (sizeof(list))
    return ({ list[0], list[1..] });
  else
    return 0;
}

//! Does the opposite of "shift".  Or the opposite of a "push",
//! depending on how you look at it.  Prepends the element to
//! the front of the array and returns the new array.
//! @example
//!   Array.unshift(({ "b", "c", "d" }), "a");
//!   > ({ "a", "b", "c", "d" })
//! @seealso
//!   @[ADT.Stack]
array unshift(array list, mixed element) {
  return ({ element }) + list;
}
