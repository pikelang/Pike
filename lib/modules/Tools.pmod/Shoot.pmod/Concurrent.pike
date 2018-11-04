#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Concurrent instant success";

int perform()
{
  Concurrent.Promise p = Concurrent.Promise();
  p->then() { };
  p->success(1);
  return 1;
}
