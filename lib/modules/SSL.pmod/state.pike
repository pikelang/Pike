/* state.pike
 *
 */

inherit "constants";

object session;

// string my_random;
// string other_random;  
object mac;
object crypt;
object compress;

object(Gmp.mpz) seq_num;    /* Bignum, values 0, .. 2^64-1 are valid */

constant Alert = (program) "alert";

void create(object s)
{
  session = s;
  seq_num = Gmp.mpz(0);
}


/* Destructively decrypt a packet. Returns an Alert object if
 * there was an error, otherwise 0. */
object decrypt_packet(object packet)
{
  if (crypt)
  {
    string msg;
//    werror("Trying decrypt..\n");
    msg = crypt->crypt(packet->fragment); 
    if (! msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message);
    if (session->cipher_spec->cipher_type == CIPHER_block)
      if (catch { msg = crypt->unpad(msg); })
	return Alert(ALERT_fatal, ALERT_unexpected_message);
    packet->fragment = msg;
  }

//  werror(sprintf("Decrypted_packet '%s'\n", packet->fragment));

  if (mac)
  {
//    werror("Trying mac verification...\n");
    int length = strlen(packet->fragment) - session->cipher_spec->hash_size;
    string digest = packet->fragment[length ..];
    packet->fragment = packet->fragment[.. length - 1];
    
    if (digest != mac->hash(packet, seq_num))
      return Alert(ALERT_fatal, ALERT_bad_record_mac);
    seq_num += 1;
  }

  if (compress)
  {
//    werror("Trying decompression...\n");
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
