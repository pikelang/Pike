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

private array(string) split_key(string key)
{
  string k1 = key[..7];
  string k2 = key[8..15];
  string k3 = (strlen(key) > 16) ? key[16..] : k1;
  return ({ k1, k2, k3 });
}

/* An exception will be raised if key is weak */
object set_encrypt_key(string key)
{
  array(string) keys = split_key(key);
  pipe :: set_encrypt_key( @ keys);
  /* Switch mode of middle crypto */
  d[1]->set_decrypt_key(keys[1]);
  return this_object();
}

/* An exception will be raised if key is weak */
object set_decrypt_key(string key)
{
  array(string) keys = split_key(key);
  pipe :: set_decrypt_key( @ keys);
  /* Switch mode of middle crypto */
  d[1]->set_encrypt_key(keys[1]);
  return this_object();
}

