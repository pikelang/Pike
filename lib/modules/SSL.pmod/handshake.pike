#pike __REAL_VERSION__
#pragma strict_types

/* $Id: handshake.pike,v 1.56 2005/05/25 14:43:37 mast Exp $
 *
 */

//! SSL.handshake keeps the state relevant for SSL handshaking. This
//! includes a pointer to a context object (which doesn't change), various
//! buffers, a pointer to a session object (reuse or created as
//! appropriate), and pending read and write states being negotiated.
//!
//! Each connection will have two sets or read and write state: The
//! current read and write states used for encryption, and pending read
//! and write states to be taken into use when the current keyexchange
//! handshake is finished.

//#define SSL3_PROFILING

#if constant(SSL.Cipher.DHKeyExchange)

import .Constants;

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

.session session;
.context context;

.state pending_read_state;
.state pending_write_state;

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

Crypto.RSA temp_key; /* Key used for session key exchange (if not the same
		      * as the server's certified key) */

.Cipher.DHKeyExchange dh_state; /* For diffie-hellman key exchange */

int rsa_message_was_bad;
array(int) version;
array(int) remote_version;

int reuse;

//! Random cookies, sent and received with the hello-messages.
string client_random;
string server_random;

constant Session = SSL.session;
constant Packet = SSL.packet;
constant Alert = SSL.alert;


#ifdef SSL3_PROFILING
int timestamp;
void addRecord(int t,int s) {
  Stdio.stdout.write("time: %.24f  type: %d sender: %d\n",time(timestamp),t,s);
}
#endif



/* Defined in connection.pike */
void send_packet(object packet, int|void fatal);

string handshake_messages;

Packet handshake_packet(int type, string data)
{

#ifdef SSL3_PROFILING
  addRecord(type,1);
#endif
  /* Perhaps one need to split large packages? */
  Packet packet = Packet();
  packet->content_type = PACKET_handshake;
  packet->fragment = sprintf("%c%3c%s", type, sizeof(data), data);
  handshake_messages += packet->fragment;
  return packet;
}

Packet hello_request()
{
  return handshake_packet(HANDSHAKE_hello_request, "");
}

Packet server_hello_packet()
{
  ADT.struct struct = ADT.struct();
  /* Build server_hello message */
  struct->put_uint(3,1); struct->put_uint(version[1],1); /* version */
#ifdef SSL3_DEBUG
  werror("Writing server hello, with version: 3."+version[1]+"\n");
#endif
  struct->put_fix_string(server_random);
  struct->put_var_string(session->identity, 1);
  struct->put_uint(session->cipher_suite, 2);
  struct->put_uint(session->compression_algorithm, 1);

  string data = struct->pop_data();
#ifdef SSL3_DEBUG
  werror("SSL.handshake: Server hello: %O\n", data);
#endif
  return handshake_packet(HANDSHAKE_server_hello, data);
}

Packet client_hello()
{
  ADT.struct struct = ADT.struct();
  /* Build client_hello message */
  struct->put_uint(3,1); struct->put_uint(1,1); /* version */
  client_random = sprintf("%4c%s", time(), context->random(28));
  struct->put_fix_string(client_random);
  struct->put_var_string("", 1);

  array(int) cipher_suites, compression_methods;
  cipher_suites = context->preferred_suites;
  compression_methods = context->preferred_compressors;

  int cipher_len = sizeof(cipher_suites)*2;
  struct->put_uint(cipher_len, 2);
  struct->put_fix_uint_array(cipher_suites, 2);
  struct->put_var_uint_array(compression_methods, 1, 1);

  string data = struct->pop_data();

#ifdef SSL3_DEBUG
  werror("SSL.handshake: Client hello: %O\n", data);
#endif

  return handshake_packet(HANDSHAKE_client_hello, data);
}

