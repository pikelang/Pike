#pike 7.0

inherit Array;

//! Much simplified type compared to later versions of map.
array map(array x, int|string|function fun, mixed ... args)
{
  return Array.map(x,fun,@args);
}


