// Moahahahah!
// $Id: error.pmod,v 1.2 1999/08/10 00:27:43 mast Exp $
void `()(string f, mixed ... args)
{
  array(array) b = backtrace();
  throw( ({ sprintf(f, @args), b[..sizeof(b)-2] }) );
}
