
//! PGP stuff. See @rfc{4880@}.

#pike __REAL_VERSION__
#require constant(Crypto.HashState)

protected Gmp.mpz read_number(Stdio.Buffer b)
{
  int bits = b->read_int(2);
  return Gmp.mpz(b->read_int( (bits+7)>>3 ));
}

// Decodes a PGP public key.
// @returns
//   @mapping
//     @member int version
//     @member int tstamp
//     @member int validity
//     @member object key
//   @endmapping
protected mapping decode_public_key(Stdio.Buffer b)
{
  mapping r = ([]);

  r->version = b->read_int8();
  r->tstamp  = b->read_int32();
  if( r->version < 4 )
    r->validity = b->read16();
  r->type    = b->read_int8();

  switch(r->type) {
  case 1: {
    // RSA, Encrypt or Sign
    r->_type = "RSA (encrypt or sign)";

    // n and e
    r->key = Crypto.RSA()->set_public_key(read_number(b), read_number(b));
  }
    break;
  case 2:
    // RSA, Encrypt only
    r->_type = "RSA (encrypt only)";
    break;
  case 3:
    // RSA, Sign only
    r->_type = "RSA (sign only)";
    break;
  case 16:
    // Elgamal, Encrypt only
    r->_type = "Elgamal (encrypt only)";
    break;
  case 17: {
    // DSA
    r->_type = "DSA";

    // p, q, g and y
    r->key = Crypto.DSA()->set_public_key(read_number(b), read_number(b),
                                          read_number(b), read_number(b));
  }
    break;
  case 18:
    // Elliptic Curve
    r->_rtype = "ECC";
    break;
  case 19:
    // ECDSA
    r->_type = "ECDSA";
    break;
  case 20:
    // Elgamal, Encrypt or Sign
    r->_type = "Elgamal (encrypt or sign)";
    break;
  case 21:
    // Diffie-Hellman, X9.42
    r->_type = "Diffie-Hellmanm X9.42";
    break;
  case 100..110:
    // 100-110 is for private/experimental algorithms.
    r->type = "Private/experimental";
    break;
  default:
    r->_type = "Unknown";
    break;
  }

  return r;
}

protected mapping decode_secret_key(Stdio.Buffer b)
{
  mapping r = decode_public_key(b);

  int usage = b->read_int8();
  if( usage != 0 )
  {
    r->_error = "Only unencrypted string-to-key usage supported.";
    return r;
  }

  switch(r->type)
  {
  case 1:
    // d, p and q.
    r->key->set_private_key(read_number(b), ({ read_number(b), read_number(b) }));
    // u. Not used
    read_number(b);
    break;

  case 17:
    // x
    r->key->set_private_key(read_number(b));
    break;
  }

  return r;
}

// Decodes a PGP signature
// @returns
//   @mapping
//     @member int version
//     @member int classification
//     @member int tstamp
//     @member string key_id
//     @member int type
//     @member int digest_algorithm
//     @member int md_csum
//
//     @member Gmp.mpz digest
//
//     @member Gmp.mpz digest_r
//     @member Gmp.mpz digest_s
//   @endmapping
protected mapping decode_signature(Stdio.Buffer b)
{
  mapping r = ([]);
  r->version = b->read_int8();
  int l5 = b->read_int8();
  r->classification = b->read_int8();
  r->tstamp = b->read_int32();
  r->key_id = b->read(8);
  r->type = b->read_int8();
  r->digest_algorithm = b->read_int8();
  r->md_csum = b->read_int16();

  if(r->type == 1) {
    r->digest = read_number(b);
  } else if(r->type == 17) {
    r->digest_r = read_number(b);
    r->digest_s = read_number(b);
  }
  return r;
}

protected mapping decode_compressed(string s) {
  int type = s[0];
  s = s[1..];
  switch(type) {
  case 1:
    // ZIP
    error("Don't know how to decompress ZIP.\n");
  case 2:
    // ZLIB
    error("Don't know how to decompress ZLIB.\n");
  case 3:
    // BZip2
    error("Don't know how to decompress BZip2.\n");
  case 100..110:
    error("Private/experimental compression.\n");
  default:
    error("Unknown compression.\n");
#if 0
    // Perhaps like this?
    return decode( Gz.inflate()->inflate(s) );
#endif
  }
}

protected constant pgp_id = ([
  0b100001:"public_key_encrypted",
  0b100010:"signature",
  0b100011:"symmetric_key",
  0b100100:"one_pass_signature",
  0b100101:"secret_key",
  0b100110:"public_key",
  0b100111:"secret_subkey",
  0b101000:"compressed_data",
  0b101001:"symmetric_key_encrypted",
  0b101010:"marker",
  0b101011:"literal_data",
  0b101100:"keyring_trust",
  0b101101:"user_id",
  0b101110:"public_subkey",
  0b101111:"user_attribute",
  0b110000:"sym_encrypt_and_integrity",
  0b110001:"modification_detection",
]);

protected mapping(string:function) pgp_decoder = ([
  "public_key":decode_public_key,
  "public_subkey":decode_public_key,
  "secret_key":decode_secret_key,
  "secret_subkey":decode_secret_key,
  "signature":decode_signature,
  "compressed_data":decode_compressed,
]);

