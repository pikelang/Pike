#pike __REAL_VERSION__
// #pragma strict_types

//! A connection switches from one set of state objects to another, one or
//! more times during its lifetime. Each state object handles a one-way
//! stream of packets, and operates in either decryption or encryption
//! mode.

#if constant(SSL.Cipher.MACAlgorithm)

import .Constants;

void create(object/*(.session)*/ s)
{
  session = s;
  seq_num = Gmp.mpz(0);
}

//! Information about the used algorithms.
object/*(.session)*/ session;

//! Message Authentication Code
.Cipher.MACAlgorithm mac;

//! Encryption or decryption object.
.Cipher.CipherAlgorithm crypt;

object compress;

//! 64-bit sequence number.
Gmp.mpz seq_num;    /* Bignum, values 0, .. 2^64-1 are valid */

//!
constant Alert = .alert;

//! TLS IV prefix length.
int tls_iv;

//! TLS 1.2 IV salt.
//! This is used as a prefix for the IV for the AEAD cipher algorithms.
string salt;

string tls_pad(string data, int blocksize) {
  int plen=(blocksize-(sizeof(data)+1)%blocksize)%blocksize;
  return data + sprintf("%c",plen)*plen+sprintf("%c",plen);
}

string tls_unpad(string data)
{
  int(0..255) plen=[int(0..255)]data[-1];

  string padding=reverse(data)[..plen];

  string ret = data[..<plen+1];

  /* NOTE: Perform some extra MAC operations, to avoid timing
   *       attacks on the length of the padding.
   *
   *       This is to alleviate the "Lucky Thirteen" attack:
   *       http://www.isg.rhul.ac.uk/tls/TLStiming.pdf
   *
   * NOTE: The digest quanta size is 64 bytes for all
   *       MAC-algorithms currently in use.
   */
  string junk = padding;
  if (!((sizeof(ret) +
	 mac->hash_header_size - session->cipher_spec->hash_size) & 63) ||
      !(sizeof(junk) & 63)) {
    // We're at the edge of a MAC block, so we need to
    // pad junk with an extra MAC block of data.
    //
    // NB: data will have at least 64 bytes if it's not empty.
    junk += data[<64..];
  }
  junk = mac && mac->hash_raw(junk);

  /* Check that the padding is correctly done. Required by TLS 1.1.
   *
   * Attempt to do it in a manner that takes constant time regardless
   * of the size of the padding.
   */
  for (int i = 0, j = 0; i < 255; i++,j++) {
    if (j >= sizeof (padding)) j = 0;
    if (padding[j] != plen) ret = UNDEFINED;	// Invalid padding.
  }

  return ret;
}

//! Destructively decrypts a packet (including inflating and MAC-verification,
//! if needed). On success, returns the decrypted packet. On failure,
//! returns an alert packet. These cases are distinguished by looking
//! at the is_alert attribute of the returned packet.
Alert|.packet decrypt_packet(.packet packet, ProtocolVersion version)
{
  /* NOTE: TLS 1.1 recommends performing the hash check before
   *       sending the alerts to protect against timing attacks.
   *
   *       This is also needed to alleviate the "Lucky Thirteen" attack.
   *
   *       We thus delay sending of any alerts to the end of the
   *       function, and attempt to make the same amount of work
   *       even if we have already detected a failure.
   */
  object(Alert) alert;

#ifdef SSL3_DEBUG_CRYPT
  werror("SSL.state->decrypt_packet (3.%d, type: %d): data = %O\n",
	 version, packet->content_type, packet->fragment);
#endif
  
  if (crypt)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying decrypt..\n");
    //    werror("SSL.state: The encrypted packet is:%O\n",packet->fragment);
    werror("sizeof of the encrypted packet is:"+sizeof(packet->fragment)+"\n");
#endif

    string msg = packet->fragment;
    if (!msg) {
      packet->fragment = #string "alert.pike";	// Some junk data.
      alert = Alert(ALERT_fatal, ALERT_unexpected_message, version);
    } else {
      switch(session->cipher_spec->cipher_type) {
      case CIPHER_stream:
	msg = crypt->crypt(msg);
	break;
      case CIPHER_block:
	if(version == PROTOCOL_SSL_3_0) {
	  // crypt->unpad() performs decrypt.
	  if (catch { msg = crypt->unpad(msg); })
	    alert = Alert(ALERT_fatal, ALERT_unexpected_message, version);
	} else if (version >= PROTOCOL_TLS_1_0) {
	  msg = crypt->crypt(msg);

	  if (catch { msg = tls_unpad(msg); })
	    alert = Alert(ALERT_fatal, ALERT_unexpected_message, version);
	  else if (!msg) {
	    // TLS 1.1 requires a bad_record_mac alert on invalid padding.
	    alert = Alert(ALERT_fatal, ALERT_bad_record_mac, version);
	  }
	}
	break;
      case CIPHER_aead:
	// NB: Only valid in TLS 1.2 and later.
	// The message consists of explicit_iv + crypted-msg + digest.
	string iv = salt + msg[..session->cipher_spec->explicit_iv_size-1];
	int digest_size = crypt->block_size();
	string digest = msg[<digest_size-1..];
	crypt->set_iv(iv);
	string auth_data = sprintf("%8c%c%c%c%2c",
				   seq_num, packet->content_type,
				   packet->protocol_version[0],
				   packet->protocol_version[1],
				   sizeof(msg) -
				   (session->cipher_spec->explicit_iv_size +
				    digest_size));
	crypt->update(auth_data);
	msg = crypt->crypt(msg[session->cipher_spec->explicit_iv_size..
			       <digest_size]);
	seq_num += 1;
	if (digest != crypt->digest()) {
	  // Bad digest.
#ifdef SSL3_DEBUG
	  werror("Failed AEAD-verification!!\n");
#endif
	  alert = Alert(ALERT_fatal, ALERT_bad_record_mac, version);
	}
	break;
      }
    }
    if (!msg) msg = packet->fragment;
    packet->fragment = msg;
  }

