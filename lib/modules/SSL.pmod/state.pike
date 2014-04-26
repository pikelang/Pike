#pike __REAL_VERSION__
// #pragma strict_types
#require constant(SSL.Cipher)

//! A connection switches from one set of state objects to another, one or
//! more times during its lifetime. Each state object handles a one-way
//! stream of packets, and operates in either decryption or encryption
//! mode.

import .Constants;

//!
constant Alert = .alert;

function(int, int, string|void: Alert) alert;

protected void create(object/*(.connection)*/ con)
{
  connection = con;
  alert = con->Alert;
}

//! Information about the used algorithms.
object/*(.connection)*/ connection;

//! Message Authentication Code
.Cipher.MACAlgorithm mac;

//! Encryption or decryption object.
.Cipher.CipherAlgorithm crypt;

object compress;

//! 64-bit sequence number.
int seq_num = 0;    /* Bignum, values 0, .. 2^64-1 are valid */

//! TLS IV prefix length.
int tls_iv;

//! TLS 1.2 IV salt.
//! This is used as a prefix for the IV for the AEAD cipher algorithms.
string salt;

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
  object(Alert) fail;

#ifdef SSL3_DEBUG_CRYPT
  werror("SSL.state->decrypt_packet (3.%d, type: %d): data = %O\n",
	 version & 0xff, packet->content_type, packet->fragment);
#endif

  int hmac_size = mac && (connection->session->truncated_hmac ? 10 :
			  connection->session->cipher_spec?->hash_size);
  int padding;

  if (crypt)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying decrypt...\n");
    //    werror("SSL.state: The encrypted packet is:%O\n",packet->fragment);
    werror("sizeof of the encrypted packet is:"+sizeof(packet->fragment)+"\n");
#endif

    string msg = packet->fragment;
    if (!msg) {
      packet->fragment = #string "alert.pike";	// Some junk data.
      fail = alert(ALERT_fatal, ALERT_unexpected_message,
		   "SSL.state: Failed to get fragment.\n");
    } else {
      switch(connection->session->cipher_spec->cipher_type) {
      case CIPHER_stream:

        // If data is too small, we can safely abort early.
        if( sizeof(msg) < hmac_size+1 )
          return alert(ALERT_fatal, ALERT_unexpected_message,
		       "SSL.state: Too short message.\n");

	msg = crypt->crypt(msg);
	break;

      case CIPHER_block:

        // If data is too small or doesn't match block size, we can
        // safely abort early.
        if( sizeof(msg) < hmac_size+1 || sizeof(msg) % crypt->block_size() )
          return alert(ALERT_fatal, ALERT_unexpected_message,
		       "SSL.state: Too short message.\n");

	if(version == PROTOCOL_SSL_3_0) {
	  // crypt->unpad() performs decrypt.
	  if (catch { msg = crypt->unpad(msg, Crypto.PAD_SSL); })
	    fail = alert(ALERT_fatal, ALERT_unexpected_message,
			 "SSL.state: Invalid padding.\n");
	} else if (version >= PROTOCOL_TLS_1_0) {

#ifdef SSL3_DEBUG_CRYPT
	  werror("SSL.state: Decrypted message: %O.\n", msg);
#endif
	  if (catch { msg = crypt->unpad(msg, Crypto.PAD_TLS); }) {
            fail = alert(ALERT_fatal, ALERT_unexpected_message,
			 "SSL.state: Invalid padding.\n");
          } else if (!msg) {
	    // TLS 1.1 requires a bad_record_mac alert on invalid padding.
            // Note that mac will still be calculated below even if
            // padding was wrong, to mitigate Lucky Thirteen attacks.
	    fail = alert(ALERT_fatal, ALERT_bad_record_mac,
			 "SSL.state: Invalid padding.\n");
	  }
	}
	break;

      case CIPHER_aead:

	// NB: Only valid in TLS 1.2 and later.
	// The message consists of explicit_iv + crypted-msg + digest.
	string iv = salt + msg[..connection->session->cipher_spec->explicit_iv_size-1];
	int digest_size = crypt->digest_size();
	string digest = msg[<digest_size-1..];
	crypt->set_iv(iv);
	string auth_data = sprintf("%8c%c%2c%2c",
				   seq_num, packet->content_type,
				   packet->protocol_version,
				   sizeof(msg) -
				   (connection->session->cipher_spec->explicit_iv_size +
				    digest_size));
	crypt->update(auth_data);
	msg = crypt->crypt(msg[connection->session->cipher_spec->explicit_iv_size..
			       <digest_size]);
	seq_num++;
	if (digest != crypt->digest()) {
	  // Bad digest.
	  fail = alert(ALERT_fatal, ALERT_bad_record_mac,
		       "Failed AEAD-verification!!\n");
	}
	break;
      }
    }
    if (!msg) msg = packet->fragment;
    padding = sizeof(packet->fragment) - sizeof(msg);
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
    int length = sizeof(packet->fragment) - hmac_size;
    string digest = packet->fragment[length ..];
    packet->fragment = packet->fragment[.. length - 1];

   /* NOTE: Perform some extra MAC operations, to avoid timing
    *       attacks on the length of the padding.
    *
    *       This is to alleviate the "Lucky Thirteen" attack:
    *       http://www.isg.rhul.ac.uk/tls/TLStiming.pdf
    */
    int block_size = mac->block_size();
    string pad_string = "\0"*block_size;
    string junk = pad_string[<padding-1..];
    if (!((sizeof(packet->fragment) +
           mac->hash_header_size - connection->session->cipher_spec->hash_size) %
          block_size ) ||
        !(padding % block_size)) {
      // We're at the edge of a MAC block, so we need to
      // pad junk with an extra MAC block of data.
      junk += pad_string;
    }
    junk = mac->hash_raw(junk);

    if (digest != mac->hash_packet(packet, seq_num)[..hmac_size-1])
      {
#ifdef SSL3_DEBUG
	werror("Failed MAC-verification!!\n");
#endif
#ifdef SSL3_DEBUG_CRYPT
	werror("Expected digest: %O\n"
	       "Calculated digest: %O\n"
	       "Seqence number: %O\n",
	       digest, mac->hash_packet(packet, seq_num), seq_num);
#endif
	fail = fail || alert(ALERT_fatal, ALERT_bad_record_mac,
			     "Bad MAC.\n");
      }
    seq_num++;
  }

  if (compress)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying decompression...\n");
