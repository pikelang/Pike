#pike __REAL_VERSION__

// Moahahahah!
// $Id: error.pmod,v 1.5 2001/03/23 18:36:27 mast Exp $
void `()(string f, mixed ... args)
{
  array(array) b = backtrace();
  if (sizeof(args)) f = sprintf(f, @args);
  throw( ({ f, b[..sizeof(b)-2] }) );
}
