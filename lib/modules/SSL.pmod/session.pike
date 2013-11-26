#pike __REAL_VERSION__
#pragma strict_types

//! The most important information in a session object is a
//! choice of encryption algorithms and a "master secret" created by
//! keyexchange with a client. Each connection can either do a full key
//! exchange to established a new session, or reuse a previously
//! established session. That is why we have the session abstraction and
//! the session cache. Each session is used by one or more connections, in
//! sequence or simultaneously.
//!
//! It is also possible to change to a new session in the middle of a
//! connection.

#if constant(SSL.Cipher.CipherSpec)

import .Constants;
protected constant Struct = ADT.struct;

//! Identifies the session to the server
string identity;

//! Always COMPRESSION_null.
int compression_algorithm;

//! Constant defining a choice of keyexchange, encryption and mac
//! algorithm.
int cipher_suite;

//! Information about the encryption method derived from the
//! cipher_suite.
.Cipher.CipherSpec cipher_spec;

//! Key exchange method, also derived from the cipher_suite.
int ke_method;

//! Key exchange factory, derived from @[ke_method].
program(.Cipher.KeyExchange) ke_factory;

//! 48 byte secret shared between the client and the server. Used for
//! deriving the actual keys.
string master_secret;

//! information about the certificate in use by the peer, such as issuing authority, and verification status.
mapping cert_data;

array(int) version;

//! the peer certificate chain
array(string) peer_certificate_chain;

//! our certificate chain
array(string) certificate_chain;

//! The server's private key
Crypto.RSA rsa;

//! The server's dsa private key
Crypto.DSA dsa;

//! Indicates if this session has the required server certificate keys
//! set. No means that no or the wrong type of certificate was sent
//! from the server.
int(0..1) has_required_certificates()
{
  if( cipher_spec->sign == .Cipher.rsa_sign) return !!rsa;
  if( cipher_spec->sign == .Cipher.dsa_sign) return !!dsa;
  if( cipher_spec->sign == .Cipher.anon_sign) return 1;
  object o = function_object([function(mixed ...:void|mixed)](mixed)
			     cipher_spec->sign);
  if (objectp(o) && cipher_spec->sign == o->rsa_sign) return !!rsa;
  if (objectp(o) && cipher_spec->sign == o->dsa_sign) return !!dsa;
  return 0;
}

//! Sets the proper authentication method and cipher specification
//! for the given cipher @[suite] and @[verison].
int set_cipher_suite(int suite, ProtocolVersion|int version,
		     array(array(int))|void signature_algorithms)
{
  // FIXME: The maximum allowable hash size depends on the size of the
  //        RSA key when RSA is in use. With a 64 byte (512 bit) key,
  //        the block size is 61 bytes, allow for 23 bytes of overhead.
  int max_hash_size = 61-23;
  array res = .Cipher.lookup(suite, version, signature_algorithms,
			     max_hash_size);
  if (!res) return 0;
  cipher_suite = suite;
  ke_method = [int]res[0];
  switch(ke_method) {
  case KE_null:
    ke_factory = .Cipher.KeyExchangeNULL;
    break;
  case KE_rsa:
    ke_factory = .Cipher.KeyExchangeRSA;
    break;
  case KE_dh_anon:
    ke_factory = .Cipher.KeyExchangeDH;
    break;
  case KE_dhe_rsa:
  case KE_dhe_dss:
    ke_factory = .Cipher.KeyExchangeDHE;
    break;
  default:
    error("set_cipher_suite: Unsupported key exchange method: %d\n",
	  ke_method);
    break;
  }
  cipher_spec = [object(.Cipher.CipherSpec)]res[1];
#ifdef SSL3_DEBUG
  werror("SSL.session: cipher_spec %O\n",
	 mkmapping(indices(cipher_spec), values(cipher_spec)));
#endif
  return 1;
}

//! Sets the compression method. Currently only @[COMPRESSION_null] is
//! supported.
void set_compression_method(int compr)
{
  if (compr != COMPRESSION_null)
    error( "Method not supported\n" );
  compression_algorithm = compr;
}

protected string generate_key_block(string client_random, string server_random,
			  array(int) version)
{
  int required = 2 * (
    cipher_spec->is_exportable ?
    (5 + cipher_spec->hash_size)
    : ( cipher_spec->key_material +
	cipher_spec->hash_size +
	cipher_spec->iv_size)
  );
  string key = "";

  key = cipher_spec->prf(master_secret, "key expansion",
			 server_random + client_random, required);

#ifdef SSL3_DEBUG
  werror("key_block: %O\n", key);
#endif
  return key;
}

#ifdef SSL3_DEBUG
protected void printKey(string name, string key) {

  string res="";
  res+=sprintf("%s:  len:%d type:%d \t\t",name,sizeof(key),0); 
  /* return; */
  for(int i=0;i<sizeof(key);i++) {
    int d=key[i];
    res+=sprintf("%02x ",d&0xff);
  }
  res+="\n";
  werror(res);
}
#endif

