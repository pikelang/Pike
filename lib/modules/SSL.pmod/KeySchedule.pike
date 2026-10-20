#pike __REAL_VERSION__
#pragma strict_types

//! TLS 1.3 key schedule (@rfc{8446:7.1@}).
//!
//! Unlike the TLS 1.0-1.2 PRF-based @[Session] key block
//! (@[Session.generate_keys()]), TLS 1.3 derives all traffic keys
//! through a chain of HKDF-Extract and HKDF-Expand-Label operations
//! tied to the negotiated cipher suite's hash (@[Cipher.CipherSpec.hash]).
//! This class holds that running state for a single connection.
//!
//! @note
//!   This only implements the key-schedule arithmetic itself --
//!   deciding *when* to call each step, and with which running
//!   transcript hash, is the handshake state machine's job.

import ".";
import Constants;

//! The key-schedule/transcript hash, tied to the negotiated cipher
//! suite (e.g. @expr{Crypto.SHA256@} or @expr{Crypto.SHA384@}).
Crypto.Hash|zero hash;

//! @expr{hash->digest_size()@}, cached.
int(0..) hash_size;

//! @rfc{8446:7.1@} Early Secret.
string(8bit)|zero early_secret;

//! @rfc{8446:7.1@} Handshake Secret.
string(8bit)|zero handshake_secret;

//! @rfc{8446:7.1@} Master Secret.
//!
//! @note
//!   This is the TLS 1.3 key-schedule secret, unrelated to the TLS
//!   1.0-1.2 @expr{Session()->master_secret@}.
string(8bit)|zero master_secret;

//! @rfc{8446:7.1@} PSK binder key.
//!
//! Derived with label @expr{"ext binder"@} for an externally
//! provisioned PSK, or @expr{"res binder"@} for a resumption PSK.
string(8bit)|zero binder_key;

//! @rfc{8446:7.1@} 0-RTT client traffic secret.
string(8bit)|zero client_early_traffic_secret;

//! @rfc{8446:7.1@} 0-RTT exporter secret.
string(8bit)|zero early_exporter_master_secret;

//! @rfc{8446:7.1@} Handshake traffic secrets.
string(8bit)|zero client_handshake_traffic_secret;
string(8bit)|zero server_handshake_traffic_secret;

//! @rfc{8446:7.1@} Application traffic secrets (generation 0; see
//! @[update_traffic_secret()] for later generations after a
//! @[HANDSHAKE_key_update]).
string(8bit)|zero client_application_traffic_secret;
string(8bit)|zero server_application_traffic_secret;

//! @rfc{8446:7.1@} Exporter and resumption secrets.
string(8bit)|zero exporter_master_secret;
string(8bit)|zero resumption_master_secret;

protected void create(Crypto.Hash hash_algorithm)
{
  hash = hash_algorithm;
  hash_size = hash->digest_size();
}

//! @rfc{5869:2.2@} HKDF-Extract(salt, ikm).
string(8bit) hkdf_extract(string(8bit) salt, string(8bit) ikm)
{
  return hash->HMAC(salt)(ikm);
}

//! @rfc{5869:2.3@} HKDF-Expand(prk, info, length).
string(8bit) hkdf_expand(string(8bit) prk, string(8bit) info, int length)
{
  if (length > 255 * hash_size)
    error("Requested output length %d exceeds RFC 5869 limit of %d.\n",
          length, 255 * hash_size);
  function(string(8bit):string(8bit)) hmac = hash->HMAC(prk);
  string(8bit) t = "";
  string(8bit) res = "";
  int(8bit) i;
  while (sizeof(res) < length) {
    i++;
    t = hmac(t + info + sprintf("%c", i));
    res += t;
  }
  return res[..length-1];
}

//! @rfc{8446:7.1@} HKDF-Expand-Label(secret, label, context, length).
string(8bit) expand_label(string(8bit) secret, string(8bit) label,
                           string(8bit) context, int length)
{
  Stdio.Buffer hkdf_label = Stdio.Buffer();
  hkdf_label->add_int(length, 2);
  hkdf_label->add_hstring("tls13 " + label, 1);
  hkdf_label->add_hstring(context, 1);
  return hkdf_expand(secret, (string(8bit))hkdf_label, length);
}

//! @rfc{8446:7.1@} Derive-Secret(secret, label, messages).
//!
//! @param messages_hash
//!   @expr{Transcript-Hash(messages)@}, i.e. @expr{hash->hash(messages)@}
//!   -- @b{not@} the raw handshake messages themselves.
string(8bit) derive_secret(string(8bit) secret, string(8bit) label,
                            string(8bit) messages_hash)
{
  return expand_label(secret, label, messages_hash, hash_size);
}

//! @rfc{8446:7.1@} Begin the key schedule: derive the Early Secret
//! from the PSK, or from an all-zero IKM if not resuming/no external
//! PSK is in use.
void generate_early_secret(string(8bit)|void psk)
{
  early_secret = hkdf_extract("\0"*hash_size, psk || "\0"*hash_size);
}

//! @rfc{8446:7.1@} Advance the key schedule: derive the Handshake
//! Secret from the (EC)DHE shared secret.
//!
//! @[generate_early_secret()] must have been called first.
void generate_handshake_secret(string(8bit) shared_secret)
{
  string(8bit) salt = derive_secret([string(8bit)]early_secret, "derived",
                                     hash->hash(""));
  handshake_secret = hkdf_extract(salt, shared_secret);
}

//! @rfc{8446:7.1@} Advance the key schedule: derive the Master
//! Secret.
//!
//! @[generate_handshake_secret()] must have been called first.
void generate_master_secret()
{
  string(8bit) salt = derive_secret(handshake_secret, "derived",
                                     hash->hash(""));
  master_secret = hkdf_extract(salt, "\0"*hash_size);
}

//! @rfc{8446:7.3@} Derive the write key and IV for a traffic secret.
//!
//! @returns
//!   @array
//!     @elem string(8bit) 0
//!       The write key, @expr{key_length@} bytes.
//!     @elem string(8bit) 1
//!       The write IV, @expr{iv_length@} bytes.
//!   @endarray
array(string(8bit)) traffic_key_iv(string(8bit) secret,
                                    int key_length, int iv_length)
{
  return ({ expand_label(secret, "key", "", key_length),
            expand_label(secret, "iv", "", iv_length) });
}

//! @rfc{8446:4.4.4@} Derive the Finished MAC key for a base key
//! (a traffic secret).
string(8bit) finished_key(string(8bit) base_key)
{
  return expand_label(base_key, "finished", "", hash_size);
}

//! @rfc{8446:4.4.4@} Compute the Finished message's verify_data.
string(8bit) verify_data(string(8bit) base_key, string(8bit) transcript_hash)
{
  return hash->HMAC(finished_key(base_key))(transcript_hash);
}

//! @rfc{8446:7.2@} Derive the next generation of an application
//! traffic secret after a @[HANDSHAKE_key_update].
string(8bit) update_traffic_secret(string(8bit) secret)
{
  return expand_label(secret, "traffic upd", "", hash_size);
}
