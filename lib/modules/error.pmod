#pike __VERSION__

// Moahahahah!
// $Id: error.pmod,v 1.3 2000/09/26 18:59:08 hubbe Exp $
void `()(string f, mixed ... args)
{
  array(array) b = backtrace();
  throw( ({ sprintf(f, @args), b[..sizeof(b)-2] }) );
}
