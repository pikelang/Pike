//
// $Id: session.pike,v 1.35 2004/07/05 17:02:36 grubba Exp $

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
static constant Struct = ADT.struct;

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

//! 48 byte secret shared between the client and the server. Used for
//! deriving the actual keys.
string master_secret;

//! information about the certificate in use by the peer, such as issuing authority, and verification status.
mapping cert_data;

array(int) version;

//!
array(string) peer_certificate_chain;

//! Sets the proper authentication method and cipher specification
//! for the given cipher @[suite] and @[verison].
void set_cipher_suite(int suite, int version)
{
  array res = .Cipher.lookup(suite, version);
  cipher_suite = suite;
  ke_method = [int]res[0];
  cipher_spec = [object(.Cipher.CipherSpec)]res[1];
#ifdef SSL3_DEBUG
  werror("SSL.session: cipher_spec %O\n",
	 mkmapping(indices(cipher_spec), values(cipher_spec)));
#endif
}

//! Sets the compression method. Currently only @[COMPRESSION_null] is
//! supported.
void set_compression_method(int compr)
{
  if (compr != COMPRESSION_null)
    error( "Method not supported\n" );
  compression_algorithm = compr;
}

static string generate_key_block(string client_random, string server_random,
			  array(int) version)
{
  int required = 2 * (
#ifndef WEAK_CRYPTO_40BIT
    cipher_spec->is_exportable ?
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
    (5 + cipher_spec->hash_size)
#ifndef WEAK_CRYPTO_40BIT
    : ( cipher_spec->key_material +
	cipher_spec->hash_size +
	cipher_spec->iv_size)
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
  );
  string key = "";

  if(version[1]==0) {
    .Cipher.MACsha sha = .Cipher.MACsha();
    .Cipher.MACmd5 md5 = .Cipher.MACmd5();
    int i = 0;

    while (sizeof(key) < required)
      {
	i++;
	string cookie = replace(allocate(i), 0, sprintf("%c", 64+i)) * "";
#ifdef SSL3_DEBUG
	werror("cookie %O\n", cookie);
#endif
	key += md5->hash_raw(master_secret +
			     sha->hash_raw(cookie + master_secret +
					   server_random + client_random));
      }
  }
  else if(version[1]==1) {
    key = .Cipher.prf(master_secret, "key expansion",
		      server_random+client_random, required);
  }

#ifdef SSL3_DEBUG
  werror("key_block: %O\n", key);
#endif
  return key;
}

#ifdef SSL3_DEBUG
static void printKey(string name, string key) {

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
  werror("client_random: %O\nserver_random: %O\nversion: %d.%d\n",
	 client_random, server_random, version[0], version[1]);
#endif
  // client_write_MAC_secret
  keys[0] = key_data->get_fix_string(cipher_spec->hash_size);
  // server_write_MAC_secret
  keys[1] = key_data->get_fix_string(cipher_spec->hash_size);

#ifndef WEAK_CRYPTO_40BIT
  if (cipher_spec->is_exportable)
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
  {
    if(version[1]==0) {
      //SSL3.0
      function(string:string) md5 = .Cipher.MACmd5()->hash_raw;
      
      keys[2] = md5(key_data->get_fix_string(5) +
		    client_random + server_random)
	[..cipher_spec->key_material-1];
      keys[3] = md5(key_data->get_fix_string(5) +
		    server_random + client_random)
	[..cipher_spec->key_material-1];
      if (cipher_spec->iv_size)
	{
	  keys[4] = md5(client_random +
			server_random)[..cipher_spec->iv_size-1];
	  keys[5] = md5(server_random +
			client_random)[..cipher_spec->iv_size-1];
	}

    } if(version[1]==1) {
      //TLS1.0
      string client_wkey = key_data->get_fix_string(5);
      string server_wkey = key_data->get_fix_string(5);
      keys[2] = .Cipher.prf(client_wkey, "client write key",
			    client_random+server_random,
			    cipher_spec->key_material);
      keys[3] = .Cipher.prf(server_wkey, "server write key",
			    client_random+server_random,
			    cipher_spec->key_material);
      if(cipher_spec->iv_size) {
	string iv_block = .Cipher.prf("", "IV block",
				      client_random+server_random,
				      2*cipher_spec->iv_size);
	keys[4]=iv_block[..cipher_spec->iv_size-1];
	keys[5]=iv_block[cipher_spec->iv_size..];
#ifdef SSL3_DEBUG
	werror("sizeof(keys[4]):%d  sizeof(keys[5]):%d\n",
	       sizeof([string]keys[4]), sizeof([string]keys[4]));
#endif
      }
      
    }
    
  }
  
#ifndef WEAK_CRYPTO_40BIT
  else {
    keys[2] = key_data->get_fix_string(cipher_spec->key_material);
    keys[3] = key_data->get_fix_string(cipher_spec->key_material);
    if (cipher_spec->iv_size)
      {
	keys[4] = key_data->get_fix_string(cipher_spec->iv_size);
	keys[5] = key_data->get_fix_string(cipher_spec->iv_size);
      }
  }
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */

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
    if (cipher_spec->iv_size)
    {
      read_state->crypt->set_iv(keys[4]);
      write_state->crypt->set_iv(keys[5]);
    }
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
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
    if (cipher_spec->iv_size)
    {
      read_state->crypt->set_iv(keys[5]);
      write_state->crypt->set_iv(keys[4]);
    }
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
    }
  }
  return ({ read_state, write_state });
}

#endif // constant(SSL.Cipher.CipherSpec)