#endif
    string msg = [string]compress(packet->fragment);
    if (!msg)
      fail = fail || alert(ALERT_fatal, ALERT_unexpected_message,
			   "Invalid compression.\n");
    packet->fragment = msg;
  }

  if (fail) return fail;

  return [object(Alert)]packet->check_size(version) || packet;
}

//! Encrypts a packet (including deflating and MAC-generation).
Alert|.packet encrypt_packet(.packet packet, ProtocolVersion version)
{
  string digest;
  packet->protocol_version = version;
  
  if (compress)
  {
    packet->fragment = [string]compress(packet->fragment);
  }

  if (mac) {
    digest = mac->hash_packet(packet, seq_num);
    if (connection->session->truncated_hmac)
      digest = digest[..9];
  } else
    digest = "";

  if (crypt)
  {
    switch(connection->session->cipher_spec->cipher_type) {
    case CIPHER_stream:
      packet->fragment=crypt->crypt(packet->fragment + digest);
      break;
    case CIPHER_block:
      if(version == PROTOCOL_SSL_3_0) {
	packet->fragment = crypt->crypt(packet->fragment + digest);
	packet->fragment += crypt->pad(Crypto.PAD_SSL);
      } else if (version >= PROTOCOL_TLS_1_0) {
	if (tls_iv) {
	  // RFC 4346 6.2.3.2.2:
	  // Generate a cryptographically strong random number R of length
	  // CipherSpec.block_length and prepend it to the plaintext prior
	  // to encryption.
	  string iv = Crypto.Random.random_string(tls_iv);
	  crypt->set_iv(iv);
	  packet->fragment = iv + crypt->crypt(packet->fragment) + crypt->crypt(digest) + crypt->pad(Crypto.PAD_TLS);
	} else {
          packet->fragment = crypt->crypt(packet->fragment) + crypt->crypt(digest) + crypt->pad(Crypto.PAD_TLS);
        }
      }
      break;
    case CIPHER_aead:
      // RFC 5288 3:
      // The nonce_explicit MAY be the 64-bit sequence number.
      // FIXME: Do we need to pay attention to threads here?
      string explicit_iv =
	sprintf("%*c", connection->session->cipher_spec->explicit_iv_size,
		seq_num);
      crypt->set_iv(salt + explicit_iv);
      string auth_data = sprintf("%8c%c%2c%2c",
				 seq_num, packet->content_type,
				 packet->protocol_version,
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