Packet server_key_exchange_packet()
{
  ADT.struct struct;
  
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
      werror("Sending a server key exchange-message, "
	     "with a %d-bits key.\n", temp_key->rsa_size());
#endif
      struct = ADT.struct();
      struct->put_bignum(temp_key->get_n());
      struct->put_bignum(temp_key->get_e());
    }
    else
      return 0;
    break;
  case KE_dhe_dss:
  case KE_dhe_rsa:
  case KE_dh_anon:
    struct = ADT.struct();

    /* werror("dh_params = %O\n", context->dh_params); */
    dh_state = .Cipher.DHKeyExchange(context->dh_params);
    dh_state->new_secret(context->random);
    
    struct->put_bignum(context->dh_params->p);
    struct->put_bignum(context->dh_params->g);
    struct->put_bignum(dh_state->our);
    break;
  default:
    return 0;
  }

  session->cipher_spec->sign(context, client_random + server_random, struct);
  return handshake_packet (HANDSHAKE_server_key_exchange,
			  struct->pop_data());
}

Packet client_key_exchange_packet()
{
  ADT.struct struct = ADT.struct();
  string data;

  switch (session->ke_method)
  {
  case KE_rsa:
    struct->put_uint(3,1); struct->put_uint(1,1);
    string random = context->random(46);
    struct->put_fix_string(random);
    string premaster_secret = struct->pop_data();
    session->master_secret = client_derive_master_secret(premaster_secret);

    array(.state) res = session->new_client_states(client_random,
						   server_random,version);
    pending_read_state = res[0];
    pending_write_state = res[1];

    data = (temp_key || context->rsa)->encrypt(premaster_secret);

    if(version[1]>0) 
      data=sprintf("%2c",sizeof(data))+data;
      
    break;
  case KE_dhe_dss:
  case KE_dhe_rsa:
  case KE_dh_anon:
#ifdef SSL3_DEBUG
    werror("FIXME: Not handled yet\n");
#endif
    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
    return 0;
    break;
  default:
    return 0;
  }

  return handshake_packet(HANDSHAKE_client_key_exchange, data);
}

int(-1..0) reply_new_session(array(int) cipher_suites,
			     array(int) compression_methods)
{
  reuse = 0;
  session = context->new_session();
  multiset(int) common_suites;

  SSL3_DEBUG_MSG("ciphers: me: %O, client: %O\n",
		 context->preferred_suites, cipher_suites);
  common_suites = mkmultiset(cipher_suites & context->preferred_suites);
  SSL3_DEBUG_MSG("intersection: %O\n", common_suites);

  if (sizeof(common_suites))
  {
    int suite;
    foreach(context->preferred_suites, suite)
      if (common_suites[suite]) break;
    session->set_cipher_suite(suite,version[1]);
  } else {
    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1]));
    return -1;
  }
  
  compression_methods &= context->preferred_compressors;
  if (sizeof(compression_methods))
    session->set_compression_method(compression_methods[0]);
  else
  {
    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1]));
    return -1;
  }
  
  send_packet(server_hello_packet());
  
  /* Send Certificate, ServerKeyExchange and CertificateRequest as
   * appropriate, and then ServerHelloDone.
   */
  if (context->certificates)
  {
    ADT.struct struct = ADT.struct();
    
    int len = `+( @ Array.map(context->certificates, sizeof));
#ifdef SSL3_DEBUG
//    werror("SSL.handshake: certificate_message size %d\n", len);
#endif
    struct->put_uint(len + 3 * sizeof(context->certificates), 3);
    foreach(context->certificates, string cert)
      struct->put_var_string(cert, 3);
    send_packet(handshake_packet(HANDSHAKE_certificate, struct->pop_data()));
  }
  else if (context->cipher_spec->sign != .Cipher.anon_sign)
    // Otherwise the server will just silently send an invalid
    // ServerHello sequence.
    error ("Certificate(s) missing.\n");

  Packet key_exchange = server_key_exchange_packet();

  if (key_exchange) {
    send_packet(key_exchange);
  }
  if (context->auth_level >= AUTHLEVEL_ask)
  {
    // if we have no authorities, this is all rather pointless. throw an error.

    if(!context->authorities_cache || !sizeof(context->authorities_cache))
      error("No certificate authorities provided.\n");
 
    /* Send a CertificateRequest message */
    ADT.struct struct = ADT.struct();
    struct->put_var_uint_array(context->preferred_auth_methods, 1, 1);

    int len; 

    // we should never get to a point where we send an empty certificate request.
    if(sizeof(context->authorities_cache))
    {
      len = `+(@ Array.map(context->authorities_cache,
			     lambda(Tools.X509.TBSCertificate s)
       { return sizeof(s->subject->get_der());} ));
      struct->put_uint(len + 2 * sizeof(context->authorities_cache), 2);
      foreach(context->authorities_cache, Tools.X509.TBSCertificate auth)
        struct->put_var_string(auth->subject->get_der(), 2);
    }
    send_packet(handshake_packet(HANDSHAKE_certificate_request,
				 struct->pop_data()));
    certificate_state = CERT_requested;
  }
  send_packet(handshake_packet(HANDSHAKE_server_hello_done, ""));
  return 0;
}