#ifdef SSL3_DEBUG_CRYPT
  werror("SSL.state: Decrypted_packet %O\n", packet->fragment);
#endif

  if (tls_iv) {
    // TLS 1.1 IV. RFC 4346 6.2.3.2:
    // The decryption operation for all three alternatives is the same.
    // The receiver decrypts the entire GenericBlockCipher structure and
    // then discards the first cipher block, corresponding to the IV
    // component.
    packet->fragment = packet->fragment[tls_iv..];
  }

  if (mac)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying mac verification...\n");
#endif
    int length = sizeof(packet->fragment) - session->cipher_spec->hash_size;
    string digest = packet->fragment[length ..];
    packet->fragment = packet->fragment[.. length - 1];
    
    if (digest != mac->hash(packet, seq_num))
      {
#ifdef SSL3_DEBUG
	werror("Failed MAC-verification!!\n");
#endif
#ifdef SSL3_DEBUG_CRYPT
	werror("Expected digest: %O\n"
	       "Calculated digest: %O\n"
	       "Seqence number: %O\n",
	       digest, mac->hash(packet, seq_num), seq_num);
#endif
	alert = alert || Alert(ALERT_fatal, ALERT_bad_record_mac, version);
      }
    seq_num += 1;
  }

  if (compress)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying decompression...\n");
#endif
    string msg = [string]compress(packet->fragment);
    if (!msg)
      alert = alert || Alert(ALERT_fatal, ALERT_unexpected_message, version);
    packet->fragment = msg;
  }

  if (alert) return alert;

  return [object(Alert)]packet->check_size(version) || packet;
}

//! Encrypts a packet (including deflating and MAC-generation).
Alert|.packet encrypt_packet(.packet packet, ProtocolVersion version)
{
  string digest;
  packet->protocol_version = ({ PROTOCOL_major, version});
  
  if (compress)
  {
    packet->fragment = [string]compress(packet->fragment);
  }

  if (mac)
    digest = mac->hash(packet, seq_num);
  else
    digest = "";

  if (crypt)
  {
    switch(session->cipher_spec->cipher_type) {
    case CIPHER_stream:
      packet->fragment=crypt->crypt(packet->fragment + digest);
      break;
    case CIPHER_block:
      if(version == PROTOCOL_SSL_3_0) {
	packet->fragment = crypt->crypt(packet->fragment + digest);
	packet->fragment += crypt->pad();
      } else if (version >= PROTOCOL_TLS_1_0) {
	packet->fragment = tls_pad(packet->fragment+digest,
				   crypt->block_size());
	if (tls_iv) {
	  // RFC 4346 6.2.3.2.2:
	  // Generate a cryptographically strong random number R of length
	  // CipherSpec.block_length and prepend it to the plaintext prior
	  // to encryption.
	  string iv = Crypto.Random.random_string(tls_iv);
	  crypt->set_iv(iv);
	  packet->fragment = iv + crypt->crypt(packet->fragment);
	} else {
	  packet->fragment = crypt->crypt(packet->fragment);
	}
      }
      break;
    case CIPHER_aead:
      // RFC 5288 3:
      // The nonce_explicit MAY be the 64-bit sequence number.
      // FIXME: Do we need to pay attention to threads here?
      string explicit_iv =
	sprintf("%*c", session->cipher_spec->explicit_iv_size, seq_num);
      crypt->set_iv(salt + explicit_iv);
      string auth_data = sprintf("%8c%c%c%c%2c",
				 seq_num, packet->content_type,
				 packet->protocol_version[0],
				 packet->protocol_version[1],
				 sizeof(packet->fragment));
      crypt->update(auth_data);
      packet->fragment = explicit_iv + crypt->crypt(packet->fragment);
      packet->fragment += crypt->digest();
      break;
    }
  }
  else
    packet->fragment += digest;
  
  seq_num++;

  return [object(Alert)]packet->check_size(version, 2048) || packet;
}

#else // constant(SSL.Cipher.MACAlgorithm)
constant this_program_does_not_exist = 1;
#endif
