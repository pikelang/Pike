#pike 7.0

inherit Array;

/* break too strict type handling...  :) */
array map(array x, int|string|function fun, mixed ... args)
{
  return Array.map(x,fun,@args);
}