Packet change_cipher_packet()
{
  Packet packet = Packet();
  packet->content_type = PACKET_change_cipher_spec;
  packet->fragment = "\001";
  return packet;
}

string hash_messages(string sender)
{
  if(version[1] == 0) {
    return .Cipher.MACmd5(session->master_secret)->hash_master(handshake_messages + sender) +
      .Cipher.MACsha(session->master_secret)->hash_master(handshake_messages + sender);
  }
  else if(version[1] > 0) {
    return .Cipher.prf(session->master_secret, sender,
		       .Cipher.MACmd5()->hash_raw(handshake_messages)+
		       .Cipher.MACsha()->hash_raw(handshake_messages),12);
  }
}

Packet finished_packet(string sender)
{
#ifdef SSL3_DEBUG
           werror("Sending finished_packet, with sender=\""+sender+"\"\n" );
#endif
	   return handshake_packet(HANDSHAKE_finished, hash_messages(sender));
}

string server_derive_master_secret(string data)
{
  string premaster_secret;
  
#ifdef SSL3_DEBUG
  werror("server_derive_master_secret: ke_method %d\n", session->ke_method);
#endif
  switch(session->ke_method)
  {
  default:
    error( "Internal error\n" );
#if 0
    /* What is this for? */
  case 0:
    return 0;
#endif
  case KE_dhe_dss:
  case KE_dhe_rsa:
    if (!sizeof(data))
    {
      /* Implicit encoding; Should never happen unless we have
       * requested and received a client certificate of type
       * rsa_fixed_dh or dss_fixed_dh. Not supported. */
#ifdef SSL3_DEBUG
      werror("SSL.handshake: Client uses implicit encoding if its DH-value.\n"
	     "               Hanging up.\n");
#endif
      send_packet(Alert(ALERT_fatal, ALERT_certificate_unknown, version[1]));
      return 0;
    }
    /* Fall through */
  case KE_dh_anon:
  {
    /* Explicit encoding */
    ADT.struct struct = ADT.struct(data);

    if (catch
	{
	  dh_state->set_other(struct->get_bignum);
	} || !struct->is_empty())
      {
	send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	return 0;
      }

    premaster_secret = (string)dh_state->get_shared();
    dh_state = 0;
    break;
  }
  case KE_rsa:
   {
     /* Decrypt the premaster_secret */
#ifdef SSL3_DEBUG
     werror("encrypted premaster_secret: %O\n", data);
#endif
     if(version[1] > 0) {
       if(sizeof(data)-2 == data[0]*256+data[1]) {
	 premaster_secret = (temp_key || context->rsa)->decrypt(data[2..]);
       }
     } else {
       premaster_secret = (temp_key || context->rsa)->decrypt(data);
     }
#ifdef SSL3_DEBUG
     werror("premaster_secret: %O\n", premaster_secret);
#endif
     if (!premaster_secret
	 || (sizeof(premaster_secret) != 48)
	 || (premaster_secret[0] != 3)
	 || (premaster_secret[1] != remote_version[1]))
     {
       /* To avoid the chosen ciphertext attack discovered by Daniel
	* Bleichenbacher, it is essential not to send any error
	* messages back to the client until after the client's
	* Finished-message (or some other invalid message) has been
	* received.
	*/
       /* Also checks for version roll-back attacks.
	*/
#ifdef SSL3_DEBUG
       werror("SSL.handshake: Invalid premaster_secret! "
	      "A chosen ciphertext attack?\n");
       if (premaster_secret && sizeof(premaster_secret) > 2) {
	 werror("SSL.handshake: Strange version (%d.%d) detected in "
		"key exchange message (expected %d.%d).\n",
		premaster_secret[0], premaster_secret[1],
		remote_version[0], remote_version[1]);
       }
#endif

       premaster_secret = context->random(48);
       rsa_message_was_bad = 1;

     } else {
     }
     break;
   }
  }
  string res = "";

  .Cipher.MACsha sha = .Cipher.MACsha();
  .Cipher.MACmd5 md5 = .Cipher.MACmd5();

  if(version[1] == 0) {
    foreach( ({ "A", "BB", "CCC" }), string cookie)
      res += md5->hash_raw(premaster_secret
			   + sha->hash_raw(cookie + premaster_secret 
					   + client_random + server_random));
  }
  else if(version[1] > 0) {
    res=.Cipher.prf(premaster_secret,"master secret",
		    client_random+server_random,48);
  }
  
#ifdef SSL3_DEBUG
  werror("master: %O\n", res);
#endif
  return res;
}

