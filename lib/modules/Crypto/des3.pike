/* des3.pike
 *
 */

inherit Crypto.pipe : pipe;

private array(object) d;

void create()
{
  d = ({ Crypto.des(), Crypto.des(), Crypto.des() });
  pipe::create( @ d); 
}

int query_key_size() { return 16; }

int query_block_size() { return 8; }

/* An exception will be raised if key is weak */
object set_encrypt_key(string key)
{
  pipe :: set_encrypt_key(key[..7], key[8..], key[..7]);
  /* Switch mode of middle crypto */
  d[1]->set_decrypt_key(key[8..]);
  return this_object();
}

/* An exception will be raised if key is weak */
object set_decrypt_key(string key)
{
  pipe :: set_decrypt_key(key[..7], key[8..], key[..7]);
  /* Switch mode of middle crypto */
  d[1]->set_encrypt_key(key[8..]);
  return this_object();
}

