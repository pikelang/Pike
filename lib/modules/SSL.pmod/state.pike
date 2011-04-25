#pike __REAL_VERSION__

/* $Id$
 *
 */

//! A connection switches from one set of state objects to another, one or
//! more times during its lifetime. Each state object handles a one-way
//! stream of packets, and operates in either decryption or encryption
//! mode.

inherit "constants";

//! Information about the used algorithms.
object session;

// string my_random;
// string other_random;  

//! Message Authentication Code
object mac;

//! Encryption or decryption object.
object crypt;

object compress;

//! 64-bit sequence number.
object(Gmp.mpz)|int seq_num;    /* Bignum, values 0, .. 2^64-1 are valid */

constant Alert = SSL.alert;

void create(object s)
{
  session = s;
  seq_num = Gmp.mpz(0);
}



string tls_pad(string data,int blocksize  ) {
//    werror("Blocksize:"+blocksize+"\n");
  int plen=(blocksize-(strlen(data)+1)%blocksize)%blocksize;
  string res=data + sprintf("%c",plen)*plen+sprintf("%c",plen);
  return res;
}

string tls_unpad(string data ) {

  int plen=data[-1];
  string res=reverse(reverse(data)[plen+1..]);
  string padding=reverse(data)[..plen];
  int tmp;

  /* Checks that the padding is correctly done */
  foreach(values(padding),tmp)
    {
      if(tmp!=plen) {
	werror("Incorrect padding detected!!!\n");
	throw(0);
      }
    }
  return res;
}

//! Destructively decrypts a packet (including inflating and MAC-verification, if
//! needed). On success, returns the decrypted packet. On failure,
//! returns an alert packet. These cases are distinguished by looking
//! at the is_alert attribute of the returned packet.
object decrypt_packet(object packet,int version)
{
#ifdef SSL3_DEBUG_CRYPT
  werror(sprintf("SSL.state->decrypt_packet: data = %O\n", packet->fragment));
#endif
  
  if (crypt)
  {
    string msg;
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying decrypt..\n");
    //    werror("SSL.state: The encrypted packet is:"+sprintf("%O\n",packet->fragment));
    werror("strlen of the encrypted packet is:"+strlen(packet->fragment)+"\n");
#endif
    msg=packet->fragment;
        
    msg = crypt->crypt(msg); 
    if (! msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message, version);
    if (session->cipher_spec->cipher_type == CIPHER_block)
      if(version==0) {
	if (catch { msg = crypt->unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      } else {
	if (catch { msg = tls_unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message, version);
      }
    packet->fragment = msg;
  }
  
#ifdef SSL3_DEBUG_CRYPT
  werror(sprintf("SSL.state: Decrypted_packet %O\n", packet->fragment));
#endif

  if (mac)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror("SSL.state: Trying mac verification...\n");
#endif
    int length = strlen(packet->fragment) - session->cipher_spec->hash_size;
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
    string msg;
    msg = compress(packet->fragment);
    if (!msg)
      return Alert(ALERT_fatal, ALERT_unexpected_message, version);
    packet->fragment = msg;
  }
  return packet->check_size(version) || packet;
}

//! Encrypts a packet (including deflating and MAC-generation).
object encrypt_packet(object packet,int version)
{
  string digest;
  packet->protocol_version=({3,version});
  
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
    if (session->cipher_spec->cipher_type == CIPHER_block)
      {
	if(version==0) {
	  packet->fragment = crypt->crypt(packet->fragment + digest);
	  packet->fragment += crypt->pad();
	} else {
	  packet->fragment = tls_pad(packet->fragment+digest,crypt->query_block_size());
	  packet->fragment = crypt->crypt(packet->fragment);
	}
      } else {
	packet->fragment=crypt->crypt(packet->fragment + digest);
      }
  }
  else
    packet->fragment += digest;
  
  return packet->check_size(version, 2048) || packet;
}


