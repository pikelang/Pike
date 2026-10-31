#pike __REAL_VERSION__
// #pragma strict_types
#require constant(SSL.Cipher)
#include "tls.h"

//! The state object handles a one-way stream of packets, and operates
//! in either decryption or encryption mode. A connection switches
//! from one set of state objects to another, one or more times during
//! its lifetime.

import ".";
import Constants;

function(int, int, string|void: Alert) alert;

protected void create(Connection con)
{
  session = con->session;
  alert = con->alert;
  version = con->version;
}

//! The negotiated protocol version, as of when this state was
//! created.
//!
//! @note
//!   This is distinct from a @[Packet]'s @expr{protocol_version@},
//!   which is always clamped to @expr{PROTOCOL_TLS_1_2@} on the wire
//!   for TLS 1.3 and later (@rfc{8446:5.1@}) -- this field is what
//!   @[decrypt_packet()]/@[encrypt_packet()] use to decide whether
//!   to apply TLS 1.3 record-protection rules.
ProtocolVersion version;

//! Information about the used algorithms.
Session session;

//! Message Authentication Code
Cipher.MACAlgorithm mac;

//! Encryption or decryption object.
Cipher.CipherAlgorithm crypt;

function(string(8bit):string(8bit)) compress;

//! 64-bit sequence number.
int next_seq_num = 0;    /* Bignum, values 0, .. 2^64-1 are valid */

//! TLS 1.3 and later (@rfc{9846:5.5@}): edge-triggered latches for
//! @[check_aead_usage_limit()], so each outcome fires at most once
//! per @[State] object (a fresh one is always built on rekey, so
//! these never need resetting).
protected int(0..1) aead_limit_rekey_signaled;
protected int(0..1) aead_limit_close_signaled;

//! TLS IV prefix length.
int tls_iv;

//! TLS 1.2 IV salt.
//! This is used as a prefix for the IV for the AEAD cipher algorithms.
string salt;

