//
// $Id: PGP.pmod,v 1.1 2003/12/01 18:51:42 nilsson Exp $

#pike __REAL_VERSION__

constant pubkey_data = MIME.decode_base64(#"
mQGiBDxoV3IRBAChUosVpfNWIqhh7g/VNZPv16x3+0WOIyBpLiDSNt1NnEyIP5MDveTwLO0a2SIL
5ScC97gXJENNWSwVH9QpL6xepT7w6tmkgZZLP6s6NeoyC2vFk5fGDaGnvAb4XoPCO+XUEItNTAVy
wqH7hZIXF4zTjZmUFz2CZYeTU/Ek/pkTVwCg8qYqy2hFVRMBOJFtqCpykiCYXR0D/iYxnMzSAotD
yFRVpg0sbHZDTgWd+mojy/5Uslxp32KMX+RMrFhAhoOQsjdmbewVgLsqKlJR1wE8vIukZQ0Poug1
AOn9Y52MLC4S0AvI/gZ67i95ulmxqzpHgUygtaDTOegGm2PzVcks+UIytskyboQs4bH79TYIxNGf
ubl3QseaA/9zk6TJR84cbdeZDeAx6jtakyIiQ2ASfyV08Vizxi7lozrbNI9JMzQ0gnuSoU+20aBw
r3dY4H/5XxNm7CtNC1f0YGOpZev1tplEg3aTPxPPTAyDFXz6zAV1NPyFjG1lQuT2r3FTIdyPpKWF
vuDgXDaQqLvd85EnAKh1R3k5TBW26LQORHJlYW1TTkVTIFRlYW2IVwQTEQIAFwUCPGhXcgULBwoD
BAMVAwIDFgIBAheAAAoJEHVpXtHTanZIQBsAn1FZlukRp6sCA21t0He3YpvakVs4AJ97YVTKFO4+
0u6vsgawb8ANXC47iA==
");

//! Decodes a PGP public key.
//! @returns
//!   @mapping
//!     @member int version
//!     @member int tstamp
//!     @member int validity
//!     @member object key
//!   @endmapping
mapping decode_public_key(string s) {

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
    r->key = Crypto.rsa()->set_public_key(n, e);
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

//! Decodes a PGP signature
//! @returns
//!   @mapping
//!     @member int version
//!     @member int classification
//!     @member int tstamp
//!     @member string key_id
//!     @member int type
//!     @member int digest_algorithm
//!     @member int md_csum
//!
//!     @member Gmp.mpz digest
//!
//!     @member Gmp.mpz digest_r
//!     @member Gmp.mpz digest_s
//!   @endmapping
mapping decode_signature(string s) {

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

static constant pgp_id = ([
  0b100001:"public_key_encrypted",
  0b100010:"signature",
  0b100101:"secret_key",
  0b100110:"public_key",
  0b101000:"compressed_data",
  0b101001:"conventional_key_encrypted",
  0b101011:"literal_data",
  0b101100:"keyring_trust",
  0b101101:"user_id",
]);

static mapping(string:function) pgp_decoder = ([
  "public_key":decode_public_key,
  "signature":decode_signature,
]);

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
    i += data_l;
  }
  return r;
}

int(0..1) verify(Crypto.HashState hash, mapping sig, mapping key) {

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


static int verify_signature(string text, string sig)
{
  catch {
    mapping signt = decode(sig)->signature;
    object hash;
    if(signt->digest_algorithm == 2)
      hash = Crypto.SHA();
    else
      hash = Crypto.MD5();
    hash->update(text);
    return verify(hash, signt,
		  decode(pubkey_data)->public_key);
  };
  return 0;
}
