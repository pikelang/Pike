#pike __REAL_VERSION__

//! Elliptic Curve Cipher Constants.
//!
//! This module contains constants used with elliptic curve algorithms.

#if constant(Nettle.ECC_Curve)

//! The definition of an elliptic curve.
//!
//! Objects of this class are typically not created by the user.
//!
//! @seealso
//!   @[SECP_192R1], @[SECP_224R1], @[SECP_256R1], @[SECP_384R1], @[SECP_521R1]
class Curve {
  inherit Nettle.ECC_Curve;

#define BitString Standards.ASN1.Types.BitString
#define Integer Standards.ASN1.Types.Integer
#define Object Standards.ASN1.Types.Object
#define Sequence Standards.ASN1.Types.Sequence

  //! Returns the PKCS-1 elliptic curve parameters for the curve.
  //! cf RFC 5480 2.1.1.
  Sequence pkcs_ec_parameters()
  {
    switch(name()) {
    case "SECP_192R1":
      return Sequence( ({ Standards.PKCS.Identifiers.ecc_secp192r1_id }) );
    case "SECP_224R1":
      return Sequence( ({ Standards.PKCS.Identifiers.ecc_secp224r1_id }) );
    case "SECP_256R1":
      return Sequence( ({ Standards.PKCS.Identifiers.ecc_secp256r1_id }) );
    case "SECP_384R1":
      return Sequence( ({ Standards.PKCS.Identifiers.ecc_secp384r1_id }) );
    case "SECP_521R1":
      return Sequence( ({ Standards.PKCS.Identifiers.ecc_secp521r1_id }) );
    }
    return 0;
  }

  //! Returns the AlgorithmIdentifier as defined in RFC5480 section 2.
  Sequence pkcs_algorithm_identifier()
  {
    return
      Sequence( ({ Standards.PKCS.Identifiers.ec_id,
		   pkcs_ec_parameters(),
		}) );
  }

  //! Elliptic Curve Digital Signing Algorithm
  //!
  class ECDSA
  {
    //! @ignore
    inherit ::this_program;
    //! @endignore

    //! @decl inherit ECC_Curve::ECDSA;

    //! Return the curve.
    Curve curve()
    {
      return Curve::this;
    }

    //! Return the curve size in bits.
    int size()
    {
      return Curve::size();
    }

    //! Set the private key.
    //!
    //! @note
    //!   Throws errors if the key isn't valid for the curve.
    this_program set_private_key(object(Gmp.mpz)|int k)
    {
      ::set_private_key(k);
      return this;
    }

    //! Change to the selected point on the curve as public key.
    //!
    //! @note
    //!   Throws errors if the point isn't on the curve.
    this_program set_public_key(object(Gmp.mpz)|int x, object(Gmp.mpz)|int y)
    {
      ::set_public_key(x, y);
      return this;
    }

    //! Set the random function, used to generate keys and parameters,
    //! to the function @[r].
    this_program set_random(function(int:string(8bit)) r)
    {
      ::set_random(r);
      return this;
    }

    //! Generate a new set of private and public keys on the current curve.
    this_program generate_key()
    {
      ::generate_key();
      return this;
    }

    //! Get the ANSI x9.62 4.3.6 encoded uncompressed public key.
    string(8bit) get_public_key()
    {
      return sprintf("%c%*c%*c",
		     4,	// Uncompressed.
		     (size() + 7)>>3, get_x(),
		     (size() + 7)>>3, get_y());
    }

    //! Signs the @[message] with a PKCS-1 signature using hash algorithm
    //! @[h].
    string(8bit) pkcs_sign(string(8bit) message, .Hash h)
    {
      array sign = map(raw_sign(h->hash(message)), Integer);
      return Sequence(sign)->get_der();
    }

    //! Verify PKCS-1 signature @[sign] of message @[message] using hash
    //! algorithm @[h].
    int(0..1) pkcs_verify(string(8bit) message, .Hash h, string(8bit) sign)
    {
      Object a = Standards.ASN1.Decode.simple_der_decode(sign);

      // The signature is the DER-encoded ASN.1 sequence Ecdsa-Sig-Value
      // with the two integers r and s. See RFC 4492 section 5.4.
      if (!a
	  || (a->type_name != "SEQUENCE")
	  || (sizeof([array]a->elements) != 2)
	  || (sizeof( ([array(object(Object))]a->elements)->type_name -
		      ({ "INTEGER" }))))
	return 0;

      return raw_verify(h->hash(message),
			[object(Gmp.mpz)]([array(object(Object))]a->elements)[0]->
			value,
			[object(Gmp.mpz)]([array(object(Object))]a->elements)[1]->
			value);
    }

    //! Returns the PKCS-1 algorithm identifier for ECDSA and the provided
    //! hash algorithm. Only SHA-2 based hashes are supported currently.
    Sequence pkcs_signature_algorithm_id(.Hash hash)
    {
      switch(hash->name())
      {
      case "sha224":
	return Sequence( ({ Standards.PKCS.Identifiers.ecdsa_sha224_id }) );
      case "sha256":
	return Sequence( ({ Standards.PKCS.Identifiers.ecdsa_sha256_id }) );
      case "sha384":
	return Sequence( ({ Standards.PKCS.Identifiers.ecdsa_sha384_id }) );
      case "sha512":
	return Sequence( ({ Standards.PKCS.Identifiers.ecdsa_sha512_id }) );
      }
      return 0;
    }

    //! Returns the AlgorithmIdentifier as defined in RFC5480 section
    //! 2.1.1 including the ECDSA parameters.
    Sequence pkcs_algorithm_identifier()
    {
      return Curve::pkcs_algorithm_identifier();
    }

    //! Creates a SubjectPublicKeyInfo ASN.1 sequence for the object.
    //! See RFC 5280 section 4.1.2.7.
    Sequence pkcs_public_key()
    {
      return Sequence(({
			pkcs_algorithm_identifier(),
			BitString(get_public_key()),
		      }));
    }
#undef Sequence
#undef Object
#undef Integer
#undef BitString
  }
}

//! @module SECP_192R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_224R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_256R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_384R1

//! @decl inherit Curve

//! @endmodule

//! @module SECP_521R1

//! @decl inherit Curve

//! @endmodule

//! @ignore
Curve SECP_192R1 = Curve(1, 192, 1);
Curve SECP_224R1 = Curve(1, 224, 1);
Curve SECP_256R1 = Curve(1, 256, 1);
Curve SECP_384R1 = Curve(1, 384, 1);
Curve SECP_521R1 = Curve(1, 521, 1);
//! @endignore

#endif /* Nettle.ECC_Curve */
