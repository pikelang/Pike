/* $Id: handshake.pike,v 1.17 2000/05/04 16:06:36 grubba Exp $
 *
 */

// int is_server;

inherit "cipher";

object session;
object context;

object pending_read_state;
object pending_write_state;

/* State variables */

constant STATE_server_wait_for_hello		= 1;
constant STATE_server_wait_for_client		= 2;
constant STATE_server_wait_for_finish		= 3;
constant STATE_server_wait_for_verify		= 4;

constant STATE_client_wait_for_hello		= 10;
constant STATE_client_wait_for_server		= 11;
constant STATE_client_wait_for_finish		= 12;
int handshake_state;

constant CERT_none = 0;
constant CERT_requested = 1;
constant CERT_received = 2;
constant CERT_no_certificate = 3;
int certificate_state;

int expect_change_cipher; /* Reset to 0 if a change_cipher message is
			   * received */

object temp_key; /* Key used for session key exchange (if not the same
		  * as the server's certified key) */

object dh_state; /* For diffie-hellman key exchange */

int rsa_message_was_bad;


int reuse;

string my_random;
string other_random;

constant Struct = ADT.struct;
constant Session = SSL.session;
constant Packet = SSL.packet;
constant Alert = SSL.alert;

/* Defined in connection.pike */
void send_packet(object packet, int|void fatal);

string handshake_messages;

object handshake_packet(int type, string data)
{
  /* Perhaps one need to split large packages? */
  object packet = Packet();
  packet->content_type = PACKET_handshake;
  packet->fragment = sprintf("%c%3c%s", type, strlen(data), data);
  handshake_messages += packet->fragment;
  return packet;
}

object server_hello_packet()
{
  object struct = Struct();
  /* Build server_hello message */
  struct->put_uint(3,1); struct->put_uint(0,1); /* version */
  struct->put_fix_string(my_random);
  struct->put_var_string(session->identity, 1);
  struct->put_uint(session->cipher_suite, 2);
  struct->put_uint(session->compression_algorithm, 1);

  string data = struct->pop_data();
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.handshake: Server hello: '%O'\n", data));
#endif
  return handshake_packet(HANDSHAKE_server_hello, data);
}

object server_key_exchange_packet()
{
  object struct;
  
  switch (session->ke_method)
  {
  case KE_rsa:
#ifdef WEAK_CRYPTO_40BIT
    temp_key = context->short_rsa;
#endif /* WEAK_CRYPTO_40BIT (magic comment) */
#ifndef WEAK_CRYPTO_40BIT
    temp_key = (session->cipher_spec->is_exportable
		? context->short_rsa
		: context->long_rsa);
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
    if (temp_key)
    {
      /* Send a ServerKeyExchange message. */
      
#ifdef SSL3_DEBUG
      werror(sprintf("Sending a server key exchange-message, "
		     "with a %d-bits key.\n", temp_key->rsa_size()));
#endif
      struct = Struct();
      struct->put_bignum(temp_key->get_n());
      struct->put_bignum(temp_key->get_e());
    }
    else
      return 0;
    break;
  case KE_dhe_dss:
  case KE_dhe_rsa:
  case KE_dh_anon:
    struct = Struct();

    /* werror("dh_params = %O\n", context->dh_params); */
    dh_state = dh_key_exchange(context->dh_params);
    dh_state->new_secret(context->random);
    
    struct->put_bignum(context->dh_params->p);
    struct->put_bignum(context->dh_params->g);
    struct->put_bignum(dh_state->our);
    break;
  default:
    return 0;
  }

  session->cipher_spec->sign(context, other_random + my_random, struct);
  return handshake_packet(HANDSHAKE_server_key_exchange,
			  struct->pop_data());
}

