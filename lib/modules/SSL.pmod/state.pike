/* $Id: state.pike,v 1.9 2001/06/25 12:21:31 noy Exp $
 *
 */


#define SSL3_DEBUG
#define SSL3_DEBUG_CRYPT
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



string tls_pad(string data,int blocksize  ) {

  werror("Blocksize:"+blocksize+"\n");
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

/* Destructively decrypt a packet. Returns an Alert object if
 * there was an error, otherwise 0. */
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
      return Alert(ALERT_fatal, ALERT_unexpected_message);
    if (session->cipher_spec->cipher_type == CIPHER_block)
      if(version==0) {
	if (catch { msg = crypt->unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message);
      } else {
	if (catch { msg = tls_unpad(msg); })
	  return Alert(ALERT_fatal, ALERT_unexpected_message);
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
    
    werror("strlen(digest)="+strlen(digest)+"     hash_size="+session->cipher_spec->hash_size+"\n");
    if(strlen(digest) != session->cipher_spec->hash_size) {
      
      
      werror("FETT MED FEL!!!!!! state.pike:100\n");
    }
    
    werror("When decrypting packet, digest in packet was:\""+digest+"\" and\n");
    if(digest != "") {
      werror(sprintf("Beg of digest:%x%x%x%x%x%x%x%x",digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7]));
    }

    werror("strlen of the decrypted packet is:"+strlen(packet->fragment)+"\n");
    werror("mac of the packet was:\""+mac->hash(packet, seq_num)+"\".\n");
    if (digest != mac->hash(packet, seq_num))
      {
#ifdef SSL3_DEBUG
	werror("Failed MAC-verification!!\n");
#endif
	return Alert(ALERT_fatal, ALERT_bad_record_mac);
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
      return Alert(ALERT_fatal, ALERT_unexpected_message);
    packet->fragment = msg;
  }
  return packet->check_size() || packet;
}

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
  werror("When encrypting packet, digest was was:\""+digest+"\"\n");
  if(digest!="") {
    werror(sprintf("Beg of digest:%x%x%x%x%x%x%x%x",digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7]));
  }

  werror("strlen of the unencrypted packet is:"+strlen(packet->fragment)+"\n");
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
  
  //  werror("SSL.state: The encrypted packet is:"+sprintf("%O\n",packet->fragment));
  werror("strlen of the encrypted packet is:"+strlen(packet->fragment)+"\n");
  return packet->check_size(2048) || packet;
}


