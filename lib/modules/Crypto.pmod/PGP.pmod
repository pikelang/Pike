//
// $Id: PGP.pmod,v 1.10 2004/03/01 19:10:58 nilsson Exp $

//! PGP stuff. See RFC 2440.

#pike __REAL_VERSION__

#if constant(Crypto.HashState)

// Decodes a PGP public key.
// @returns
//   @mapping
//     @member int version
//     @member int tstamp
//     @member int validity
//     @member object key
//   @endmapping
static mapping decode_public_key(string s) {

  mapping r = ([]);
  string key;
  int l;
  if(s[0]>=4)
    sscanf(s, "%1c%4c%1c%2c%s", r->version, r->tstamp,
	   r->type, l, key);
  else
    sscanf(s, "%1c%4c%2c%1c%2c%s", r->version, r->tstamp, r->validity,
	   r->type, l, key);

  switch(r->type) {
  case 1: {
    // RSA, Encrypt or Sign
    Gmp.mpz n, e;

    l = (l+7)>>3;
    n = Gmp.mpz(key[..l-1],256);
    sscanf(key[l..], "%2c%s", l, key);
    l = (l+7)>>3;
    e = Gmp.mpz(key[..l-1],256);
    r->key = Crypto.RSA()->set_public_key(n, e);
  }
    break;
  case 2:
    // RSA, Encrypt only
    break;
  case 3:
    // RSA, Sign only
    break;
  case 16:
    // Elgamal, Encrypt only
    break;
  case 17: {
    // DSA
    Gmp.mpz p, q, g, y;

    l = (l+7)>>3;
    p = Gmp.mpz(key[..l-1],256);
    sscanf(key[l..], "%2c%s", l, key);
    l = (l+7)>>3;
    q = Gmp.mpz(key[..l-1],256);
    sscanf(key[l..], "%2c%s", l, key);
    l = (l+7)>>3;
    g = Gmp.mpz(key[..l-1],256);
    sscanf(key[l..], "%2c%s", l, key);
    l = (l+7)>>3;
    y = Gmp.mpz(key[..l-1],256);
    r->key = Crypto.dsa()->set_public_key(p, q, g, y);
    r->key->random = Crypto.Random.random_string;
  }
    break;
  case 18:
    // Elliptic Curve
    break;
  case 19:
    // ECDSA
    break;
  case 20:
    // Elgamal, Encrypt or Sign
    break;
  case 21:
    // Diffie-Hellman, X9.42
    break;
    // 100-110 is for private/experimental algorithms.
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
static mapping decode_signature(string s) {

  mapping r = ([]);
  int l5, l;
  string dig;
  sscanf(s, "%1c%1c%1c%4c%8s%1c%1c%2c%2c%s", r->version, l5, r->classification,
	 r->tstamp, r->key_id, r->type, r->digest_algorithm,
	 r->md_csum, l, dig);
  if(r->type == 1) {
    l = (l+7)>>3;
    r->digest = Gmp.mpz(dig[..l-1],256);
  } else if(r->type == 17) {
    l = (l+7)>>3;
    r->digest_r = Gmp.mpz(dig[..l-1],256);
    sscanf(dig[l..], "%2c%s", l, dig);
    l = (l+7)>>3;
    r->digest_s = Gmp.mpz(dig[..l-1],256);
  }
  return r;
}

static mapping decode_compressed(string s) {
  error("Can't decompress.\n");
  int type = s[0];
  s = s[1..];
  switch(type) {
  case 1:
    // ZIP
    error("Don't know how to decompress ZIP.\n");
  case 2:
    // ZLIB
    error("Don't know how to decompress ZLIB.\n");
#if 0
    // Perhaps like this?
    return decode( Gz.inflate()->inflate(s) );
#endif
  default:
    return ([ "type":type, "data":s ]);
  }
}

static constant pgp_id = ([
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

static mapping(string:function) pgp_decoder = ([
  "public_key":decode_public_key,
  "public_subkey":decode_public_key,
  "signature":decode_signature,
  "compressed_data":decode_compressed,
]);

//! Decodes PGP data.
mapping(string:string|mapping) decode(string s) {

  mapping(string:string|mapping) r = ([]);
  int i = 0;
  while(i<strlen(s)) {
    int h = s[i++];
    int data_l = 0;
    switch(h&3) {
     case 0:
       sscanf(s[i..i], "%1c", data_l);
       i++;
       break;
     case 1:
       sscanf(s[i..i+1], "%2c", data_l);
       i+=2;
       break;
     case 2:
       sscanf(s[i..i+3], "%4c", data_l);
       i+=4;
       break;       
     case 3:
       data_l = strlen(s)-i;
       break;
    }
    if(i+data_l > strlen(s))
      error("Bad PGP data\n");
    h>>=2;
    if(pgp_id[h])
      r[pgp_id[h]] = (pgp_decoder[pgp_id[h]] || `+)(s[i..i+data_l-1]);
    else
      r[sprintf("unknown_%07b",h)] = s[i..i+data_l-1];
    i += data_l;
  }
  return r;
}

string encode(int type, string data) {
  type <<= 2;
  if(sizeof(data)>65535)
    return sprintf("%c%4c%s", type+2, sizeof(data), data);
  if(sizeof(data)>255)
    return sprintf("%c%2c%s", type+1, sizeof(data), data);
  return sprintf("%c%c%s", type, sizeof(data), data);
}

static int(0..1) verify(Crypto.HashState hash, mapping sig, mapping key) {

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
  if(signt->digest_algorithm == 2)
    hash = Crypto.SHA1();
  else
    hash = Crypto.MD5();
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


static int crc24(string data) {
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
		      void|mapping(string:string) extra) {
  string ret = "-----BEGIN " + type + "-----\n";
  if(!extra) extra = ([]);
  if(!extra->Version)
    extra->Version = "Pike " + __REAL_MAJOR__ + "." +
      __REAL_MINOR__ + "." + __REAL_BUILD__;
  foreach(extra; string key; string value)
    ret += key + ": " + value + "\n";
  ret += "\n";
  ret += (MIME.encode_base64(data,1)/64.0)*"\n" + "\n";
  ret += "=" + MIME.encode_base64(sprintf("%3c",crc24(data))) + "\n";
  ret += "-----END " + type + "-----\n";
  return ret;
}

//! Decode ASCII armour.
mapping(string:mixed) decode_radix64(string data) {
  mapping ret = ([]);
  string tmp;
  if(sscanf(data, "%*s-----BEGIN %s-----", tmp)!=2)
    error("String does not contain \"-----BEGIN\".\n");
  ret->armor_header = tmp;
  if(!has_value(data, "-----END "+tmp+"-----"))
    error("String is imcomplete; no armor tail found.\n");
  sscanf(data, "%*s-----BEGIN "+tmp+"-----\n%s-----END "+tmp+"-----", data);

  array lines = String.trim_all_whites(data)/"\n";
  while(String.trim_all_whites(lines[0])!="") {
    string key,value;
    if(sscanf(lines[0], "%s:%s", key, value)!=2)
      error("Error in key-value pairs.\n");
    switch(key) {
    case "Comment":
      ret->Comment = utf8_to_string(value);
      break;
    case "Hash":
      ret->Hash = value/",";
      break;
    default:
      ret[key]=value;
    }
    lines = lines[1..];
  }

  if(lines[-1][0]=='=') {
    ret->checksum = (int)Gmp.mpz(MIME.decode_base64(lines[-1][1..]),256);
    lines = lines[..sizeof(lines)-2];
  }

  ret->data = MIME.decode_base64(lines*"\n");
  ret->actual_checksum = crc24(ret->data);
  return ret;
}

#endif
