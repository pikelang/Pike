#pike __REAL_VERSION__

// Moahahahah!
// $Id: error.pmod,v 1.4 2000/09/28 03:38:27 hubbe Exp $
void `()(string f, mixed ... args)
{
  array(array) b = backtrace();
  throw( ({ sprintf(f, @args), b[..sizeof(b)-2] }) );
}