int reply_new_session(array(int) cipher_suites, array(int) compression_methods)
{
  reuse = 0;
  session = context->new_session();
  multiset(int) common_suites;
  
#ifdef SSL3_DEBUG
  werror(sprintf("ciphers: me: %O, client: %O\n",
		   context->preferred_suites, cipher_suites)); 
//  werror(sprintf("compr: me: %O, client: %O\n",
//		   context->preferred_compressors, compression_methods)); 
#endif
  common_suites = mkmultiset(cipher_suites & context->preferred_suites);
#ifdef SSL3_DEBUG
  werror(sprintf("intersection: %O\n", common_suites));
#endif
  if (sizeof(common_suites))
  {
    int suite;
    foreach(context->preferred_suites, suite)
      if (common_suites[suite]) break;
    session->set_cipher_suite(suite);
  } else {
    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure));
    return -1;
  }
  
  compression_methods &= context->preferred_compressors;
  if (sizeof(compression_methods))
    session->set_compression_method(compression_methods[0]);
  else
  {
    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure));
    return -1;
  }
  
  send_packet(server_hello_packet());
  
  /* Send Certificate, ServerKeyExchange and CertificateRequest as
   * appropriate, and then ServerHelloDone.
   */
  if (context->certificates)
  {
    object struct = Struct();
    
    int len = `+( @ Array.map(context->certificates, strlen));
#ifdef SSL3_DEBUG
//    werror(sprintf("SSL.handshake: certificate_message size %d\n", len));
#endif
    struct->put_uint(len + 3 * sizeof(context->certificates), 3);
    foreach(context->certificates, string cert)
      struct->put_var_string(cert, 3);
    send_packet(handshake_packet(HANDSHAKE_certificate, struct->pop_data()));
  }

  object key_exchange = server_key_exchange_packet();

  if (key_exchange)
    send_packet(key_exchange);
  
  if (context->auth_level >= AUTHLEVEL_ask)
  {
    /* Send a CertificateRequest message */
    object struct = Struct();
    struct->put_var_uint_array(context->preferred_auth_methods, 1, 1);

    int len = `+(@ Array.map(context->authorities, strlen));
    struct->put_uint(len + 2 * sizeof(context->authorities), 2);
    foreach(context->authorities, string auth)
      struct->put_var_string(auth, 2);
    send_packet(handshake_packet(HANDSHAKE_certificate_request,
				 struct->pop_data()));
    certificate_state = CERT_requested;
  }
  send_packet(handshake_packet(HANDSHAKE_server_hello_done, ""));
  return 0;
}

object change_cipher_packet()
{
  object packet = Packet();
  packet->content_type = PACKET_change_cipher_spec;
  packet->fragment = "\001";
  return packet;
}

string hash_messages(string sender)
{
  return mac_md5(session->master_secret)->hash_master(handshake_messages + sender) +
    mac_sha(session->master_secret)->hash_master(handshake_messages + sender);
}

object finished_packet(string sender)
{
  return handshake_packet(HANDSHAKE_finished, hash_messages(sender));
}

string server_derive_master_secret(string data)
{
  string premaster_secret;
  
#ifdef SSL3_DEBUG
  werror(sprintf("server_derive_master_secret: ke_method %d\n",
		 session->ke_method));
#endif
  switch(session->ke_method)
  {
  default:
    throw( ({ "SSL.handshake: internal error\n", backtrace() }) );
#if 0
    /* What is this for? */
  case 0:
    return 0;
#endif
  case KE_dhe_dss:
  case KE_dhe_rsa:
    if (!strlen(data))
    {
      /* Implicit encoding; Should never happen unless we have
       * requested and received a client certificate of type
       * rsa_fixed_dh or dss_fixed_dh. Not supported. */
#ifdef SSL3_DEBUG
      werror("SSL.handshake: Client uses implicit encoding if its DH-value.\n"
	     "               Hanging up.\n");
#endif
      send_packet(Alert(ALERT_fatal, ALERT_certificate_unknown));
      return 0;
    }
    /* Fall through */
  case KE_dh_anon:
  {
    /* Explicit encoding */
    object struct = Struct(data);

    if (catch
	{
	  dh_state->set_other(struct->get_bignum);
	} || !struct->is_empty())
      {
	send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	return 0;
      }

    premaster_secret = dh_state->get_shared();
    dh_state = 0;
    break;
  }
  case KE_rsa:
   {
     /* Decrypt the premaster_secret */
#ifdef SSL3_DEBUG
     werror(sprintf("encrypted premaster_secret: '%O'\n", data));
#endif
     // trace(1);
     premaster_secret = (temp_key || context->rsa)->decrypt(data);
#ifdef SSL3_DEBUG
     werror(sprintf("premaster_secret: '%O'\n", premaster_secret));
#endif
     if (!premaster_secret
	 || (strlen(premaster_secret) != 48)
	 || (premaster_secret[0] != 3))
     {

       /* To avoid the chosen ciphertext attack discovered by Daniel
	* Bleichenbacher, it is essential not to send any error *
	* messages back to the client until after the client's *
	* Finished-message (or some other invalid message) has been *
	* recieved. */

       werror("SSL.handshake: Invalid premaster_secret! "
	      "A chosen ciphertext attack?\n");

       premaster_secret = context->random(48);
       rsa_message_was_bad = 1;

     } else {
       /* FIXME: When versions beyond 3.0 are supported,
	* the version number here must be checked more carefully
	* for a version rollback attack. */
#ifdef SSL3_DEBUG
       if (premaster_secret[1] > 0)
	 werror("SSL.handshake: Newer version detected in key exchange message.\n");
#endif
     }
     break;
   }
  }
  string res = "";
  
  object sha = mac_sha();
  object md5 = mac_md5();
  foreach( ({ "A", "BB", "CCC" }), string cookie)
    res += md5->hash_raw(premaster_secret
			 + sha->hash_raw(cookie + premaster_secret 
					 + other_random + my_random));
    
#ifdef SSL3_DEBUG
  werror(sprintf("master: '%O'\n", res));
#endif
  return res;
}