//! Decodes PGP data.
mapping(string:string|mapping|array) decode(string s)
{
  Stdio.Buffer buf = Stdio.Buffer(s);
  mapping(string:string|mapping|array) r = ([]);
  while( sizeof(buf) )
  {
    int h = buf->read_int8();
    Stdio.Buffer data;
    if( (h&3) == 3 )
    {
      data = buf;
    }
    else
    {
      int len = 1<<(h&3);
      data = buf->read_hbuffer( len );
      if( !data )
      {
        if( len<=sizeof(buf) )
          len = buf->read_int(len);
        error("Attempt to read %d from %d bytes of data.\n", len, sizeof(buf));
      }
    }

    h >>= 2;
    string id = pgp_id[h] || sprintf("unknown_%07b",h);
    mapping|string val;
    if(pgp_decoder[id])
      val = pgp_decoder[id](data);
    else
      val = data->read();

    if( r[id] )
    {
      if(!arrayp(r[id]))
        r[id] = ({ r[id], val });
      else
        r[id] += ({ r[id] });
    }
    else
      r[id] = val;
  }
  return r;
}

string encode(int type, string data) {
  type <<= 2;
  if(sizeof(data)>65535)
    return sprintf("%c%4H", type+2, data);
  if(sizeof(data)>255)
    return sprintf("%c%2H", type+1, data);
  return sprintf("%c%1H", type, data);
}

protected int(0..1) verify(Crypto.HashState hash, mapping sig, mapping key) {

  if(!objectp(hash) || !mappingp(sig) || !mappingp(key) || !key->key)
    return 0;

  if(sig->type != key->type)
    return 0;

  string digest = hash->update(sprintf("%c%4c", sig->classification,
				       sig->tstamp))->digest();
  int csum;
  if(1 != sscanf(digest, "%2c", csum) || csum != sig->md_csum)
    return 0;

  if(key->type == 1 && sig->digest_algorithm == 1)
    return key->key->raw_verify("0 0\14\6\10*\x86H\x86\xf7\15\2\5\5\0\4\20"+
				digest, sig->digest);

  else if(key->type == 17)
    return key->key->raw_verify(Gmp.mpz(digest,256),
				sig->digest_r, sig->digest_s);
  else
    return 0;
}

//! Verify @[text] against signature @[sig] with the public key
//! @[pubkey].
int verify_signature(string text, string sig, string pubkey)
{
  mapping signt = decode(sig)->signature;
  object hash;

  switch( signt->digest_algorithm )
  {
  case 1:  hash = Crypto.MD5();    break;
  case 2:  hash = Crypto.SHA1();   break;
  case 3: error("RIPE-MD/160 not supported.\n"); break;
  case 8:  hash = Crypto.SHA256(); break;
#if constant(Crypto.SHA384)
  case 9:  hash = Crypto.SHA384(); break;
#endif
#if constant(Crypto.SHA512)
  case 10: hash = Crypto.SHA512(); break;
#endif
  case 11: error("SHA224 not supported.\n"); break;
  case 100..110: error("Private/experimental hash function.\n");
  default: error("Unknown hash function %O\n", signt->digest_algorithm );
  }
  if(signt->literal_data && signt->literal_data!=text) return 0;
  hash->update(text);
  return verify(hash, signt,
		decode(pubkey)->public_key);
}

string sha_sign(string text, mixed key) {
  string hash=Crypto.SHA1.hash(text);
  if(key->name()=="DSA") {
    int r,s;
    [ r, s ] = key->raw_sign( Gmp.mpz(hash,256) );

    // version, l5, classification, tstamp, key_id, type,
    // digest_algorithm, md_csum, l, dig
    hash = sprintf( "%1c%1c%1c%4c%8s%1c%1c%2c", 3, 5, 0,
			  time(), "12345678", 17, 2, 0);
    string t;
    t = r->digits(256);
    hash += sprintf("%2c%s", (sizeof(t)<<3)-7, t);
    t = s->digits(256);
    hash += sprintf("%2c%s", (sizeof(t)<<3)-7, t);
  }
  else
    error("Only DSA keys supported.\n");
  return encode(  0b100010, hash );
}


protected int crc24(string data) {
  int crc = 0xb704ce;
  foreach(data; int pos; int char) {
    crc ^= char<<16;
    for(int i; i<8; i++) {
      crc <<= 1;
      if(crc & 0x1000000)
	crc ^= 0x864cfb;
    }
  }
  return crc & 0xffffff;
}

// PGP MESSAGE
// PGP SIGNED MESSAGE
// PGP PUBLIC KEY BLOCK
// PGP PRIVATE KEY BLOCK
// PGP SIGNATURE

//! Encode PGP data with ASCII armour.
string encode_radix64(string data, string type,
		      mapping(string:string) extra = ([]))
{
  if(!extra->Version)
    extra->Version = "Pike " + __REAL_MAJOR__ + "." +
      __REAL_MINOR__ + "." + __REAL_BUILD__;

  return Standards.PEM.build(type, data, extra, sprintf("%3c",crc24(data)));
}

//! Decode ASCII armour.
mapping(string:mixed) decode_radix64(string data) {

  Standards.PEM.Message m = Standards.PEM.Message(data);
  if( m->pre != m->post )
    error("Illegal format. Begin token %O doesn't match end token %O.\n",
          m->pre, m->post);


  mapping ret = m->headers || ([]);
  if( ret->comment )
    ret->comment = utf8_to_string(ret->comment);
  if( ret->hash )
    ret->hash = ret->hash/",";

  if( m->trailer )
    ret->checksum = (int)Gmp.mpz(m->trailer,256);

  ret->armor_header = m->pre;
  ret->data = m->body;
  ret->actual_checksum = crc24(ret->data);
  return ret;
}
