#!/usr/local/bin/pike

/* $Id$ */

void verify();

mapping fc=([]);

int cnt;

#define FUN(X) void X() \
{ \
  if(!(--fc[X] && (++cnt & 15))) { verify(); werror("."); } \
}

FUN(f0)
FUN(f1)
FUN(f2)
FUN(f3)
FUN(f4)
FUN(f5)
FUN(f6)
FUN(f7)
FUN(f8)
FUN(f9)


array fn=({f0,f1,f2,f3,f4,f5,f6,f7,f8,f9});

void verify()
{
  mapping ff=([]);
  foreach(call_out_info(), mixed f) ff[f[2]]++;
  foreach(fn, mixed f)
  {
    if(fc[f] != ff[f])
    {
      werror("Incorrect number of call outs!\n");
      werror(sprintf("%O != %O\n",fc,ff));
      exit(1);
    }
  }
  if(!sizeof(ff)) { werror("\n"); exit(0); }
  gc();
}

mixed co(mixed func, mixed ... args)
{
  mixed ret;
  fc[func]++;
  ret=call_out(func,@args);
  return ret;
}

mixed rco(mixed func)
{
  mixed ret;
  if(zero_type(ret=remove_call_out(func))!=1)
  {
    if(arrayp(func))
    {
      fc[func[0]]--;
    }else{
      fc[func]--;
    }
  }
  return ret;
}

void do_remove()
{
  fc[do_remove]--;
  werror("\nRemoving call outs ");
  for(int d=0;d<50;d++)
  {
    for(int e=0;e<200;e++)
      rco(fn[random(10)]);
    
    verify();
    werror(".");
  }
  werror("\nWaiting to exit ");
  call_out(exit,30,1);
}

int main()
{
  random_seed(0);
  write("\nCreating call outs ");
  for(int d=0;d<50;d++)
  {
    for(int e=0;e<100;e++)
    {
      co(fn[random(10)],random(1000)/100.0);
    }
    werror(".");
    verify();
  }

  werror("\nTesting end of heap ...");

  verify();

  array  tmp=allocate(10000);
  for(int e=0;e<sizeof(tmp);e++) tmp[e]=co(f0,50.0);

  verify();

  for(int e=0;e<sizeof(tmp);e++)
  {
    if(zero_type(rco(tmp[e]))==1)
    {
      werror("Remove call out failed!!!\n");
      exit(1);
    }
  }

  werror("\nTesting beginning of heap ...");

  verify();

  for(int e=0;e<sizeof(tmp);e++) tmp[e]=co(f0,-50.0);

  verify();

  for(int e=0;e<sizeof(tmp);e++)
  {
    if(zero_type(rco(tmp[e]))==1)
    {
      werror("Remove call out failed!!!\n");
      exit(1);
    }
  }

  verify();
  
  werror("\nWaiting ");

  co(do_remove,1.0);
  
  return -17;
}