//! Generates keys appropriate for the SSL version given in @[version],
//! based on the @[client_random] and @[server_random].
//! @returns
//!   @array
//!     @elem string 0
//!       Client write MAC secret
//!     @elem string 1
//!       Server write MAC secret
//!     @elem string 2
//!       Client write key
//!     @elem string 3
//!       Server write key
//!     @elem string 4
//!       Client write IV
//!     @elem string 5
//!       Server write IV
//!  @endarray
array(string) generate_keys(string client_random, string server_random,
			    array(int) version)
{
  Struct key_data = Struct(generate_key_block(client_random, server_random,
					      version));
  array(string) keys = allocate(6);

#ifdef SSL3_DEBUG
  werror("client_random: %s\nserver_random: %s\nversion: %d.%d\n",
	 client_random?String.string2hex(client_random):"NULL",
         server_random?String.string2hex(server_random):"NULL",
         version[0], version[1]);
#endif
  // client_write_MAC_secret
  keys[0] = key_data->get_fix_string(cipher_spec->hash_size);
  // server_write_MAC_secret
  keys[1] = key_data->get_fix_string(cipher_spec->hash_size);

  if (cipher_spec->is_exportable)
  {
    // Exportable (ie weak) crypto.
    if(version[1] == PROTOCOL_SSL_3_0) {
      // SSL 3.0
      keys[2] = Crypto.MD5.hash(key_data->get_fix_string(5) +
				client_random + server_random)
	[..cipher_spec->key_material-1];
      keys[3] = Crypto.MD5.hash(key_data->get_fix_string(5) +
				server_random + client_random)
	[..cipher_spec->key_material-1];
      if (cipher_spec->iv_size)
	{
	  keys[4] = Crypto.MD5.hash(client_random +
				    server_random)[..cipher_spec->iv_size-1];
	  keys[5] = Crypto.MD5.hash(server_random +
				    client_random)[..cipher_spec->iv_size-1];
	}

    } else if(version[1] >= PROTOCOL_TLS_1_0) {
      // TLS 1.0 or later.
      string client_wkey = key_data->get_fix_string(5);
      string server_wkey = key_data->get_fix_string(5);
      keys[2] = cipher_spec->prf(client_wkey, "client write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      keys[3] = cipher_spec->prf(server_wkey, "server write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      if(cipher_spec->iv_size) {
	string iv_block =
	  cipher_spec->prf("", "IV block",
			   client_random + server_random,
			   2 * cipher_spec->iv_size);
	keys[4]=iv_block[..cipher_spec->iv_size-1];
	keys[5]=iv_block[cipher_spec->iv_size..];
#ifdef SSL3_DEBUG
	werror("sizeof(keys[4]):%d  sizeof(keys[5]):%d\n",
	       sizeof([string]keys[4]), sizeof([string]keys[4]));
#endif
      }

    }
    
  }
  else {
    keys[2] = key_data->get_fix_string(cipher_spec->key_material);
    keys[3] = key_data->get_fix_string(cipher_spec->key_material);
    if (cipher_spec->iv_size)
      {
	keys[4] = key_data->get_fix_string(cipher_spec->iv_size);
	keys[5] = key_data->get_fix_string(cipher_spec->iv_size);
      }
  }

#ifdef SSL3_DEBUG
  printKey( "client_write_MAC_secret",keys[0]);
  printKey( "server_write_MAC_secret",keys[1]);
  printKey( "keys[2]",keys[2]);
  printKey( "keys[3]",keys[3]);
  
  if(cipher_spec->iv_size) {
    printKey( "keys[4]",keys[4]);
    printKey( "keys[5]",keys[5]);
    
  } else {
    werror("No IVs!!\n");
  }
#endif
  
  return keys;
}

//! Computes a new set of encryption states, derived from the
//! client_random, server_random and master_secret strings.
//!
//! @returns
//!   @array
//!     @elem SSL.state read_state
//!       Read state
//!     @elem SSL.state write_state
//!       Write state
//!   @endarray
array(.state) new_server_states(string client_random, string server_random,
				array(int) version)
{
  .state write_state = .state(this);
  .state read_state = .state(this);
  array(string) keys = generate_keys(client_random, server_random,version);

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
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
    }
    if (cipher_spec->iv_size)
    {
      if (version[1] >= PROTOCOL_TLS_1_1) {
	// TLS 1.1 and later have an explicit IV.
	read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
      }
      read_state->crypt->set_iv(keys[4]);
      write_state->crypt->set_iv(keys[5]);
    }
  }
  return ({ read_state, write_state });
}

//! Computes a new set of encryption states, derived from the
//! client_random, server_random and master_secret strings.
//!
//! @returns
//!   @array
//!     @elem SSL.state read_state
//!       Read state
//!     @elem SSL.state write_state
//!       Write state
//!   @endarray
array(.state) new_client_states(string client_random, string server_random,
				array(int) version)
{
  .state write_state = .state(this);
  .state read_state = .state(this);
  array(string) keys = generate_keys(client_random, server_random,version);
  
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
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
    }
    if (cipher_spec->iv_size)
    {
      if (version[1] >= PROTOCOL_TLS_1_1) {
	// TLS 1.1 and later have an explicit IV.
	read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
      }
      read_state->crypt->set_iv(keys[5]);
      write_state->crypt->set_iv(keys[4]);
    }
  }
  return ({ read_state, write_state });
}

#else // constant(SSL.Cipher.CipherSpec)
constant this_program_does_not_exist = 1;
#endif
