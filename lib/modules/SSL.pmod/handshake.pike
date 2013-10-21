#pike __REAL_VERSION__
#pragma strict_types

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

constant STATE_client_min			= 10;
constant STATE_client_wait_for_hello		= 10;
constant STATE_client_wait_for_server		= 11;
constant STATE_client_wait_for_finish		= 12;
int handshake_state;

int handshake_finished = 0;

constant CERT_none = 0;
constant CERT_requested = 1;
constant CERT_received = 2;
constant CERT_no_certificate = 3;
int certificate_state;

int expect_change_cipher; /* Reset to 0 if a change_cipher message is
			   * received */

// RFC 5746-related fields
int secure_renegotiation;
string client_verify_data = "";	// 3.2: Initially of zero length for both the
string server_verify_data = "";	//      ClientHello and the ServerHello.

// RFC 4366 3.1
array(string) server_names;

Crypto.RSA temp_key; /* Key used for session key exchange (if not the same
		      * as the server's certified key) */

.Cipher.DHKeyExchange dh_state; /* For diffie-hellman key exchange */

int rsa_message_was_bad;
ProtocolVersion min_version = PROTOCOL_SSL_3_0;
array(int) version;
array(int) client_version; /* Used to check for version roll-back attacks. */
int anonymous = 0;
int reuse;

//! A few storage variables for client certificate handling on the client side.
array(int) client_cert_types;
array(string) client_cert_distinguished_names;


//! Random cookies, sent and received with the hello-messages.
string client_random;
string server_random;

constant Session = SSL.session;
constant Packet = SSL.packet;
constant Alert = SSL.alert;

int has_next_protocol_negotiation;
int has_application_layer_protocol_negotiation;
string next_protocol;

#ifdef SSL3_PROFILING
int timestamp;
void addRecord(int t,int s) {
  Stdio.stdout.write("time: %.24f  type: %d sender: %d\n",time(timestamp),t,s);
}
#endif

private array(string) select_server_certificate()
{
  array(string) certs;
  if(context->select_server_certificate_func)
    certs = context->select_server_certificate_func(context, server_names);
  if(!certs)
    certs = context->certificates;

  return certs;
}

private object select_server_key()
{
  object key;
  if(context->select_server_key_func)
    key = context->select_server_key_func(context, server_names);
  if(!key) // fallback on previous behavior.
    key = context->rsa || context->dsa;

  return key;
}

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
  packet->fragment = sprintf("%c%3H", type, [string(0..255)]data);
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
  struct->put_uint(version[0],1); struct->put_uint(version[1],1); /* version */
  SSL3_DEBUG_MSG("Writing server hello, with version: %d.%d\n", @version);
  struct->put_fix_string(server_random);
  struct->put_var_string(session->identity, 1);
  struct->put_uint(session->cipher_suite, 2);
  struct->put_uint(session->compression_algorithm, 1);

  ADT.struct extensions = ADT.struct();

  if (secure_renegotiation) {
    // RFC 5746 3.7:
    // The server MUST include a "renegotiation_info" extension
    // containing the saved client_verify_data and server_verify_data in
    // the ServerHello.
    extensions->put_uint(EXTENSION_renegotiation_info, 2);
    ADT.struct extension = ADT.struct();
    extension->put_var_string(client_verify_data + server_verify_data, 1);

    extensions->put_var_string(extension->pop_data(), 2);
  }

  if (has_application_layer_protocol_negotiation &&
      next_protocol)
  {
    extensions->put_uint(EXTENSION_application_layer_protocol_negotiation,2);
    extensions->put_uint(sizeof(next_protocol)+3, 2);
    extensions->put_uint(sizeof(next_protocol)+1, 2);
    extensions->put_var_string(next_protocol, 1);
  }
  else if (has_next_protocol_negotiation &&
           context->advertised_protocols) {
    extensions->put_uint(EXTENSION_next_protocol_negotiation, 2);
    ADT.struct extension = ADT.struct();
    foreach (context->advertised_protocols;; string proto) {
      extension->put_var_string(proto, 1);
    }
    extensions->put_var_string(extension->pop_data(), 2);
  }

  if (!extensions->is_empty())
      struct->put_var_string(extensions->pop_data(), 2);

  string data = struct->pop_data();
  SSL3_DEBUG_MSG("SSL.handshake: Server hello: %O\n", data);
  return handshake_packet(HANDSHAKE_server_hello, data);
}

