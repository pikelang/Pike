/* session.pike
 *
 */

inherit "cipher" : cipher;

// object server;

string identity; /* Identifies the session to the server */
int compression_algorithm;
int cipher_suite;
object cipher_spec;
int ke_method;
string master_secret; /* 48 byte secret shared between client and server */

constant Struct = ADT.struct;
constant State = SSL.state;

void set_cipher_suite(int suite)
{
  array res = cipher::lookup(suite);
  cipher_suite = suite;
  ke_method = res[0];
  cipher_spec = res[1];
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.session: cipher_spec %O\n",
		 mkmapping(indices(cipher_spec), values(cipher_spec))));
#endif
}

void set_compression_method(int compr)
{
  if (compr != COMPRESSION_null)
    throw( ({ "SSL.session->set_compression_method: Method not supported\n",
		backtrace() }) );
  compression_algorithm = compr;
}

string generate_key_block(string client_random, string server_random)
{
  int required = 2 * (cipher_spec->is_exportable
		      ? (5 + cipher_spec->hash_size)
		      : ( cipher_spec->key_material +
			  cipher_spec->hash_size +
			  cipher_spec->iv_size));
  object sha = mac_sha();
  object md5 = mac_md5();
  int i = 0;
  string key = "";
  while (strlen(key) < required)
  {
    i++;
    string cookie = replace(allocate(i), 0, sprintf("%c", 64+i)) * "";
#ifdef SSL3_DEBUG
//    werror(sprintf("cookie '%s'\n", cookie));
#endif
    key += md5->hash_raw(master_secret +
			 sha->hash_raw(cookie + master_secret +
				       server_random + client_random));
  }
#ifdef SSL3_DEBUG
//  werror(sprintf("key_block: '%s'\n", key));
#endif
  return key;
}

array generate_keys(string client_random, string server_random)
{
  object key_data = Struct(generate_key_block(client_random, server_random));
  array keys = allocate(6);

#ifdef SSL3_DEBUG
  werror(sprintf("client_random: '%s'\nserver_random: '%s'\n",
		client_random, server_random));
#endif
  /* client_write_MAC_secret */
  keys[0] = key_data->get_fix_string(cipher_spec->hash_size);
  /* server_write_MAC_secret */
  keys[1] = key_data->get_fix_string(cipher_spec->hash_size);

  if (cipher_spec->is_exportable)
  {
    object md5 = mac_md5()->hash_raw;
    
    keys[2] = md5(key_data->get_fix_string(5) +
		 client_random + server_random)
      [..cipher_spec->key_material-1];
    keys[3] = md5(key_data->get_fix_string(5) +
		 server_random + client_random)
      [..cipher_spec->key_material-1];
    if (cipher_spec->iv_size)
    {
      keys[4] = md5(client_random + server_random)[..cipher_spec->iv_size-1];
      keys[5] = md5(server_random + client_random)[..cipher_spec->iv_size-1];
    }
  } else {
    keys[2] = key_data->get_fix_string(cipher_spec->key_material);
    keys[3] = key_data->get_fix_string(cipher_spec->key_material);
    if (cipher_spec->iv_size)
    {
      keys[4] = key_data->get_fix_string(cipher_spec->iv_size);
      keys[5] = key_data->get_fix_string(cipher_spec->iv_size);
    }
  }
  return keys;
}

array new_server_states(string client_random, string server_random)
{
  object write_state = State(this_object());
  object read_state = State(this_object());
  array keys = generate_keys(client_random, server_random);

  if (cipher_spec->mac_algorithm)
  {
    read_state->mac = cipher_spec->mac_algorithm(keys[0]);
    write_state->mac = cipher_spec->mac_algorithm(keys[1]);
  }
  if (cipher_spec->bulk_cipher_algorithm)
  {
    read_state->crypt = cipher_spec->bulk_cipher_algorithm();
    read_state->crypt->set_decrypt_key(keys[2]);
    write_state->crypt = cipher_spec->bulk_cipher_algorithm();
    write_state->crypt->set_encrypt_key(keys[3]);
    if (cipher_spec->iv_size)
    {
      read_state->crypt->set_iv(keys[4]);
      write_state->crypt->set_iv(keys[5]);
    }
    if (cipher_spec->cipher_type == CIPHER_block)
    { /* Crypto.crypto takes care of splitting input into blocks */
      read_state->crypt = Crypto.crypto(read_state->crypt);
      write_state->crypt = Crypto.crypto(write_state->crypt);
    }
  }
  return ({ read_state, write_state });
}

array new_client_states(string client_random, string server_random)
{
  object write_state = State(this_object());
  object read_state = State(this_object());
  array keys = generate_keys(client_random, server_random);
  
  if (cipher_spec->mac_algorithm)
  {
    read_state->mac = cipher_spec->mac_algorithm(keys[1]);
    write_state->mac = cipher_spec->mac_algorithm(keys[0]);
  }
  if (cipher_spec->bulk_cipher_algorithm)
  {
    read_state->crypt = cipher_spec->bulk_cipher_algorithm();
    read_state->crypt->set_decrypt_key(keys[3]);
    write_state->crypt = cipher_spec->bulk_cipher_algorithm();
    write_state->crypt->set_encrypt_key(keys[2]);
    if (cipher_spec->iv_size)
    {
      read_state->crypt->set_iv(keys[5]);
      write_state->crypt->set_iv(keys[4]);
    }
    if (cipher_spec->cipher_type == CIPHER_block)
    { /* Crypto.crypto takes care of splitting input into blocks */
      read_state->crypt = Crypto.crypto(read_state->crypt);
      write_state->crypt = Crypto.crypto(write_state->crypt);
    }
  }
  return ({ read_state, write_state });
}
    
#if 0
void create(int is_s, int|void auth)
{
  is_server = is_s;
  if (is_server)
  {
    handshake_state = STATE_SERVER_WAIT_FOR_HELLO;
    auth_type = auth || AUTH_none;
  }
  else
  {
    handshake_state = STATE_CLIENT_WAIT_FOR_HELLO;
    auth_type = auth || AUTH_require;
  }
}
#endif
