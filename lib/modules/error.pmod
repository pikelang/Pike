// Moahahahah!
// $Id: error.pmod,v 1.1 1998/04/30 07:02:33 per Exp $
void `()(mixed ... args)
{
  throw( ({ sprintf(@args), backtrace() }) );
}
