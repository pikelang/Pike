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

constant Struct = (program) "struct";
constant State = (program) "state";

void set_cipher_suite(int suite)
{
  array res = cipher::lookup(suite);
  cipher_suite = suite;
  ke_method = res[0];
  cipher_spec = res[1];
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
  int required = 2 * (cipher_spec->key_material +
		       cipher_spec->hash_size +
		       cipher_spec->iv_size);
  object sha = mac_sha();
  object md5 = mac_md5();
  int i = 0;
  string key = "";
  while (strlen(key) < required)
  {
    i++;
    string cookie = replace(allocate(i), 0, sprintf("%c", 64+i)) * "";
//    werror(sprintf("cookie '%s'\n", cookie));
    key += md5->hash_raw(master_secret +
			 sha->hash_raw(cookie + master_secret +
				       server_random + client_random));
  }
//  werror(sprintf("key_block: '%s'\n", key));
  return key;
}

array new_server_states(string client_random, string server_random)
{
  object key_data = Struct(generate_key_block(client_random, server_random));
  object write_state = State(this_object());
  object read_state = State(this_object());

  write(sprintf("client_random: '%s'\nserver_random: '%s'\n",
		client_random, server_random));
  read_state->mac = cipher_spec->
    mac_algorithm(key_data->get_fix_string(cipher_spec->hash_size));
  write_state->mac = cipher_spec->
    mac_algorithm(key_data->get_fix_string(cipher_spec->hash_size));
  read_state->crypt = cipher_spec->bulk_cipher_algorithm();
  read_state->crypt->
    set_decrypt_key(key_data->get_fix_string(cipher_spec->key_material));

  write_state->crypt = cipher_spec->bulk_cipher_algorithm();
  write_state->crypt->
    set_encrypt_key(key_data->get_fix_string(cipher_spec->key_material));

  if (cipher_spec->iv_size)
  {
    read_state->crypt->
      set_iv(key_data->get_fix_string(cipher_spec->iv_size));
    write_state->crypt->
      set_iv(key_data->get_fix_string(cipher_spec->iv_size));
  }
  return ({ read_state, write_state });
}

array new_client_states(string client_random, string server_random)
{
  object key_data = Struct(generate_key_block(client_random, server_random));
  object write_state = State(this_object());
  object read_state = State(this_object());

  write_state->mac = cipher_spec->
    mac_algorithm(key_data->get_fix_string(cipher_spec->hash_size));
  read_state->mac = cipher_spec->
    mac_algorithm(key_data->get_fix_string(cipher_spec->hash_size));

  write_state->crypt = cipher_spec->bulk_cipher_algorithm();
  write_state->crypt->
    set_encrypt_key(key_data->get_fix_string(cipher_spec->key_material));

  read_state->crypt = cipher_spec->bulk_cipher_algorithm();
  read_state->crypt->
    set_decrypt_key(key_data->get_fix_string(cipher_spec->key_material));

  if (cipher_spec->iv_size)
  {
    write_state->crypt->
      set_iv(key_data->get_fix_string(cipher_spec->iv_size));
    read_state->crypt->
      set_iv(key_data->get_fix_string(cipher_spec->iv_size));
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