string client_derive_master_secret(string premaster_secret)
{
  string res = "";

  .Cipher.MACsha sha = .Cipher.MACsha();
  .Cipher.MACmd5 md5 = .Cipher.MACmd5();

#ifdef SSL3_DEBUG
  werror("Handshake.pike: in client_derive_master_secret is version[1]="+version[1]+"\n");
#endif

  if(version[1] == 0) {
    foreach( ({ "A", "BB", "CCC" }), string cookie)
      res += md5->hash_raw(premaster_secret
			   + sha->hash_raw(cookie + premaster_secret 
					   + client_random + server_random));
  }
  else if(version[1] > 0) {
    res+=.Cipher.prf(premaster_secret,"master secret",client_random+server_random,48);
  }
  
#ifdef SSL3_DEBUG
  werror("bahmaster: %O\n", res);
#endif
  return res;
}

#ifdef SSL3_DEBUG_HANDSHAKE_STATE
mapping state_descriptions = lambda()
{
  array inds = glob("STATE_*", indices(this));
  array vals = map(inds, lambda(string ind) { return this[ind]; });
  return mkmapping(vals, inds);
}();

mapping type_descriptions = lambda()
{
  array inds = glob("HANDSHAKE_*", indices(SSL.Constants));
  array vals = map(inds, lambda(string ind) { return SSL.Constants[ind]; });
  return mkmapping(vals, inds);
}();

string describe_state(int i)
{
  return state_descriptions[i] || (string)i;
}

string describe_type(int i)
{
  return type_descriptions[i] || (string)i;
}
#endif


// verify that a certificate chain is acceptable
//
int verify_certificate_chain(array(string) certs)
{
  // do we need to verify the certificate chain?
  if(!context->verify_certificates)
    return 1;

  int issuer_known = 0;

  // step one: see if the issuer of the certificate is acceptable.
  // this means the issuer of the certificate must be one of the authorities.
  string r=Standards.PKCS.Certificate.get_certificate_issuer(certs[-1])
    ->get_der();

  // if we've got authorities, we need to check to see that the provided cert is authorized.
  // is this useful for server connections???
  if(sizeof(context->authorities_cache))
  {
    foreach(context->authorities_cache, Tools.X509.TBSCertificate c)
    {
      if((r == (c->subject->get_der()))) // we have a trusted issuer
      {
        issuer_known = 1;
        break;
      }
    }

    if(issuer_known==0)
    {
      return 0;
    }
  }

  // ok, so we have a certificate chain whose client certificate is 
  // issued by an authority known to us.
  
  // next we must verify the chain to see if the chain is unbroken

  mapping auth=([]);

  foreach(context->trusted_issuers_cache, array(Tools.X509.TBSCertificate) i)
  {
    // we want the first item, the top level
    auth[i[-1]->subject->get_der()] = i[-1]->public_key;
  }

  mapping result = Tools.X509.verify_certificate_chain(certs, auth, context->require_trust);


  if(result->verified)
  {
    session->cert_data = result;
    return 1;
  }
  else return 0;  

}