/* return 0 if handshake is in progress, 1 if finished, -1 if there's a
 * fatal error. */
int handle_handshake(int type, string data, string raw)
{
  object input = Struct(data);

#ifdef SSL3_DEBUG
  werror(sprintf("SSL.handshake: state %d, type %d\n", handshake_state, type));
#endif
  
  switch(handshake_state)
  {
  default:
    throw( ({ "SSL.handshake: internal error\n", backtrace() }) );
  case STATE_server_wait_for_hello:
   {
     array(int) cipher_suites;

     /* Reset all extra state variables */
     expect_change_cipher = certificate_state = 0;
     rsa_message_was_bad = 0;
     temp_key = 0;
     
     handshake_messages = raw;
     my_random = sprintf("%4c%s", time(), context->random(28));

     switch(type)
     {
     default:
       send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			 "SSL.session->handle_handshake: unexpected message\n",
			 backtrace()));
       return -1;
     case HANDSHAKE_client_hello:
      {
	array(int) version;
	string id;
	int cipher_len;
	array(int) cipher_suites;
	array(int) compression_methods;
             
	if (catch{
	  version = input->get_fix_uint_array(1, 2);
	  other_random = input->get_fix_string(32);
	  id = input->get_var_string(1);
	  cipher_len = input->get_uint(2);
	  cipher_suites = input->get_fix_uint_array(2, cipher_len/2);
	  compression_methods = input->get_var_uint_array(1, 1);
	} || (version[0] != 3) || (cipher_len & 1))
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	  return -1;
	}

#ifdef SSL3_DEBUG
	if (!input->is_empty())
	  werror("SSL.connection->handle_handshake: "
		 "extra data in hello message ignored\n");
      
	if (version[1] > 0)
	  werror(sprintf("SSL.handshake->handle_handshake: "
			 "Version %d.%d hello detected\n", @version));

	if (strlen(id))
	  werror(sprintf("SSL.handshake: Looking up session %O\n", id));
#endif
	session = strlen(id) && context->lookup_session(id);
	if (session)
	{
#ifdef SSL3_DEBUG
	  werror(sprintf("SSL.handshake: Reusing session %O\n", id));
#endif
	  /* Reuse session */
	  reuse = 1;
	  if (! ( (cipher_suites & ({ session->cipher_suite }))
		  && (compression_methods & ({ session->compression_algorithm }))))
	  {
	    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure));
	    return -1;
	  }
	  send_packet(server_hello_packet());

	  array res = session->new_server_states(other_random, my_random);
	  pending_read_state = res[0];
	  pending_write_state = res[1];
	  send_packet(change_cipher_packet());
	  send_packet(finished_packet("SRVR"));
	  expect_change_cipher = 1;
	 
	  handshake_state = STATE_server_wait_for_finish;
	} else {
	  /* New session, do full handshake. */
	  
	  int err = reply_new_session(cipher_suites, compression_methods);
	  if (err)
	    return err;
	  handshake_state = STATE_server_wait_for_client;
	}
	break;
      }
     case HANDSHAKE_hello_v2:
      {
#ifdef SSL3_DEBUG
	werror("SSL.handshake: SSL2 hello message received\n");
#endif
	int ci_len;
	int id_len;
	int ch_len;
	array(int) version;
	mixed err;
	if (err = catch{
	  version = input->get_fix_uint_array(1, 2);
	  ci_len = input->get_uint(2);
	  id_len = input->get_uint(2);
	  ch_len = input->get_uint(2);
	} || (ci_len % 3) || !ci_len || (id_len) || (ch_len < 16)
	|| (version[0] != 3))
	{
#ifdef SSL3_DEBUG
	  werror(sprintf("SSL.handshake: Error decoding SSL2 handshake:\n"
			 "%s\n", describe_backtrace(err)));
#endif /* SSL3_DEBUG */
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	  return -1;
	}

#ifdef SSL3_DEBUG
	if (version[1] > 0)
	  werror(sprintf("SSL.connection->handle_handshake: "
			 "Version %d.%d hello detected\n", @version));
#endif

	string challenge;
	if (catch{
	  cipher_suites = input->get_fix_uint_array(3, ci_len/3);
	  challenge = input->get_fix_string(ch_len);
	} || !input->is_empty()) 
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	  return -1;
	}
	other_random = ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" + challenge)[..31];
	int err = reply_new_session(cipher_suites, ({ COMPRESSION_null }) );
	if (err)
	  return err;
	handshake_state = STATE_server_wait_for_client;
	
	break;
      }
     }
     break;
   }
  case STATE_server_wait_for_finish:
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_finished:
     {
       string my_digest = hash_messages("CLNT");
       string digest;
       if (catch {
	 digest = input->get_fix_string(36);
       } || !input->is_empty())
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	 return -1;
       }
       if (rsa_message_was_bad       /* Error delayed until now */
	   || (my_digest != digest))
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message));
	 return -1;
       }
       handshake_messages += raw; /* Second hash includes this message,
				   * the first doesn't */
       /* Handshake complete */
       
       if (!reuse)
       {
	 send_packet(change_cipher_packet());
	 send_packet(finished_packet("SRVR"));
	 expect_change_cipher = 1;
	 context->record_session(session); /* Cache this session */
       }
       handshake_state = STATE_server_wait_for_hello;

       return 1;
     }   
    }
    break;
  case STATE_server_wait_for_client:
    handshake_messages += raw;
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_client_key_exchange:
#ifdef SSL3_DEBUG
      werror("client_key_exchange\n");
