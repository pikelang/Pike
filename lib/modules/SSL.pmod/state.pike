#pike __REAL_VERSION__
// #pragma strict_types

// $Id$

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

string tls_pad(string data, int blocksize) {
  int plen=(blocksize-(sizeof(data)+1)%blocksize)%blocksize;
  return data + sprintf("%c",plen)*plen+sprintf("%c",plen);
}

string tls_unpad(string data) {
  int(0..255) plen=[int(0..255)]data[-1];

  string padding=reverse(data)[..plen];

  /* Check that the padding is correctly done. Required by TLS 1.1. */
  foreach(values(padding), int tmp)
    {
      if(tmp!=plen) {
	return UNDEFINED;	// Invalid padding.
      }
    }

  return data[..<plen+1];
}

//! Destructively decrypts a packet (including inflating and MAC-verification,
//! if needed). On success, returns the decrypted packet. On failure,
//! returns an alert packet. These cases are distinguished by looking
//! at the is_alert attribute of the returned packet.
Alert|.packet decrypt_packet(.packet packet, int version)
{
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
    if (! msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message, version);

    if (session->cipher_spec->cipher_type == CIPHER_block) {
      if(version==0) {
	// crypt->unpad() performs decrypt.
	if (catch { msg = crypt->unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      } else {
	msg = crypt->crypt(msg);

	// FIXME: TLS 1.1 recommends performing the hash check before
	//        sending the alerts to protect against timing attacks.

	if (catch { msg = tls_unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
	if (!msg) {
	  // TLS 1.1 requires a bad_record_mac alert on invalid padding.
	  return Alert(ALERT_fatal, ALERT_bad_record_mac, version);
	}
      }
    } else {
      msg = crypt->crypt(msg);
    }
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
	return Alert(ALERT_fatal, ALERT_bad_record_mac, version);
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
      return Alert(ALERT_fatal, ALERT_unexpected_message, version);
    packet->fragment = msg;
  }

  return [object(Alert)]packet->check_size(version) || packet;
}

//! Encrypts a packet (including deflating and MAC-generation).
Alert|.packet encrypt_packet(.packet packet, int version)
{
  string digest;
  packet->protocol_version = ({3, version});
  
  if (compress)
  {
    packet->fragment = [string]compress(packet->fragment);
  }

  if (mac)
    digest = mac->hash(packet, seq_num);
  else
    digest = "";

  seq_num++;

  if (crypt)
  {
    if (session->cipher_spec->cipher_type == CIPHER_block)
      {
	if(version==0) {
	  packet->fragment = crypt->crypt(packet->fragment + digest);
	  packet->fragment += crypt->pad();
	} else {
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
      } else {
	packet->fragment=crypt->crypt(packet->fragment + digest);
      }
  }
  else
    packet->fragment += digest;
  
  return [object(Alert)]packet->check_size(version, 2048) || packet;
}

#else // constant(SSL.Cipher.MACAlgorithm)
constant this_program_does_not_exist = 1;
#endif