//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if hadshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occured. It uses the
//! send_packet() function to trasnmit packets.
int(-1..1) handle_handshake(int type, string data, string raw)
{
  ADT.struct input = ADT.struct(data);
#ifdef SSL3_PROFILING
  addRecord(type,0);
#endif
#ifdef SSL3_DEBUG_HANDSHAKE_STATE
  werror("SSL.handshake: state %s, type %s\n",
	 describe_state(handshake_state), describe_type(type));
  werror("sizeof(data)="+sizeof(data)+"\n");
#endif

  switch(handshake_state)
  {
  default:
    error( "Internal error\n" );
  case STATE_server_wait_for_hello:
   {
     array(int) cipher_suites;

     /* Reset all extra state variables */
     expect_change_cipher = certificate_state = 0;
     rsa_message_was_bad = 0;
     temp_key = 0;
     
     handshake_messages = raw;
     server_random = sprintf("%4c%s", time(), context->random(28));

     switch(type)
     {
     default:
       send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			 "SSL.session->handle_handshake: unexpected message\n",
			 backtrace()));
       return -1;
     case HANDSHAKE_client_hello:
      {
	string id;
	int cipher_len;
	array(int) cipher_suites;
	array(int) compression_methods;

       	if (
	  catch{
	  version = (remote_version = input->get_fix_uint_array(1, 2)) + ({});
	  client_random = input->get_fix_string(32);
	  id = input->get_var_string(1);
	  cipher_len = input->get_uint(2);
	  cipher_suites = input->get_fix_uint_array(2, cipher_len/2);
	  compression_methods = input->get_var_uint_array(1, 1);
	  SSL3_DEBUG_MSG("STATE_server_wait_for_hello: received hello\n"
			 "version = %d.%d\n"
			 "id=%O\n"
			 "cipher suites: %O\n"
			 "compression methods: %O\n",
			 version[0], version[1],
			 id, cipher_suites, compression_methods);

	}
	  || (version[0] != 3) || (cipher_len & 1))
	{
	  if (version[1] > 1) version[1] = 1;
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: unexpected message\n",
			    backtrace()));
	  return -1;
	}
	if (version[1] > 1) {
	  SSL3_DEBUG_MSG("Falling back to from SSL 3.%d to "
			 "SSL 3.1 (aka TLS 1.0).\n",
			 version[1]);
	  version[1] = 1;
	}

#ifdef SSL3_DEBUG
	if (!input->is_empty())
	  werror("SSL.connection->handle_handshake: "
		 "extra data in hello message ignored\n");
      
	if (sizeof(id))
	  werror("SSL.handshake: Looking up session %O\n", id);
#endif
	session = sizeof(id) && context->lookup_session(id);
	if (session)
	  {
#ifdef SSL3_DEBUG
	    werror("SSL.handshake: Reusing session %O\n", id);
#endif
	    /* Reuse session */
	  reuse = 1;
	  if (! ( (cipher_suites & ({ session->cipher_suite }))
		  && (compression_methods & ({ session->compression_algorithm }))))
	  {
	    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure,
			      version[1]));
	    return -1;
	  }
	  send_packet(server_hello_packet());

	  array(.state) res = session->new_server_states(client_random,
							 server_random,
							 version);
	  pending_read_state = res[0];
	  pending_write_state = res[1];
	  send_packet(change_cipher_packet());
	  if(version[1] == 0)
	    send_packet(finished_packet("SRVR"));
	  else if(version[1] > 0)
	    send_packet(finished_packet("server finished"));

	  expect_change_cipher = 1;
	 
	  handshake_state = STATE_server_wait_for_finish;
	} else {
	  /* New session, do full handshake. */
	  
	  int(-1..0) err = reply_new_session(cipher_suites,
					     compression_methods);
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
	int ci_len;	// Cipher specs length
	int id_len;	// Session id length
	int ch_len;	// Challenge length
	mixed err;
	if (err = catch{
	  version = (remote_version = input->get_fix_uint_array(1, 2)) + ({});
	  ci_len = input->get_uint(2);
	  id_len = input->get_uint(2);
	  ch_len = input->get_uint(2);
	} || (ci_len % 3) || !ci_len || (id_len) || (ch_len < 16)
	    || (version[0] != 3))
	{
#ifdef SSL3_DEBUG
	  werror("SSL.handshake: Error decoding SSL2 handshake:\n"
		 "%s\n", err?describe_backtrace(err):"");
#endif /* SSL3_DEBUG */
	  if (version[1] > 1) version[1] = 1;
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	  return -1;
	}

	if (version[1] > 1) {
	  SSL3_DEBUG_MSG("Falling back to from SSL 3.%d to "
			 "SSL 3.1 (aka TLS 1.0).\n",
			 version[1]);
	  version[1] = 1;
	}

	string challenge;
	if (catch{
	    // FIXME: Support for restarting sessions?
	  cipher_suites = input->get_fix_uint_array(3, ci_len/3);
	  string session = input->get_fix_string(id_len);
	  challenge = input->get_fix_string(ch_len);
	} || !input->is_empty()) 
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	  return -1;
	}

	if (ch_len < 32)
	  challenge = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" + challenge;
	client_random = challenge[sizeof (challenge) - 32..];

	{
	  int(-1..0) err = reply_new_session(cipher_suites,
					     ({ COMPRESSION_null }) );
	  if (err)
	    return err;
	}
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
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_finished:
     {
       string my_digest;
       string digest;
       
       if(version[1] == 0) {
	 my_digest=hash_messages("CLNT");
	 if (catch {
	   digest = input->get_fix_string(36);
	 } || !input->is_empty())
	   {
	     send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			       version[1],
			       "SSL.session->handle_handshake: unexpected message\n",
			       backtrace()));
	     return -1;
	   }
       } else if(version[1] == 1) {
	 my_digest=hash_messages("client finished");
	 if (catch {
	   digest = input->get_fix_string(12);
	 } || !input->is_empty())
	   {
	     send_packet(Alert(ALERT_fatal, ALERT_unexpected_message,
			       version[1],
			       "SSL.session->handle_handshake: unexpected message\n",
			       backtrace()));
	     return -1;
	   }
	 

       }

       if (rsa_message_was_bad       /* Error delayed until now */
	   || (my_digest != digest))
       {
	 if(rsa_message_was_bad)
	   SSL3_DEBUG_MSG("rsa_message_was_bad\n");
	 if(my_digest != digest)
	   SSL3_DEBUG_MSG("digests differ\n");
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	 return -1;
       }
       handshake_messages += raw; /* Second hash includes this message,
				   * the first doesn't */
       /* Handshake complete */
       
       if (!reuse)
       {
	 send_packet(change_cipher_packet());
	 if(version[1] == 0) send_packet(finished_packet("SRVR"));
	 else if(version[1] == 1) send_packet(finished_packet("server finished"));
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
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_client_key_exchange:
#ifdef SSL3_DEBUG
      werror("client_key_exchange\n");
#endif
      if (certificate_state == CERT_requested)
      { /* Certificate should be sent before key exchange message */
	send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			  "SSL.session->handle_handshake: unexpected message\n",
			  backtrace()));
	return -1;
      }

      if (!(session->master_secret = server_derive_master_secret(data)))
      {
	return -1;
      } else {

	// trace(1);
	array(.state) res = session->new_server_states(client_random,
						       server_random,version);
	pending_read_state = res[0];
	pending_write_state = res[1];
	
#ifdef SSL3_DEBUG
	werror("certificate_state: %d\n", certificate_state);
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
       if (certificate_state != CERT_requested)
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			   "SSL.session->handle_handshake: unexpected message\n",
			   backtrace()));
	 return -1;
       }
       if (catch {
	 int certs_len = input->get_uint(3);
	 array(string) certs = ({ });
	 while(!input->is_empty())
	   certs += ({ input->get_var_string(3) });

	  // we have the certificate chain in hand, now we must verify them.
          if(!verify_certificate_chain(certs))
          {
	     send_packet(Alert(ALERT_fatal, ALERT_bad_certificate, version[1],
			       "SSL.session->handle_handshake: bad certificate\n",
			       backtrace()));
  	     return -1;
          }
          else
          {
           session->peer_certificate_chain = certs;
          }
       } || !input->is_empty())
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
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
    // handshake_messages += raw;
    // compute challenge first, then update handshake_messages /Sigge
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_certificate_verify:
      if (!rsa_message_was_bad)
      {
	int(0..1) verification_ok;
	if( catch
	{
	  Gmp.mpz signature = input->get_bignum();
	  ADT.struct handshake_messages_struct = ADT.struct();
	  handshake_messages_struct->put_fix_string(handshake_messages);
	  verification_ok = session->cipher_spec->verify(
	    context, "", handshake_messages_struct, signature);
	} || verification_ok)
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: verification of"
			    " CertificateVerify message failed\n",
			    backtrace()));
	  return -1;
	}