#endif
      if (certificate_state == CERT_requested)
      { /* Certificate should be sent before key exchange message */
	send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			  "SSL.session->handle_handshake: unexpected message\n",
			  backtrace()));
	return -1;
      }

      if (!(session->master_secret = server_derive_master_secret(data)))
      {
	return -1;
      } else {
	
	// trace(1);
	array res = session->new_server_states(other_random, my_random);
	pending_read_state = res[0];
	pending_write_state = res[1];
	
#ifdef SSL3_DEBUG
	werror(sprintf("certificate_state: %d\n", certificate_state));
#endif
      }
      if (certificate_state != CERT_received)
      {
	handshake_state = STATE_server_wait_for_finish;
	expect_change_cipher = 1;
      }
      else
	handshake_state = STATE_server_wait_for_verify;
      
      break;
    case HANDSHAKE_certificate:
     {
       if (certificate_state == CERT_requested)
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			   "SSL.session->handle_handshake: unexpected message\n",
			   backtrace()));
	 return -1;
       }
       if (catch {
	 session->client_certificate = input->get_var_string(3);
       } || !input->is_empty())
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			   "SSL.session->handle_handshake: unexpected message\n",
			   backtrace()));
	 return -1;
       }	
       certificate_state = CERT_received;
       break;
     }
    }
    break;
  case STATE_server_wait_for_verify:
    handshake_messages += raw;
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_certificate_verify:
      if (!rsa_message_was_bad)
      {
	session->client_challenge =
	  mac_md5(session->master_secret)->hash_master(handshake_messages) +
	  mac_sha(session->master_secret)->hash_master(handshake_messages);
	session->client_signature = data;
      }
      handshake_messages += raw;
      handshake_state = STATE_server_wait_for_finish;
      expect_change_cipher = 1;
      break;
    }
    break;

  case STATE_client_wait_for_hello:
   {
   }
  case STATE_client_wait_for_server:
   {
   }
  case STATE_client_wait_for_finish:
   {
   }
  }
#ifdef SSL3_DEBUG
//  werror(sprintf("SSL.handshake: messages = '%O'\n", handshake_messages));
#endif
  return 0;
}

void create(int is_server)
{
  if (is_server)
    handshake_state = STATE_server_wait_for_hello;
  else
    throw( ({ "SSL.handshake->create: client handshake not implemented\n",
		backtrace() }) );
}