Packet client_hello()
{
  ADT.struct struct = ADT.struct();
  /* Build client_hello message */
  client_version = version + ({});
  struct->put_uint(client_version[0], 1); /* version */
  struct->put_uint(client_version[1], 1);
  client_random = sprintf("%4c%s", time(), context->random(28));
  struct->put_fix_string(client_random);
  struct->put_var_string("", 1);

  array(int) cipher_suites, compression_methods;
  cipher_suites = context->preferred_suites;
  if (!handshake_finished && !secure_renegotiation) {
    // Initial handshake.
    // Use the backward-compat way of asking for
    // support for secure renegotiation.
    cipher_suites += ({ TLS_empty_renegotiation_info_scsv });
  }
  compression_methods = context->preferred_compressors;

  int cipher_len = sizeof(cipher_suites)*2;
  struct->put_uint(cipher_len, 2);
  struct->put_fix_uint_array(cipher_suites, 2);
  struct->put_var_uint_array(compression_methods, 1, 1);

  ADT.struct extensions = ADT.struct();
  ADT.struct extension;
  int have_extensions = 0;

  if (secure_renegotiation) {
    have_extensions++;

    // RFC 5746 3.4:
    // The client MUST include either an empty "renegotiation_info"
    // extension, or the TLS_EMPTY_RENEGOTIATION_INFO_SCSV signaling
    // cipher suite value in the ClientHello.  Including both is NOT
    // RECOMMENDED.
    extension = ADT.struct();
    extension->put_var_string(client_verify_data, 1);
    extensions->put_uint(EXTENSION_renegotiation_info, 2);

    extensions->put_var_string(extension->pop_data(), 2);
  }

  if(context->client_use_sni)
  {
    have_extensions++;
    extension = ADT.struct();
    if(context->client_server_names)
    {
      foreach(context->client_server_names;; string server_name)
      {
        ADT.struct hostname = ADT.struct();
        hostname->put_uint(0, 1); // hostname
        hostname->put_var_string(server_name, 2); // hostname
 
        extension->put_var_string(hostname->pop_data(), 2);
      }
    }

    SSL3_DEBUG_MSG("SSL.handshake: Adding Server Name extension.\n");
    extensions->put_uint(EXTENSION_server_name, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  } 

  if (context->advertised_protocols)
  {
    have_extensions++;
    array(string) prots = context->advertised_protocols;
    extension = ADT.struct();
    extension->put_uint( [int]Array.sum(Array.map(prots, sizeof)) +
                         sizeof(prots), 2);
    foreach(context->advertised_protocols;; string proto)
      extension->put_var_string(proto, 1);

    SSL3_DEBUG_MSG("SSL.handshake: Adding ALPN extension.\n");
    extensions->put_uint(EXTENSION_application_layer_protocol_negotiation, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  }

  if(have_extensions)
    struct->put_var_string(extensions->pop_data(), 2);

  string data = struct->pop_data();

  SSL3_DEBUG_MSG("SSL.handshake: Client hello: %O\n", data);
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
      
      SSL3_DEBUG_MSG("Sending a server key exchange-message, "
                     "with a %d-bits key.\n", temp_key->rsa_size());
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
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;
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

  session->cipher_spec->sign(session, client_random + server_random, struct);
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
    // NOTE: To protect against version roll-back attacks,
    //       the version sent here MUST be the same as the
    //       one in the initial handshake!
    struct->put_uint(client_version[0], 1);
    struct->put_uint(client_version[1], 1);
    string random = context->random(46);
    struct->put_fix_string(random);
    string premaster_secret = struct->pop_data();
    session->master_secret = client_derive_master_secret(premaster_secret);

    array(.state) res = session->new_client_states(client_random,
						   server_random,version);
    pending_read_state = res[0];
    pending_write_state = res[1];

    data = (temp_key || session->rsa)->encrypt(premaster_secret);

    if(version[1] >= PROTOCOL_TLS_1_0)
      data=sprintf("%2H", [string(0..255)]data);
      
    break;
  case KE_dhe_dss:
  case KE_dhe_rsa:
  case KE_dh_anon:
    SSL3_DEBUG_MSG("FIXME: Not handled yet\n");
    anonymous = 1;
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

Packet certificate_verify_packet()
{

  ADT.struct struct = ADT.struct();

  .context cx = .context();
  cx->rsa = context->client_rsa;

  session->cipher_spec->sign(cx, handshake_messages, struct);

  return handshake_packet (HANDSHAKE_certificate_verify,
			  struct->pop_data());
  
}

int(-1..0) reply_new_session(array(int) cipher_suites,
			     array(int) compression_methods)
{
  reuse = 0;
  session = context->new_session();
  multiset(int) common_suites;

  SSL3_DEBUG_MSG("ciphers: me:\n%s, client:\n%s",
		 fmt_cipher_suites(context->preferred_suites),
                 fmt_cipher_suites(cipher_suites));
  common_suites = mkmultiset(cipher_suites & context->preferred_suites);
  SSL3_DEBUG_MSG("intersection:\n%s\n",
                 fmt_cipher_suites((array(int))common_suites));

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

  array(string) certs;
  SSL3_DEBUG_MSG("Selecting server key.\n");

  // populate the key to be used for the session.
  object key = select_server_key();
  SSL3_DEBUG_MSG("Selected server key: %O\n", key);

  if(Program.implements(object_program(key), Crypto.DSA))
  { 
    session->dsa = [object(Crypto.DSA)]key;
  }
  else
  {
    session->rsa = [object(Crypto.RSA)]key;
  }

  SSL3_DEBUG_MSG("Checking for Certificate.\n");

  if (certs = select_server_certificate())
  {
    SSL3_DEBUG_MSG("Sending Certificate.\n");
    send_packet(certificate_packet(certs));
  }
  else if (session->cipher_spec->sign != .Cipher.anon_sign)
    // Otherwise the server will just silently send an invalid
    // ServerHello sequence.
    error ("Certificate(s) missing.\n");

  Packet key_exchange = server_key_exchange_packet();

  if (key_exchange) {
    send_packet(key_exchange);
  }
  if (context->auth_level >= AUTHLEVEL_ask)
  {
    // we can send a certificate request packet, even if we don't have
    // any authorized issuers.
    send_packet(certificate_request_packet(context)); 
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
  if(version[1] == PROTOCOL_SSL_3_0) {
    return .Cipher.MACmd5(session->master_secret)->hash_master(handshake_messages + sender) +
      .Cipher.MACsha(session->master_secret)->hash_master(handshake_messages + sender);
  }
  else if(version[1] >= PROTOCOL_TLS_1_0) {
    return .Cipher.prf(session->master_secret, sender,
		       .Cipher.MACmd5()->hash_raw(handshake_messages)+
		       .Cipher.MACsha()->hash_raw(handshake_messages),12);
  }
}

Packet finished_packet(string sender)
{
  SSL3_DEBUG_MSG("Sending finished_packet, with sender=\""+sender+"\"\n" );
  string verify_data = hash_messages(sender);
  if (handshake_state >= STATE_client_min) {
    // We're the client.
    client_verify_data = verify_data;
  } else {
    // We're the server.
    server_verify_data = verify_data;
  }
  return handshake_packet(HANDSHAKE_finished, verify_data);
}

Packet certificate_request_packet(SSL.context context)
{
    /* Send a CertificateRequest message */
    ADT.struct struct = ADT.struct();
    struct->put_var_uint_array(context->preferred_auth_methods, 1, 1);

    int len; 

    // an empty certificate request is allowed.
    if(context->authorities_cache && sizeof(context->authorities_cache))
      len = `+(@ Array.map(context->authorities_cache,
			     lambda(Standards.X509.TBSCertificate s)
       { return sizeof(s->subject->get_der());} ));

    struct->put_uint(len + 2 * sizeof(context->authorities_cache), 2);
    foreach(context->authorities_cache, Standards.X509.TBSCertificate auth)
      struct->put_var_string(auth->subject->get_der(), 2);
    return handshake_packet(HANDSHAKE_certificate_request,
				 struct->pop_data());
}

Packet certificate_packet(array(string) certificates)
{
  ADT.struct struct = ADT.struct();
  int len = 0;

  if(certificates && sizeof(certificates))
    len = `+( @ Array.map(certificates, sizeof));
  //  SSL3_DEBUG_MSG("SSL.handshake: certificate_message size %d\n", len);
  struct->put_uint(len + 3 * sizeof(certificates), 3);
  foreach(certificates, string cert)
    struct->put_var_string(cert, 3);

  return handshake_packet(HANDSHAKE_certificate, struct->pop_data());

}

string server_derive_master_secret(string data)
{
  string premaster_secret;
  
  SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
                 session->ke_method);
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
      SSL3_DEBUG_MSG("SSL.handshake: Client uses implicit encoding if its DH-value.\n"
	     "               Hanging up.\n");
      send_packet(Alert(ALERT_fatal, ALERT_certificate_unknown, version[1]));
      return 0;
    }
    /* Fall through */
  case KE_dh_anon:
  {
    anonymous = 1;

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
     SSL3_DEBUG_MSG("encrypted premaster_secret: %O\n", data);
     if(version[1] >= PROTOCOL_TLS_1_0) {
       if(sizeof(data)-2 == data[0]*256+data[1]) {
	 premaster_secret = (temp_key || session->rsa)->decrypt(data[2..]);
       }
     } else {
       premaster_secret = (temp_key || session->rsa)->decrypt(data);
     }
     SSL3_DEBUG_MSG("premaster_secret: %O\n", premaster_secret);
     if (!premaster_secret
	 || (sizeof(premaster_secret) != 48)
	 || (premaster_secret[0] != 3)
	 || (premaster_secret[1] != client_version[1]))
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
		client_version[0], client_version[1]);
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

  if(version[1] == PROTOCOL_SSL_3_0) {
    foreach( ({ "A", "BB", "CCC" }), string cookie)
      res += md5->hash_raw(premaster_secret
			   + sha->hash_raw(cookie + premaster_secret 
					   + client_random + server_random));
  }
  else if(version[1] >= PROTOCOL_TLS_1_0) {
    res=.Cipher.prf(premaster_secret,"master secret",
		    client_random+server_random,48);
  }
  
  SSL3_DEBUG_MSG("master: %O\n", res);
  return res;
}

string client_derive_master_secret(string premaster_secret)
{
  string res = "";

  .Cipher.MACsha sha = .Cipher.MACsha();
  .Cipher.MACmd5 md5 = .Cipher.MACmd5();

  SSL3_DEBUG_MSG("Handshake.pike: in client_derive_master_secret is version[1]="+version[1]+"\n");

  if(version[1] == PROTOCOL_SSL_3_0) {
    foreach( ({ "A", "BB", "CCC" }), string cookie)
      res += md5->hash_raw(premaster_secret
			   + sha->hash_raw(cookie + premaster_secret 
					   + client_random + server_random));
  }
  else if(version[1] >= PROTOCOL_TLS_1_0) {
    res+=.Cipher.prf(premaster_secret,"master secret",client_random+server_random,48);
  }
  
  SSL3_DEBUG_MSG("bahmaster: %O\n", res);
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

  // if we're not requiring the certificate, and we don't provide one, 
  // that should be okay. 
  if((context->auth_level < AUTHLEVEL_require) && !sizeof(certs))
    return 1;

  // a lack of certificates when we reqiure and must verify the
  // certificates is probably a failure.
  if(!certs || !sizeof(certs))
    return 0;

  int issuer_known = 0;

  // step one: see if the issuer of the certificate is acceptable.
  // this means the issuer of the certificate must be one of the authorities.
  string r=Standards.PKCS.Certificate.get_certificate_issuer(certs[-1])
    ->get_der();

  // if we've got authorities, we need to check to see that the
  // provided cert is authorized. is this useful for server
  // connections???
  if(sizeof(context->authorities_cache))
  {
    foreach(context->authorities_cache, Standards.X509.TBSCertificate c)
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

  foreach(context->trusted_issuers_cache, array(Standards.X509.TBSCertificate) i)
  {
    // we want the first item, the top level
    auth[i[-1]->subject->get_der()] = i[-1]->public_key;
  }

  mapping result = Standards.X509.verify_certificate_chain(certs, auth, context->require_trust);

  if(result->verified)
  {
    session->cert_data = result;
    return 1;
  }

 return 0;
}

protected string fmt_cipher_suites(array(int) s)
{
  String.Buffer b = String.Buffer();
  mapping(int:string) ciphers = ([]);
  foreach([array(string)]indices(.Constants), string id)
    if( has_prefix(id, "SSL_") || has_prefix(id, "TLS_") )
      ciphers[.Constants[id]] = id;
  foreach(s, int c)
    b->sprintf("   %-6d: %s\n", c, ciphers[c]||"unknown");
  return (string)b;
}

//! Do handshake processing. Type is one of HANDSHAKE_*, data is the
//! contents of the packet, and raw is the raw packet received (needed
//! for supporting SSLv2 hello messages).
//!
//! This function returns 0 if hadshake is in progress, 1 if handshake
//! is finished, and -1 if a fatal error occurred. It uses the
//! send_packet() function to transmit packets.
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

	SSL3_DEBUG_MSG("SSL.session: CLIENT_HELLO\n");

       	if (
	  catch{
	  client_version = input->get_fix_uint_array(1, 2);
	  client_random = input->get_fix_string(32);
	  id = input->get_var_string(1);
	  cipher_len = input->get_uint(2);
	  cipher_suites = input->get_fix_uint_array(2, cipher_len/2);
	  compression_methods = input->get_var_uint_array(1, 1);
	  SSL3_DEBUG_MSG("STATE_server_wait_for_hello: received hello\n"
			 "version = %d.%d\n"
			 "id=%O\n"
			 "cipher suites:\n%s\n"
			 "compression methods: %O\n",
			 client_version[0], client_version[1],
			 id, fmt_cipher_suites(cipher_suites),
                         compression_methods);

	}
	  || (cipher_len & 1))
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: unexpected message\n",
			    backtrace()));
	  return -1;
	}
	if ((client_version[0] != PROTOCOL_major) ||
	    (client_version[1] < min_version)) {
	  SSL3_DEBUG_MSG("Unsupported version of SSL: %d.%d.\n",
			 @client_version);
	  send_packet(Alert(ALERT_fatal, ALERT_protocol_version, version[1],
			    "SSL.session->handle_handshake: Unsupported version.\n",
			    backtrace()));
	  return -1;
	}
	if (client_version[1] > version[1]) {
	  if (version[1]) {
	    SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			   "SSL 3.%d (aka TLS 1.%d).\n",
			   client_version[1], version[1], version[1]-1);
	  } else {
	    SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			   "SSL 3.%d.\n",
			   client_version[1], version[1]);
	  }
	} else if (version[1] > client_version[1]) {
	  if (client_version[1]) {
	    SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			   "SSL 3.%d (aka TLS 1.%d).\n",
			   version[1], client_version[1],
			   client_version[1]-1);
	  } else {
	    SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			   "SSL 3.%d.\n",
			   version[1], client_version[1]);
	  }
	  version[1] = client_version[1];
	}

	int missing_secure_renegotiation = secure_renegotiation;

	if (!input->is_empty()) {
	  ADT.struct extensions = ADT.struct(input->get_var_string(2));

	  while (!extensions->is_empty()) {
	    int extension_type = extensions->get_uint(2);
	    ADT.struct extension_data =
	      ADT.struct(extensions->get_var_string(2));
	    SSL3_DEBUG_MSG("SSL.connection->handle_handshake: "
			   "Got extension 0x%04x, %O (%d bytes).\n",
			   extension_type,
			   extension_data->buffer,
			   sizeof(extension_data->buffer));
          extensions:
	    switch(extension_type) {
	    case EXTENSION_server_name:
	      // RFC 4366 3.1 "Server Name Indication"
	      // Example: "\0\f\0\0\tlocalhost"
	      server_names = ({});
	      while (!extension_data->is_empty()) {
		ADT.struct server_name =
		  ADT.struct(extension_data->get_var_string(2));
		switch(server_name->get_uint(1)) {	// name_type
		case 0:	// host_name
		  server_names += ({ server_name->get_var_string(2) });
		  break;
		default:
		  // Ignore other NameTypes for now.
		  break;
		}
	      }
              SSL3_DEBUG_MSG("SNI extension: %O\n", server_names);
	      break;

	    case EXTENSION_renegotiation_info:
	      string renegotiated_connection =
		extension_data->get_var_string(1);
	      if ((renegotiated_connection != client_verify_data) ||
		  (handshake_finished && !secure_renegotiation)) {
		// RFC 5746 3.7: (secure_renegotiation)
		// The server MUST verify that the value of the
		// "renegotiated_connection" field is equal to the saved
		// client_verify_data value; if it is not, the server MUST
		// abort the handshake.
		//
		// RFC 5746 4.4: (!secure_renegotiation)
		// The server MUST verify that the "renegotiation_info"
		// extension is not present; if it is, the server MUST
		// abort the handshake.
		send_packet(Alert(ALERT_fatal, ALERT_handshake_failure,
				  version[1],
				  "SSL.session->handle_handshake: "
				  "Invalid renegotiation data.\n",
				  backtrace()));
		return -1;
	      }
	      secure_renegotiation = 1;
	      missing_secure_renegotiation = 0;
              SSL3_DEBUG_MSG("Renego extension: %O\n", renegotiated_connection);
	      break;

	    case EXTENSION_next_protocol_negotiation:
	      has_next_protocol_negotiation = 1;
              SSL3_DEBUG_MSG("NPN extension\n");
	      break;

            case EXTENSION_application_layer_protocol_negotiation:
              {
                has_application_layer_protocol_negotiation = 1;
                if( !context->advertised_protocols )
                  break;
                array(string) protocols = ({});
                while (!extension_data->is_empty()) {
                  string server_name = extension_data->get_var_string(1);
                  if( sizeof(server_name)==0 )
                  {
                    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure,
                                      version[1],
                                      "SSL.session->handle_handshake: "
                                      "Empty protocol in ALPN.\n",
                                      backtrace()));
                    return -1;
                  }
                  protocols += ({ server_name });
                }

                if( !sizeof(protocols) )
                {
                  // FIXME: What does an empty list mean? Ignore, no
                  // protocol failure or handshake failure?
                }

                // Although the protocol list is sent in client
                // preference order, it is the server preference that
                // wins.
                next_protocol = 0;
                foreach(context->advertised_protocols;; string prot)
                  if( has_value(protocols, prot) )
                    next_protocol = prot;
                if( !next_protocol )
                  send_packet(Alert(ALERT_fatal, ALERT_no_application_protocol,
                                    version[1],
                                    "SSL.session->handler_handshake: "
                                    "No compatible ALPN protocol.\n",
                                    backtrace()));
                SSL3_DEBUG_MSG("ALPN extension: %O %O\n", protocols, next_protocol);
              }
              break;

	    default:
#ifdef SSL3_DEBUG
              foreach(indices(.Constants), string id)
                if(has_prefix(id, "EXTENSION_") &&
                   .Constants[id]==extension_type)
                {
                  werror("Unhandled extension %s\n", id);
                  break extensions;
                }
              werror("Unknown extension %O\n", extension_type);
#endif
	      break;
	    }
	  }
	}
	if (missing_secure_renegotiation) {
	  // RFC 5746 3.7: (secure_renegotiation)
	  // The server MUST verify that the "renegotiation_info" extension is
	  // present; if it is not, the server MUST abort the handshake.
	  send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			    "SSL.session->handle_handshake: "
			    "Missing secure renegotiation extension.\n",
			    backtrace()));
	  return -1;
	}
	if (has_value(cipher_suites, TLS_empty_renegotiation_info_scsv)) {
	  if (secure_renegotiation || handshake_finished) {
	    // RFC 5746 3.7: (secure_renegotiation)
	    // When a ClientHello is received, the server MUST verify that it
	    // does not contain the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If
	    // the SCSV is present, the server MUST abort the handshake.
	    //
	    // RFC 5746 4.4: (!secure_renegotiation)
	    // When a ClientHello is received, the server MUST verify
	    // that it does not contain the
	    // TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If the SCSV is
	    // present, the server MUST abort the handshake.
	    send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			      "SSL.session->handle_handshake: "
			      "SCSV is present.\n",
			      backtrace()));
	    return -1;
	  } else {
	    // RFC 5746 3.6:
	    // When a ClientHello is received, the server MUST check if it
	    // includes the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If it
	    // does, set the secure_renegotiation flag to TRUE.
	    secure_renegotiation = 1;
	  }
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
            SSL3_DEBUG_MSG("SSL.handshake: Reusing session %O\n", id);
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
	  if(version[1] == PROTOCOL_SSL_3_0)
	    send_packet(finished_packet("SRVR"));
	  else if(version[1] >= PROTOCOL_TLS_1_0)
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
	SSL3_DEBUG_MSG("SSL.session: CLIENT_HELLO_V2\n");
        SSL3_DEBUG_MSG("SSL.handshake: SSL2 hello message received\n");

	int ci_len;	// Cipher specs length
	int id_len;	// Session id length
	int ch_len;	// Challenge length
	mixed err;
	if (err = catch{
	  client_version = input->get_fix_uint_array(1, 2);
	  ci_len = input->get_uint(2);
	  id_len = input->get_uint(2);
	  ch_len = input->get_uint(2);
	} || (ci_len % 3) || !ci_len || (id_len) || (ch_len < 16))
	{
          SSL3_DEBUG_MSG("SSL.handshake: Error decoding SSL2 handshake:\n"
                         "%s\n", err?describe_backtrace(err):"");
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	  return -1;
	}

	if ((client_version[0] != PROTOCOL_major) ||
	    (client_version[1] < min_version)) {
	  SSL3_DEBUG_MSG("Unsupported version of SSL: %d.%d.\n",
			 @client_version);
	  send_packet(Alert(ALERT_fatal, ALERT_protocol_version, version[1],
			    "SSL.session->handle_handshake: Unsupported version.\n",
			    backtrace()));
	  return -1;
	}
	if (client_version[1] > version[1]) {
	  if (version[1]) {
	    SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			   "SSL 3.%d (aka TLS 1.%d).\n",
			   client_version[1], version[1], version[1]-1);
	  } else {
	    SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			   "SSL 3.%d.\n",
			   client_version[1], version[1]);
	  }
	} else if (version[1] > client_version[1]) {
	  if (client_version[1]) {
	    SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			   "SSL 3.%d (aka TLS 1.%d).\n",
			   version[1], client_version[1],
			   client_version[1]-1);
	  } else {
	    SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			   "SSL 3.%d.\n",
			   version[1], client_version[1]);
	  }
	  version[1] = client_version[1];
	}

	string challenge;
	if (catch{
	    // FIXME: Support for restarting sessions?
	  cipher_suites = input->get_fix_uint_array(3, ci_len/3);
	  input->get_fix_string(id_len);	// session.
	  challenge = input->get_fix_string(ch_len);
	} || !input->is_empty()) 
	{
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
		      "SSL.session->handle_handshake: unexpected message\n",
		      backtrace()));
	  return -1;
	}

	if (has_value(cipher_suites, TLS_empty_renegotiation_info_scsv)) {
	  // RFC 5746 3.6:
	  // When a ClientHello is received, the server MUST check if it
	  // includes the TLS_EMPTY_RENEGOTIATION_INFO_SCSV SCSV.  If it
	  // does, set the secure_renegotiation flag to TRUE.
	  secure_renegotiation = 1;
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
    case HANDSHAKE_next_protocol:
     {
       next_protocol = input->get_var_string(1);
       handshake_messages += raw;
       return 1;
     }
    case HANDSHAKE_finished:
     {
       string my_digest;
       string digest;
       
	SSL3_DEBUG_MSG("SSL.session: FINISHED\n");

       if(version[1] == PROTOCOL_SSL_3_0) {
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
       } else if(version[1] >= PROTOCOL_TLS_1_0) {
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

       client_verify_data = digest;
       
       if (!reuse)
       {
	 send_packet(change_cipher_packet());
	 if(version[1] == PROTOCOL_SSL_3_0)
	   send_packet(finished_packet("SRVR"));
	 else if(version[1] >= PROTOCOL_TLS_1_0)
	   send_packet(finished_packet("server finished"));
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
      SSL3_DEBUG_MSG("SSL.session: CLIENT_KEY_EXCHANGE\n");

      if (certificate_state == CERT_requested)
      { /* Certificate must be sent before key exchange message */
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
	
        SSL3_DEBUG_MSG("certificate_state: %d\n", certificate_state);
      }
      // TODO: we need to determine whether the certificate has signing abilities.
      if (certificate_state == CERT_received)
      {
	handshake_state = STATE_server_wait_for_verify;
      }
      else
      {
	handshake_state = STATE_server_wait_for_finish;
	expect_change_cipher = 1;
      }

      break;
    case HANDSHAKE_certificate:
     {
       SSL3_DEBUG_MSG("SSL.session: CLIENT_CERTIFICATE\n");

       if (certificate_state != CERT_requested)
       {
	 send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			   "SSL.session->handle_handshake: unexpected message\n",
			   backtrace()));
	 return -1;
       }
       mixed e;
       if (e = catch {
	 int certs_len = input->get_uint(3);
#ifdef SSL3_DEBUG
	 werror("got %d certificate bytes\n", certs_len);
#else
	 certs_len;	// Fix warning.
#endif
	 array(string) certs = ({ });
	 while(!input->is_empty())
	   certs += ({ input->get_var_string(3) });

	  // we have the certificate chain in hand, now we must verify them.
          if((!sizeof(certs) && context->auth_level == AUTHLEVEL_require) || 
                     !verify_certificate_chain(certs))
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

       if(session->peer_certificate_chain && sizeof(session->peer_certificate_chain))
          certificate_state = CERT_received;
       else certificate_state = CERT_no_certificate;
       break;
     }
    }
    break;
  case STATE_server_wait_for_verify:
    // compute challenge first, then update handshake_messages /Sigge
    switch(type)
    {
    default:
      send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			"SSL.session->handle_handshake: unexpected message\n",
			backtrace()));
      return -1;
    case HANDSHAKE_certificate_verify:
      SSL3_DEBUG_MSG("SSL.session: CERTIFICATE_VERIFY\n");

      if (!rsa_message_was_bad)
      {
	int(0..1) verification_ok;
	if( catch
	{
	  Gmp.mpz signature = input->get_bignum();
	  ADT.struct handshake_messages_struct = ADT.struct();
	  handshake_messages_struct->put_fix_string(handshake_messages);
	  verification_ok = session->cipher_spec->verify(
	    session, "", handshake_messages_struct, signature);
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
      SSL3_DEBUG_MSG("SSL.session: SERVER_HELLO\n");

      handshake_messages += raw;
      string id;
      int cipher_suite, compression_method;

      version = input->get_fix_uint_array(1, 2);
      server_random = input->get_fix_string(32);
      id = input->get_var_string(1);
      cipher_suite = input->get_uint(2);
      compression_method = input->get_uint(1);

      if( !has_value(context->preferred_suites, cipher_suite) ||
	  !has_value(context->preferred_compressors, compression_method))
      {
	// The server tried to trick us to use some other cipher suite
	// or compression method than we wanted
	version = client_version + ({});
	send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			  "SSL.session->handle_handshake: handshake failure\n",
			  backtrace()));
	return -1;
      }

      if ((version[0] != PROTOCOL_major) || (version[1] < min_version)) {
	SSL3_DEBUG_MSG("Unsupported version of SSL: %d.%d.\n",
		       @version);
	version = client_version + ({});
	send_packet(Alert(ALERT_fatal, ALERT_protocol_version, version[1],
			  "SSL.session->handle_handshake: Unsupported version.\n",
			  backtrace()));
	return -1;
      }
      if (client_version[1] > version[1]) {
	if (version[1]) {
	  SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			 "SSL 3.%d (aka TLS 1.%d).\n",
			 client_version[1], version[1], version[1]-1);
	} else {
	  SSL3_DEBUG_MSG("Falling back client from SSL 3.%d to "
			 "SSL 3.%d.\n",
			 client_version[1], version[1]);
	}
      } else if (version[1] > client_version[1]) {
	if (client_version[1]) {
	  SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			 "SSL 3.%d (aka TLS 1.%d).\n",
			 version[1], client_version[1],
			 client_version[1]-1);
	} else {
	  SSL3_DEBUG_MSG("Falling back server from SSL 3.%d to "
			 "SSL 3.%d.\n",
			 version[1], client_version[1]);
	}
	version[1] = client_version[1];
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

      int missing_secure_renegotiation = secure_renegotiation;

      if (!input->is_empty()) {
	ADT.struct extensions = ADT.struct(input->get_var_string(2));

	while (!extensions->is_empty()) {
	  int extension_type = extensions->get_uint(2);
	  ADT.struct extension_data =
	    ADT.struct(extensions->get_var_string(2));
	  SSL3_DEBUG_MSG("SSL.connection->handle_handshake: "
			 "Got extension 0x%04x, %O (%d bytes).\n",
			 extension_type,
			 extension_data->buffer,
			 sizeof(extension_data->buffer));
	  switch(extension_type) {
	  case EXTENSION_renegotiation_info:
	    string renegotiated_connection = extension_data->get_var_string(1);
	    if ((renegotiated_connection !=
		 (client_verify_data + server_verify_data)) ||
		(handshake_finished && !secure_renegotiation)) {
	      // RFC 5746 3.5: (secure_renegotiation)
	      // The client MUST then verify that the first half of the
	      // "renegotiated_connection" field is equal to the saved
	      // client_verify_data value, and the second half is equal to the
	      // saved server_verify_data value.  If they are not, the client
	      // MUST abort the handshake.
	      //
	      // RFC 5746 4.2: (!secure_renegotiation)
	      // When the ServerHello is received, the client MUST
	      // verify that it does not contain the
	      // "renegotiation_info" extension. If it does, the client
	      // MUST abort the handshake. (Because the server has
	      // already indicated it does not support secure
	      // renegotiation, the only way that this can happen is if
	      // the server is broken or there is an attack.)
	      send_packet(Alert(ALERT_fatal, ALERT_handshake_failure,
				version[1],
				"SSL.session->handle_handshake: "
				"Invalid renegotiation data.\n",
				backtrace()));
	      return -1;
	    }
	    secure_renegotiation = 1;
	    missing_secure_renegotiation = 0;
	    break;
	  case EXTENSION_server_name:
	SSL3_DEBUG_MSG("SSL.handshake: Server sent Server Name extension, ignoring.\n");
            break;
	  default:
	    // RFC 5246 7.4.1.4:
	    // If a client receives an extension type in ServerHello
	    // that it did not request in the associated ClientHello, it
	    // MUST abort the handshake with an unsupported_extension
	    // fatal alert.
	    send_packet(Alert(ALERT_fatal, ALERT_unsupported_extension,
			      version[1],
			      "SSL.session->handle_handshake: "
			      "Unsupported extension.\n",
			      backtrace()));
	    return -1;
	  }
	}
      }
      if (missing_secure_renegotiation) {
	// RFC 5746 3.5:
	// When a ServerHello is received, the client MUST verify that the
	// "renegotiation_info" extension is present; if it is not, the
	// client MUST abort the handshake.
	send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			  "SSL.session->handle_handshake: "
			  "Missing secure renegotiation extension.\n",
			  backtrace()));
	return -1;
      }

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
	SSL3_DEBUG_MSG("SSL.session: CERTIFICATE\n");

      // we're anonymous, so no certificate is requred.
      if(anonymous)
         break;

      SSL3_DEBUG_MSG("Handshake: Certificate message received\n");
      int certs_len = input->get_uint(3); certs_len;
      array(string) certs = ({ });
      while(!input->is_empty())
	certs += ({ input->get_var_string(3) });

      // we have the certificate chain in hand, now we must verify them.
      if(!verify_certificate_chain(certs))
      {
        werror("Unable to verify peer certificate chain.\n");
        send_packet(Alert(ALERT_fatal, ALERT_bad_certificate, version[1],
			  "SSL.session->handle_handshake: bad certificate\n",
			  backtrace()));
  	return -1;
      }
      else
      {
        session->peer_certificate_chain = certs;
      }
      
      mixed error=catch
      {
	Standards.X509.Verifier public_key = Standards.X509.decode_certificate(
                session->peer_certificate_chain[0])->public_key;

	if(public_key->type == "rsa")
	  {
	    Crypto.RSA rsa = Crypto.RSA();
	    rsa->set_public_key(public_key->rsa->get_n(),
				public_key->rsa->get_e());
	    session->rsa = rsa;
	  }
	else
	  {
            SSL3_DEBUG_MSG("Other certificates than RSA not supported!\n");
	    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			      "SSL.session->handle_handshake: Unsupported certificate type\n",
			      backtrace()));
	    return -1;
	  }
      };

      if(error)
	{
          SSL3_DEBUG_MSG("Failed to decode certificate!\n");
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: Failed to decode certificate\n",
			    error));
	  return -1;
	}
      
      certificate_state = CERT_received;
      break;
      }

    case HANDSHAKE_server_key_exchange:
      {
	SSL3_DEBUG_MSG("SSL.session: SERVER_KEY_EXCHANGE\n");

	Gmp.mpz n = input->get_bignum();
	Gmp.mpz e = input->get_bignum();
	Gmp.mpz signature = input->get_bignum();
	ADT.struct temp_struct = ADT.struct();
	temp_struct->put_bignum(n);
	temp_struct->put_bignum(e);
	int verification_ok;
	if( catch{ verification_ok = session->cipher_spec->verify(
	  session, client_random + server_random, temp_struct, signature); }
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
	session->rsa = rsa;
	break;
      }

    case HANDSHAKE_certificate_request:
      SSL3_DEBUG_MSG("SSL.session: CERTIFICATE_REQUEST\n");

        // it is a fatal handshake_failure alert for an anonymous server to
        // request client authentication.
        if(anonymous)
        {
	  send_packet(Alert(ALERT_fatal, ALERT_handshake_failure, version[1],
			    "SSL.session->handle_handshake: anonymous server "
			    "requested authentication by certificate\n",
			    backtrace()));
	  return -1;
        }

        client_cert_types = input->get_var_uint_array(1, 1);
        client_cert_distinguished_names = ({});
        int num_distinguished_names = input->get_uint(2);
        if(num_distinguished_names)
        {
          ADT.struct s = ADT.struct(input->get_fix_string(num_distinguished_names));
          while(!s->is_empty())
          {
            object asn = Standards.ASN1.Decode.simple_der_decode((string)s->get_var_string(2));
            if(object_program(asn) != Standards.ASN1.Types.Sequence)
            {
                    send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
                            "SSL.session->handle_handshake: Badly formed Certificate Request.\n",
                            backtrace()));              
            }
            Standards.ASN1.Types.Sequence seq = [object(Standards.ASN1.Types.Sequence)]asn;
            client_cert_distinguished_names += ({ (string)Standards.PKCS.Certificate.get_dn_string( 
                                            seq ) }); 
            SSL3_DEBUG_MSG("got an authorized issuer: %O\n",
                           client_cert_distinguished_names[-1]);
           }
        }

      certificate_state = CERT_requested;
      break;

    case HANDSHAKE_server_hello_done:
      SSL3_DEBUG_MSG("SSL.session: SERVER_HELLO_DONE\n");

      /* Send Certificate, ClientKeyExchange, CertificateVerify and
       * ChangeCipherSpec as appropriate, and then Finished.
       */
      /* only send a certificate if it's been requested. */
      if(certificate_state == CERT_requested)
      {
        // okay, we have a list of certificate types and dns that are
        // acceptable to the remote server. we should weed out the certs
        // we have so that we only send certificates that match what they 
        // want.

        array(string) certs = context->client_certificate_selector(context, 
                                          client_cert_types, 
                                          client_cert_distinguished_names);
        if(!certs || !sizeof(certs))
          certs = ({});

#ifdef SSL3_DEBUG
        foreach(certs, string c)
        {
werror("sending certificate: " + Standards.PKCS.Certificate.get_dn_string(Standards.X509.decode_certificate(c)->subject) + "\n");
        }
#endif

	send_packet(certificate_packet(certs));
        if(!sizeof(certs))
          certificate_state = CERT_no_certificate;
        else
        {
          certificate_state = CERT_received; // we use this as a way of saying "the server received our certificate"
          session->certificate_chain = certs;
        }
      }


      {

      check_serv_cert: {
	  switch (session->cipher_spec->sign) {
	    case .Cipher.rsa_sign:
	      if (session->rsa) break check_serv_cert;
	      break;
	    case .Cipher.dsa_sign:
	      if (session->dsa) break check_serv_cert;
	      break;
	  }

          SSL3_DEBUG_MSG("Certificate message required from server.\n");
	  send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			    "SSL.session->handle_handshake: Certificate message missing\n",
			    backtrace()));
	  return -1;
	}

      Packet key_exchange = client_key_exchange_packet();

      if (key_exchange)
	send_packet(key_exchange);

      // FIXME: Certificate verify; we should redo this so it makes more sense
      if(certificate_state == CERT_received
          && sizeof(context->client_certificates) && context->client_rsa) 
         // we sent a certificate, so we should send the verification.
      {
         send_packet(certificate_verify_packet());
      }

      send_packet(change_cipher_packet());

      if(version[1] == PROTOCOL_SSL_3_0)
	send_packet(finished_packet("CLNT"));
      else if(version[1] >= PROTOCOL_TLS_1_0)
	send_packet(finished_packet("client finished"));
      
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
    } else {
      SSL3_DEBUG_MSG("SSL.session: FINISHED\n");

      string my_digest;
      if (version[1] == PROTOCOL_SSL_3_0) {
	server_verify_data = input->get_fix_string(36);
	my_digest = hash_messages("SRVR");
      } else if (version[1] >= PROTOCOL_TLS_1_0) {
	server_verify_data = input->get_fix_string(12);
	my_digest = hash_messages("server finished");
      }

      if (my_digest != server_verify_data) {
	SSL3_DEBUG_MSG("digests differ\n");
	send_packet(Alert(ALERT_fatal, ALERT_unexpected_message, version[1],
			  "SSL.session->handle_handshake: unexpected message\n",
			  backtrace()));
	return -1;
      }

      return 1;			// We're done shaking hands
    }
    }
  }
  //  SSL3_DEBUG_MSG("SSL.handshake: messages = %O\n", handshake_messages);
  return 0;
}

//! @param is_server
//!   Whether this is the server end of the connection or not.
//! @param ctx
//!   The context for the connection.
//! @param min_version
//!   Minimum version of SSL to support.
//!   Defaults to @[Constants.PROTOCOL_SSL_3_0].
//! @param max_version
//!   Maximum version of SSL to support.
//!   Defaults to @[Constants.PROTOCOL_minor].
void create(int is_server, void|SSL.context ctx,
	    void|ProtocolVersion min_version,
	    void|ProtocolVersion max_version)
{

#ifdef SSL3_PROFILING
  timestamp=time();
  Stdio.stdout.write("New...\n");
#endif

  if (zero_type(max_version) || (max_version < PROTOCOL_SSL_3_0) ||
      (max_version > PROTOCOL_minor)) {
    max_version = PROTOCOL_minor;
  }

  if (zero_type(min_version) || (min_version < PROTOCOL_SSL_3_0)) {
    min_version = PROTOCOL_SSL_3_0;
  } else if (min_version > max_version) {
    min_version = max_version;
  }

  this_program::min_version = min_version;

  version = ({ PROTOCOL_major, max_version });
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

#else // constant(SSL.Cipher.DHKeyExchange)
constant this_program_does_not_exist = 1;
#endif