//! Destructively decrypts a packet (including inflating and MAC-verification,
//! if needed). On success, returns the decrypted packet. On failure,
//! returns an alert packet. These cases are distinguished by looking
//! at the is_alert attribute of the returned packet.
Alert|Packet decrypt_packet(Packet packet)
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
  Alert fail;
  string data = packet->fragment;

  if (!data)
  {
    // packet->fragment is zero when the packet format is illegal,
    // so returning early is safe. We should never get to this
    // though, as decrypt_packet isn't called from connection if the
    // fragment isn't successfully parsed.
    return alert(ALERT_fatal, ALERT_unexpected_message,
                 "SSL.State: Failed to get fragment.\n");
  }

  int hmac_size = mac && (session->truncated_hmac ? 10 :
			  session->cipher_spec->?hash_size);
  int padding;

  if (undefinedp(packet->seq_num) && (hmac_size || crypt)) {
    packet->seq_num = next_seq_num++;
  }
  SSL3_DEBUG_MSG("DECRYPT: Packet #%O\n", packet->seq_num);

  SSL3_DEBUG_CRYPT_MSG("SSL.State->decrypt_packet "
                       "(3.%d, type: %d, seq_num: %d): data = %O\n",
                       version & 0xff, packet->content_type,
		       packet->seq_num, data);

  if (hmac_size && session->encrypt_then_mac) {
    string(8bit) digest = data[<hmac_size-1..];
    data = data[..<hmac_size];

    // Set data without HMAC. This never returns an Alert as the data
    // is smaller.
    packet->set_encrypted(data);

    if (mac->hash_packet(packet)[..hmac_size-1] != digest) {
      // Bad digest.
      SSL3_DEBUG_MSG("Failed MAC-verification!!\n");
      SSL3_DEBUG_CRYPT_MSG("Expected digest: %O\n"
                           "Calculated digest: %O\n"
                           "Seqence number: %O\n",
                           digest,
                           mac->hash_packet(packet)[..hmac_size-1],
                           packet->seq_num);
      return alert(ALERT_fatal, ALERT_bad_record_mac,
		   "Bad MAC-verification.\n");
    }
    hmac_size = 0;
  }

  if (crypt)
  {
    SSL3_DEBUG_CRYPT_MSG("SSL.State: Trying decrypt...\n");
    SSL3_DEBUG_CRYPT_MSG("encrypted packet size is %d\n", sizeof(data));

    string msg = data;

    switch(session->cipher_spec->cipher_type) {
    case CIPHER_stream:

      // If data is too small, we can safely abort early.
      if( sizeof(msg) < hmac_size+1 )
        return alert(ALERT_fatal, ALERT_unexpected_message,
                     "SSL.State: Too short message.\n");

      msg = crypt->crypt(msg);
      break;

    case CIPHER_block:

      // If data is too small or doesn't match block size, we can
      // safely abort early.
      if( sizeof(msg) < hmac_size+1 || sizeof(msg) % crypt->block_size() )
        return alert(ALERT_fatal, ALERT_unexpected_message,
                     "SSL.State: Too short message.\n");

      if(version == PROTOCOL_SSL_3_0) {
        // crypt->unpad() performs decrypt.
        if (catch { msg = crypt->unpad(msg, Crypto.PAD_SSL); })
          fail = alert(ALERT_fatal, ALERT_unexpected_message,
                       "SSL.State: Invalid padding.\n");
      } else if (version >= PROTOCOL_TLS_1_0) {

        SSL3_DEBUG_CRYPT_MSG("SSL.State: Decrypted message: %O.\n", msg);
        if (catch { msg = crypt->unpad(msg, Crypto.PAD_TLS); }) {
          fail = alert(ALERT_fatal, ALERT_unexpected_message,
                       "SSL.State: Invalid padding.\n");
        } else if (!msg) {
          // TLS 1.1 requires a bad_record_mac alert on invalid padding.
          // Note that mac will still be calculated below even if
          // padding was wrong, to mitigate Lucky Thirteen attacks.
          fail = alert(ALERT_fatal, ALERT_bad_record_mac,
                       "SSL.State: Invalid padding.\n");
        }
      }
      break;

    case CIPHER_aead:

      string iv;
      if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.3: the nonce is the static per-direction write
        // IV (the whole of it -- see Cipher.pmod's TLS 1.3 iv_size
        // override, salt is never just a "salt" prefix here) XORed
        // with the sequence number, left-padded with zeros to the
        // IV's length. No nonce octets are sent on the wire.
        iv = salt ^ sprintf("%*c", sizeof(salt), packet->seq_num);
      } else if (session->cipher_spec->explicit_iv_size) {
        // The message consists of explicit_iv + crypted-msg + digest.
        iv = salt + msg[..session->cipher_spec->explicit_iv_size-1];
      } else {
        // ChaCha20-POLY1305 uses an implicit iv, and no salt,
        // but we've generalized it here to allow for ciphers
        // using salt and implicit iv.
        iv = sprintf("%s%*c", salt, crypt->iv_size() - sizeof(salt),
                     packet->seq_num);
      }
      SSL3_DEBUG_CRYPT_MSG("SSL.State: AEAD IV: %O.\n", iv);

      int digest_size = crypt->digest_size();
      string digest = msg[<digest_size-1..];
      crypt->set_iv(iv);
      string auth_data;
      if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.2: the additional data is exactly the record
        // header as sent on the wire (opaque_type, legacy_record_
        // version, length) -- no sequence number (that only enters
        // via the nonce above).
        auth_data = sprintf("%c%2c%2c", packet->content_type,
                            packet->protocol_version, sizeof(msg));
      } else {
        auth_data = sprintf("%8c%c%2c%2c",
                            packet->seq_num, packet->content_type, version,
                            sizeof(msg) -
                            (session->cipher_spec->explicit_iv_size +
                             digest_size));
      }
      SSL3_DEBUG_CRYPT_MSG("SSL.State: AEAD Auth data: %O.\n", auth_data);

      crypt->update(auth_data);
      msg = crypt->crypt(msg[session->cipher_spec->explicit_iv_size..
                             <digest_size]);
      SSL3_DEBUG_CRYPT_MSG("SSL.State: Decrypted message: %O.\n", msg);
      if (digest != crypt->digest()) {
        // Bad digest.
        fail = alert(ALERT_fatal, ALERT_bad_record_mac,
                     "Failed AEAD-verification!!\n");
      } else if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.2: TLSInnerPlaintext -- the true content type is
        // the last non-zero octet, with any zero octets after it
        // being padding. The outer record's content type stays
        // application_data on the wire; recover the real type here
        // for the rest of the stack (got_data()'s dispatch) to see.
        int i = sizeof(msg);
        while (i > 0 && !msg[i-1]) i--;
        if (!i) {
          fail = alert(ALERT_fatal, ALERT_unexpected_message,
                       "SSL.State: Empty TLSInnerPlaintext.\n");
        } else {
          packet->content_type = msg[i-1];
          msg = msg[..i-2];
        }
      }
      break;
    }

    if (!msg) msg = data;
    padding = sizeof(data) - sizeof(msg);
    data = msg;
  }

  SSL3_DEBUG_CRYPT_MSG("SSL.State: Decrypted_packet %O\n", data);

  if (tls_iv) {
    // TLS 1.1 IV. RFC 4346 6.2.3.2:
    // The decryption operation for all three alternatives is the same.
    // The receiver decrypts the entire GenericBlockCipher structure and
    // then discards the first cipher block, corresponding to the IV
    // component.
    data = data[tls_iv..];
  }

  if (hmac_size)
  {
    SSL3_DEBUG_CRYPT_MSG("SSL.State: Trying mac verification...\n");
    int length = sizeof(data) - hmac_size;
    string digest = data[length ..];
    data = data[.. length - 1];

   /* NOTE: Perform some extra MAC operations, to avoid timing
    *       attacks on the length of the padding.
    *
    *       This is to alleviate the "Lucky Thirteen" attack:
    *       http://www.isg.rhul.ac.uk/tls/TLStiming.pdf
    *
    * NB: This is not needed in encrypt-then-mac mode, since we
    *     always MAC the entire block.
    */
    int block_size = mac->block_size();
    string pad_string = "\0"*block_size;
    string junk = pad_string[<padding-1..];
    if (!((sizeof(data) +
           mac->hash_header_size - session->cipher_spec->hash_size) %
          block_size ) ||
        !(padding % block_size)) {
      // We're at the edge of a MAC block, so we need to
      // pad junk with an extra MAC block of data.
      junk += pad_string;
    }
    junk = mac->hash_raw(junk);

    // Set decrypted data without HMAC.
    fail = fail || [object(Alert)]packet->set_compressed(data);

    if (digest != mac->hash_packet(packet)[..hmac_size-1])
      {
        SSL3_DEBUG_MSG("Failed MAC-verification!!\n");
        SSL3_DEBUG_CRYPT_MSG("Expected digest: %O\n"
                             "Calculated digest: %O\n"
                             "Seqence number: %O\n",
                             digest, mac->hash_packet(packet),
                             packet->seq_num);
	fail = fail || alert(ALERT_fatal, ALERT_bad_record_mac,
			     "Bad MAC.\n");
      }
  }
  else
  {
    // Set decrypted data.
    fail = fail || [object(Alert)]packet->set_compressed(data);
  }

  if (compress)
  {
    SSL3_DEBUG_CRYPT_MSG("SSL.State: Trying decompression...\n");
    data = compress(data);
    if (!data)
      fail = fail || alert(ALERT_fatal, ALERT_unexpected_message,
			   "Invalid compression.\n");

    // Set uncompressed data
    fail = fail || [object(Alert)]packet->set_plaintext(data);
  }

  return fail || packet;
}