// 	session->client_challenge =
// 	  mac_md5(session->master_secret)->hash_master(handshake_messages) +
// 	  mac_sha(session->master_secret)->hash_master(handshake_messages);
// 	session->client_signature = data;
      }
      handshake_messages += raw;
      handshake_state = STATE_server_wait_for_finish;
      expect_change_cipher = 1;
      break;
    }
    break;

  case STATE_client_wait_for_hello:
    if(type != HANDSHAKE_server_hello)
    {
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    }
    else
    {
      handshake_messages += raw;
      string id;
      int cipher_suite, compression_method;

      version = (remote_version = input->get_fix_uint_array(1, 2)) + ({});
      server_random = input->get_fix_string(32);
      id = input->get_var_string(1);
      cipher_suite = input->get_uint(2);
      compression_method = input->get_uint(1);

      if( !has_value(context->preferred_suites, cipher_suite) ||
	  !has_value(context->preferred_compressors, compression_method) ||
	  (version[0] != 3))
      {
	// The server tried to trick us to use some other cipher suite
	// or compression method than we wanted
	if (version[1] > 1) version[1] = 1;
	send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			  "SSL.session->handle_handshake: handshake failure\n",
			  backtrace()));
	return -1;
      }

      if (version[1] > 1) {
	SSL3_DEBUG_MSG("Falling back to from SSL 3.%d to "
		       "SSL 3.1 (aka TLS 1.0).\n",
		       version[1]);
	version[1] = 1;
      }

      session->set_cipher_suite(cipher_suite,version[1]);
      session->set_compression_method(compression_method);
      SSL3_DEBUG_MSG("STATE_client_wait_for_hello: received hello\n"
		     "version = %d.%d\n"
		     "id=%O\n"
		     "cipher suite: %O\n"
		     "compression method: %O\n",
		     version[0], version[1],
		     id, cipher_suite, compression_method);

      handshake_state = STATE_client_wait_for_server;
      break;
    }
    break;

  case STATE_client_wait_for_server:
    handshake_messages += raw;
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_certificate:
      {
      // FIXME: If anonymous connection, we don't need a cert.
      SSL3_DEBUG_MSG("Handshake: Certificate message received\n");
      int certs_len = input->get_uint(3);
      array(string) certs = ({ });
      while(!input->is_empty())
	certs += ({ input->get_var_string(3) });

      // we have the certificate chain in hand, now we must verify them.
      if(!verify_certificate_chain(certs))
      {
        send_packet(Alert(ALERT_fatal, ALERT_bad_certificate, version[1],
			  "SSL.session->handle_handshake: bad certificate\n",
			  backtrace()));
        error("Unable to verify peer certificate chain.\n");
//  	return -1;
      }
      else
      {
        session->peer_certificate_chain = certs;
      }
      
      mixed error=catch
      {
	Tools.X509.Verifier public_key = Tools.X509.decode_certificate(
                session->peer_certificate_chain[0])->public_key;

	if(public_key->type == "rsa")
	  {
	    Crypto.RSA rsa = Crypto.RSA();
	    rsa->set_public_key(public_key->rsa->get_n(),
				public_key->rsa->get_e());
	    context->rsa = rsa;
	  }
	else
	  {
#ifdef SSL3_DEBUG
	    werror("Other certificates than RSA not supported!\n");
#endif
	    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			      "SSL.session->handle_handshake: Unsupported certificate type\n",
			      backtrace()));
	    return -1;
	  }
      };

      if(error)
	{
#ifdef SSL3_DEBUG
	  werror("Failed to decode certificate!\n");
#endif
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: Failed to decode certificate\n",
			    backtrace()));
	  return -1;
	}
      
      certificate_state = CERT_received;
      break;
      }

    case HANDSHAKE_server_key_exchange:
      {
	Gmp.mpz n = input->get_bignum();
	Gmp.mpz e = input->get_bignum();
	Gmp.mpz signature = input->get_bignum();
	ADT.struct temp_struct = ADT.struct();
	temp_struct->put_bignum(n);
	temp_struct->put_bignum(e);
	int verification_ok;
	if( catch{ verification_ok = session->cipher_spec->verify(
	  context, client_random + server_random, temp_struct, signature); }
	    || !verification_ok)
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: verification of"
			    " ServerKeyExchange message failed\n",
			    backtrace()));
	  return -1;
	}
	Crypto.RSA rsa = Crypto.RSA();
	rsa->set_public_key(n, e);
	context->rsa = rsa;
	break;
      }

    case HANDSHAKE_certificate_request:
      {
#ifdef SSL3_DEBUG
	werror("Certificate request not yet implemented.\n");
#endif
	array(int) cert_types = input->get_var_uint_array(1, 1);
//       int num_distinguished_names = input->get_uint(2);
//       array(string) distinguished_names =
	send_packet(Alert(ALERT_warning, ALERT_no_certificate, version[1],
			  "", backtrace()));
      }
      break;

    case HANDSHAKE_server_hello_done:
      /* Send Certificate, ClientKeyExchange, CertificateVerify and
       * ChangeCipherSpec as appropriate, and then Finished.
       */
      {

      check_serv_cert: {
	  switch (session->cipher_spec->sign) {
	    case .Cipher.rsa_sign:
	      if (context->rsa) break check_serv_cert;
	      break;
	    case .Cipher.dsa_sign:
	      if (context->dsa) break check_serv_cert;
	      break;
	  }

#ifdef SSL3_DEBUG
	  werror ("Certificate message required from server.\n");
#endif
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: Certificate message missing\n",
			    backtrace()));
	  return -1;
	}

      if (context->certificates)
      {
	ADT.struct struct = ADT.struct();
    
	int len = `+( @ Array.map(context->certificates, sizeof));
#ifdef SSL3_DEBUG
	//    werror("SSL.handshake: certificate_message size %d\n", len);
#endif
	struct->put_uint(len + 3 * sizeof(context->certificates), 3);
	foreach(context->certificates, string cert)
	  struct->put_var_string(cert, 3);
	send_packet(handshake_packet(HANDSHAKE_certificate, struct->pop_data()));
      }


      Packet key_exchange = client_key_exchange_packet();

      if (key_exchange)
	send_packet(key_exchange);

      // FIXME: Certificate verify

      send_packet(change_cipher_packet());

      if(version[1] == 0) send_packet(finished_packet("CLNT"));
      else if(version[1] == 1) send_packet(finished_packet("client finished"));
      
      }
	handshake_state = STATE_client_wait_for_finish;
	expect_change_cipher = 1;
	break;
    }
    break;

  case STATE_client_wait_for_finish:
    {
    if((type) != HANDSHAKE_finished)
    {
      SSL3_DEBUG_MSG("Expected type HANDSHAKE_finished(%d), got %d\n",
		     HANDSHAKE_finished, type);
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    }
    else
      return 1;			// We're done shaking hands
    }
  }
#ifdef SSL3_DEBUG
//  werror("SSL.handshake: messages = %O\n", handshake_messages);
#endif
  return 0;
}

void create(int is_server, void|SSL.context ctx)
{

#ifdef SSL3_PROFILING
  timestamp=time();
  Stdio.stdout.write("New...\n");
#endif 
  version=({0,0});
  context = ctx;

  if (is_server)
    handshake_state = STATE_server_wait_for_hello;
  else
  {
    handshake_state = STATE_client_wait_for_hello;
    handshake_messages = "";
    session = context->new_session();
    send_packet(client_hello());
  }
}

#endif // constant(SSL.Cipher.DHKeyExchange)
