#pike __REAL_VERSION__
inherit Tools.Shoot.Test;

constant disabled = 1;
constant name="Concurrent indirect success";

int perform()
{
  Concurrent.Promise p = Concurrent.Promise();
  p->then() { };
  call_out(lambda() { p->success(1); }, 0);
  p->get();
  return 1;
}
