#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else
#define SSL3_DEBUG_MSG(X ...)
#endif

import .Constants;
inherit .Connection;

#define Packet .Packet

Packet client_hello()
{
  ADT.struct struct = ADT.struct();
  /* Build client_hello message */
  client_version = version;
  struct->put_uint(client_version, 2); /* version */

  // The first four bytes of the client_random is specified to be the
  // timestamp on the client side. This is to guard against bad random
  // generators, where a client could produce the same random numbers
  // if the seed is reused. This argument is flawed, since a broken
  // random generator will make the connection insecure anyways. The
  // standard explicitly allows these bytes to not be correct, so
  // sending random data instead is safer and reduces client
  // fingerprinting.
  client_random = context->random(32);

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
  SSL3_DEBUG_MSG("Client ciphers:\n%s",
                 .Constants.fmt_cipher_suites(cipher_suites));
  compression_methods = context->preferred_compressors;

  int cipher_len = sizeof(cipher_suites)*2;
  struct->put_uint(cipher_len, 2);
  struct->put_fix_uint_array(cipher_suites, 2);
  struct->put_var_uint_array(compression_methods, 1, 1);

  ADT.struct extensions = ADT.struct();

  if (secure_renegotiation) {
    // RFC 5746 3.4:
    // The client MUST include either an empty "renegotiation_info"
    // extension, or the TLS_EMPTY_RENEGOTIATION_INFO_SCSV signaling
    // cipher suite value in the ClientHello.  Including both is NOT
    // RECOMMENDED.
    ADT.struct extension = ADT.struct();
    extension->put_var_string(client_verify_data, 1);
    extensions->put_uint(EXTENSION_renegotiation_info, 2);

    extensions->put_var_string(extension->pop_data(), 2);
  }

  if (sizeof(context->ecc_curves)) {
    // RFC 4492 5.1:
    // The extensions SHOULD be sent along with any ClientHello message
    // that proposes ECC cipher suites.
    ADT.struct extension = ADT.struct();
    foreach(context->ecc_curves, int curve) {
      extension->put_uint(curve, 2);
    }
    extension->put_var_string(extension->pop_data(), 2);
    extensions->put_uint(EXTENSION_elliptic_curves, 2);
    extensions->put_var_string(extension->pop_data(), 2);

    extension->put_uint(POINT_uncompressed, 1);
    extension->put_var_string(extension->pop_data(), 1);
    extensions->put_uint(EXTENSION_ec_point_formats, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  }

  // We always attempt to enable the heartbeat extension.
  {
    // RFC 6420
    ADT.struct extension = ADT.struct();
    extension->put_uint(HEARTBEAT_MODE_peer_allowed_to_send, 1);
    extensions->put_uint(EXTENSION_heartbeat, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  }

  if (client_version >= PROTOCOL_TLS_1_2) {
    // RFC 5246 7.4.1.4.1:
    // If the client supports only the default hash and signature algorithms
    // (listed in this section), it MAY omit the signature_algorithms
    // extension.  If the client does not support the default algorithms, or
    // supports other hash and signature algorithms (and it is willing to
    // use them for verifying messages sent by the server, i.e., server
    // certificates and server key exchange), it MUST send the
    // signature_algorithms extension, listing the algorithms it is willing
    // to accept.

    // We list all hashes and signature formats that we support.
    ADT.struct extension = ADT.struct();
    extension->put_var_string(get_signature_algorithms(), 2);
    extensions->put_uint(EXTENSION_signature_algorithms, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  }

  if(context->client_use_sni)
  {
    ADT.struct extension = ADT.struct();
    if(context->client_server_names)
    {
      foreach(context->client_server_names;; string(0..255) server_name)
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
    array(string) prots = context->advertised_protocols;
    ADT.struct extension = ADT.struct();
    extension->put_uint( [int]Array.sum(Array.map(prots, sizeof)) +
                         sizeof(prots), 2);
    foreach(context->advertised_protocols;; string(0..255) proto)
      extension->put_var_string(proto, 1);

    SSL3_DEBUG_MSG("SSL.handshake: Adding ALPN extension.\n");
    extensions->put_uint(EXTENSION_application_layer_protocol_negotiation, 2);
    extensions->put_var_string(extension->pop_data(), 2);
  }

  // When the client HELLO packet data is in the range 256-511 bytes
  // f5 SSL terminators will intepret it as SSL2 requiring an
  // additional 8k of data, which will cause the connection to hang.
  // The solution is to pad the package to more than 511 bytes using a
  // dummy exentsion.
  // Reference: draft-agl-tls-padding
  int packet_size = sizeof(struct)+sizeof(extensions)+2;
  if(packet_size>255 && packet_size<512)
  {
    int padding = max(0, 512-packet_size-4);
    SSL3_DEBUG_MSG("SSL.handshake: Adding %d bytes of padding.\n",
                   padding);
    extensions->put_uint(EXTENSION_padding, 2);
    extensions->put_var_string("\0"*padding, 2);
  }

  if(sizeof(extensions))
    struct->put_var_string(extensions->pop_data(), 2);

  string data = struct->pop_data();

  SSL3_DEBUG_MSG("SSL.handshake: Client hello: %q\n", data);
  return handshake_packet(HANDSHAKE_client_hello, data);
}

protected void create(.context ctx)
{
  ::create(ctx);
  handshake_state = STATE_client_wait_for_hello;
  handshake_messages = "";
  session = context->new_session();
  send_packet(client_hello());
}