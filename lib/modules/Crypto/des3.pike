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

object set_encrypt_key(string key)
{
  /* An exception will be raised if key is weak */
  d[0]->set_encrypt_key(key[..7]);
  d[1]->set_decrypt_key(key[8..]);
  d[2]->set_encrypt_key(key[..7]);
  return this_object();
}

object set_decrypt_key(string key)
{
  /* An exception will be raised if key is weak */
  d[0]->set_decrypt_key(key[..7]);
  d[1]->set_encrypt_key(key[8..]);
  d[2]->set_decrypt_key(key[..7]);
  return this_object();
}

