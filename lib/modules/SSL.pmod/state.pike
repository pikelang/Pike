#pike __REAL_VERSION__
// #pragma strict_types

// $Id: state.pike,v 1.24 2004/02/29 02:56:04 nilsson Exp $

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

constant Alert = .alert;


string tls_pad(string data, int blocksize) {
  int plen=(blocksize-(sizeof(data)+1)%blocksize)%blocksize;
  return data + sprintf("%c",plen)*plen+sprintf("%c",plen);
}

string tls_unpad(string data) {
  int(0..255) plen=[int(0..255)]data[-1];

#ifdef DEBUG
  string padding=reverse(data)[..plen];

  /* Checks that the padding is correctly done */
  foreach(values(padding), int tmp)
    {
      if(tmp!=plen) {
	werror("Incorrect padding detected!!!\n");
	throw(0);
      }
    }
#endif

  return data[..sizeof(data)-plen-2];
}

//! Destructively decrypts a packet (including inflating and MAC-verification,
//! if needed). On success, returns the decrypted packet. On failure,
//! returns an alert packet. These cases are distinguished by looking
//! at the is_alert attribute of the returned packet.
Alert|.packet decrypt_packet(.packet packet, int version)
{
#ifdef SSL3_DEBUG_CRYPT
  werror("SSL.state->decrypt_packet: data = %O\n", packet->fragment);
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

    if (session->cipher_spec->cipher_type == CIPHER_block)
      if(version==0) {
	if (catch { msg = crypt->unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      } else {
	msg = crypt->crypt(msg);
	if (catch { msg = tls_unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      }
    packet->fragment = msg;
  }
  
#ifdef SSL3_DEBUG_CRYPT
  werror("SSL.state: Decrypted_packet %O\n", packet->fragment);
#endif

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
	  packet->fragment = crypt->crypt(packet->fragment);
	}
      } else {
	packet->fragment=crypt->crypt(packet->fragment + digest);
      }
  }
  else
    packet->fragment += digest;
  
  return [object(Alert)]packet->check_size(version, 2048) || packet;
}

#endif // constant(SSL.Cipher.MACAlgorithm)
