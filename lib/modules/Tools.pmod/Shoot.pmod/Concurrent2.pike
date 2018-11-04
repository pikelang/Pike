#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant name="Concurrent indirect success";

int perform()
{
  return 1;
  Concurrent.Promise p = Concurrent.Promise();
  p->then() { };
  call_out(lambda() { p->success(1); }, 0);
  p->get();
  return 1;
}
