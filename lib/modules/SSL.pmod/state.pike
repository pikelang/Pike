/* $Id: state.pike,v 1.5 2000/05/29 12:23:58 grubba Exp $
 *
 */

inherit "constants";

object session;

// string my_random;
// string other_random;  
object mac;
object crypt;
object compress;

object(Gmp.mpz)|int seq_num;    /* Bignum, values 0, .. 2^64-1 are valid */

constant Alert = SSL.alert;

void create(object s)
{
  session = s;
  seq_num = Gmp.mpz(0);
}


/* Destructively decrypt a packet. Returns an Alert object if
 * there was an error, otherwise 0. */
object decrypt_packet(object packet)
{
#ifdef SSL3_DEBUG
  werror(sprintf("SSL.state->decrypt_packet: data = '%s'\n", packet->fragment));
#endif
  
  if (crypt)
  {
    string msg;
#ifdef SSL3_DEBUG
    werror("SSL.state: Trying decrypt..\n");
#endif
    msg = crypt->crypt(packet->fragment); 
    if (! msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message);
    if (session->cipher_spec->cipher_type == CIPHER_block)
      if (catch { msg = crypt->unpad(msg); })
	return Alert(ALERT_fatal, ALERT_unexpected_message);
    packet->fragment = msg;
  }

#ifdef SSL3_DEBUG
  werror(sprintf("SSL.state: Decrypted_packet '%s'\n", packet->fragment));
#endif

  if (mac)
  {
#ifdef SSL3_DEBUG
    werror("SSL.state: Trying mac verification...\n");
#endif
    int length = strlen(packet->fragment) - session->cipher_spec->hash_size;
    string digest = packet->fragment[length ..];
    packet->fragment = packet->fragment[.. length - 1];
    
    if (digest != mac->hash(packet, seq_num))
      return Alert(ALERT_fatal, ALERT_bad_record_mac);
    seq_num += 1;
  }

  if (compress)
  {
#ifdef SSL3_DEBUG
    werror("SSL.state: Trying decompression...\n");
#endif
    string msg;
    msg = compress(packet->fragment);
    if (!msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message);
    packet->fragment = msg;
  }
  return packet->check_size() || packet;
}

object encrypt_packet(object packet)
{
  string digest;
  
  if (compress)
  {
    packet->fragment = compress(packet->fragment);
  }

  if (mac)
    digest = mac->hash(packet, seq_num);
  else
    digest = "";
  seq_num += 1;
  
  if (crypt)
  {
    packet->fragment = crypt->crypt(packet->fragment + digest);
    if (session->cipher_spec->cipher_type == CIPHER_block)
      packet->fragment += crypt->pad();
  }
  else
    packet->fragment += digest;

  return packet->check_size(2048) || packet;
}