//! Encrypts a packet (including deflating and MAC-generation).
Alert|Packet encrypt_packet(Packet packet, Context ctx)
{
  string data = packet->fragment;
  string digest;
  Alert res;

  if (undefinedp(packet->seq_num)) {
    packet->seq_num = next_seq_num++;
  }
  SSL3_DEBUG_MSG("ENCRYPT: Packet #%d\n", packet->seq_num);

  if (compress)
  {
    // RFC 5246 6.2.2. states that data growth must be at most 1024
    // bytes. Since zlib doesn't do that, and no other implementation
    // is defined, don't bother checking.
    data = compress(data);

    // Set compressed data.
    res = [object(Alert)]packet->set_compressed(data);
    if(res) return res;
  }

  int hmac_size = mac && (session->truncated_hmac ? 10 :
			  session->cipher_spec->?hash_size);

  if (mac && !session->encrypt_then_mac) {
    digest = mac->hash_packet(packet)[..hmac_size-1];
    hmac_size = 0;
  } else
    digest = "";

  if (crypt)
  {
    SSL3_DEBUG_CRYPT_MSG("SSL.State: Encrypting message #%d: %O.\n",
                         packet->seq_num, data);

    switch(session->cipher_spec->cipher_type) {
    case CIPHER_stream:
      data = crypt->crypt(data + digest);
      break;
    case CIPHER_block:
      if(version == PROTOCOL_SSL_3_0) {
	data = crypt->crypt(data + digest);
	data += crypt->pad(Crypto.PAD_SSL);
      } else if (version >= PROTOCOL_TLS_1_0) {
	if (tls_iv) {
	  // RFC 4346 6.2.3.2.2:
	  // Generate a cryptographically strong random number R of length
	  // CipherSpec.block_length and prepend it to the plaintext prior
	  // to encryption.
	  string iv = ctx->random(tls_iv);
	  crypt->set_iv(iv);
	  data = iv + crypt->crypt(data) + crypt->crypt(digest) + crypt->pad(Crypto.PAD_TLS);
	} else {
          data = crypt->crypt(data) + crypt->crypt(digest) + crypt->pad(Crypto.PAD_TLS);
        }
      }
      break;
    case CIPHER_aead:
      // FIXME: Do we need to pay attention to threads here?
      if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.2: TLSInnerPlaintext -- append the true content
        // type (no padding by default); the outer record is always
        // presented as application_data on the wire.
        data += sprintf("%c", packet->content_type);
        packet->content_type = PACKET_application_data;
      }
      string explicit_iv = "";
      string iv;
      if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.3: the nonce is the static per-direction write IV
        // (the whole of it -- see Cipher.pmod's TLS 1.3 iv_size
        // override) XORed with the sequence number, left-padded with
        // zeros to the IV's length. No nonce octets are sent.
        iv = salt ^ sprintf("%*c", sizeof(salt), packet->seq_num);
      } else if (session->cipher_spec->explicit_iv_size) {
        // RFC 5288 3:
        // The nonce_explicit MAY be the 64-bit sequence number.
        //
        explicit_iv = sprintf("%*c", session->cipher_spec->explicit_iv_size,
                              packet->seq_num);
        iv = salt + explicit_iv;
      } else {
        // Draft ChaCha20-Poly1305 5:
        //   When used in TLS, the "record_iv_length" is zero and the nonce is
        //   the sequence number for the record, as an 8-byte, big-endian
        //   number.
        iv = sprintf("%s%*c", salt, crypt->iv_size() - sizeof(salt),
                     packet->seq_num);
      }
      SSL3_DEBUG_CRYPT_MSG("SSL.State: AEAD IV: %O.\n", iv);
      crypt->set_iv(iv);
      string auth_data;
      if (version >= PROTOCOL_TLS_1_3) {
        // RFC 8446 5.2: the additional data is exactly the record
        // header as sent on the wire (opaque_type, legacy_record_
        // version, length) -- no sequence number (that only enters
        // via the nonce above).
        auth_data = sprintf("%c%2c%2c", packet->content_type,
                            packet->protocol_version,
                            sizeof(data) + crypt->digest_size());
      } else {
        auth_data = sprintf("%8c%c%2c%2c",
                            packet->seq_num, packet->content_type,
                            version, sizeof(data));
      }

      SSL3_DEBUG_CRYPT_MSG("SSL.State: AEAD auth data: %O.\n", auth_data);
      crypt->update(auth_data);
      data = explicit_iv + crypt->crypt(data) + crypt->digest();
      break;
    }
  }
  else
    data += digest;

  // Set encrypted data.
  res = [object(Alert)]packet->set_encrypted(data);
  if(res) return res;

  if (hmac_size) {
    // Encrypt-then-MAC mode.
    data += mac->hash_packet(packet)[..hmac_size-1];

    // Set HMAC protected data.
    res = [object(Alert)]packet->set_encrypted(data);
    if(res) return res;
  }

  return packet;
}

//! TLS 1.3 and later (@rfc{9846:5.5@}): check whether this State's
//! AEAD key is approaching, or has reached, its safe usage limit (a
//! MUST since RFC 9846, previously only a SHOULD in @rfc{8446@}) --
//! call periodically from both @[Connection.to_write()] (this
//! State's own usage) and @[Connection.recv_packet()] (the peer's).
//!
//! @returns
//!   @[AEAD_LIMIT_rekey] the first time @[next_seq_num] crosses a
//!   conservative threshold below the real limit (giving a key
//!   update time to complete before the hard limit is ever reached);
//!   @[AEAD_LIMIT_close] the first time it reaches the hard limit
//!   itself (meaning no rekey happened in time); @[AEAD_LIMIT_none]
//!   otherwise. Each of the first two fires at most once per object.
//!
//! @note
//!   RFC 9846 5.5 gives an explicit limit only for AES-GCM
//!   (~2^24.5 records); ChaCha20-Poly1305 has no limit of this kind
//!   short of the 64-bit sequence number itself (unreachable in
//!   practice). AES-CCM is given no limit of its own either -- this
//!   conservatively applies the same AES-GCM figure to it too,
//!   rather than assume an unbounded one.
int(0..2) check_aead_usage_limit()
{
  if (!crypt || (session->cipher_spec->cipher_type != CIPHER_aead)) {
    return AEAD_LIMIT_none;
  }
#if constant(Crypto.ChaCha20.POLY1305)
  if (session->cipher_spec->bulk_cipher_algorithm ==
      Crypto.ChaCha20.POLY1305.State) {
    return AEAD_LIMIT_none;
  }
#endif

  // ~2^24.5 full-size records, per RFC 9846 5.5's AES-GCM figure.
  constant hard_limit = 23726566;
  constant soft_limit = 16000000;

  if (!aead_limit_close_signaled && (next_seq_num >= hard_limit)) {
    aead_limit_close_signaled = 1;
    return AEAD_LIMIT_close;
  }
  if (!aead_limit_rekey_signaled && (next_seq_num >= soft_limit)) {
    aead_limit_rekey_signaled = 1;
    return AEAD_LIMIT_rekey;
  }
  return AEAD_LIMIT_none;
}
