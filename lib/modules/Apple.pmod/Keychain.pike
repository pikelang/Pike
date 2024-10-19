#pike __REAL_VERSION__

//!
//! Support for the Apple Keychain format.
//!
//! This is used for the files in eg @tt{/System/Library/Keychains@}.
//!

#ifdef APPLE_KEYCHAIN_DEBUG
#define WERR(X ...)	sprintf("Keychain: " X)
#else
#define WERR(X ...)
#endif

array(Standards.X509.TBSCertificate) certs = ({});

protected void create(string(8bit)|void keychain)
{
  if (keychain) add_keychain(keychain);
}

protected Stdio.Buffer read_hbuffer(Stdio.Buffer in)
{
  int len = in->read_int32();
  Stdio.Buffer ret = in->read_buffer(len - 4);
  if (len & 0x03) {
    int pad = in->read_int(4 - (len & 3));
    if (pad && (pad != -1)) {
      werror("Unexpected content in padding: 0x%06x\n", pad);
    }
  }
  return ret;
}

protected string(8bit) read_string(Stdio.Buffer in, int len)
{
  string(8bit) res = in->read(len);
  if (len & 3) {
    int pad = in->read_int(4 - (len & 3));
    if (pad && (pad != -1)) {
      werror("Unexpected content in padding: 0x%06x\n", pad);
    }
  }
  return res;
}

protected void parse_cert(Stdio.Buffer buf)
{
  if (sizeof(buf) < 8) return;

  /* Each certificate has a header of 60 bytes:
   *
   * Offset
   *   0:	INT32		0x05f4	Total length
   *   4:	INT32		0x19	Certificate sequence number.
   *   8:	INT32		0x1c	Unknown (probably some other certs #)
   *   c:	INT32		0
   *  10:	INT32BE		0x0436	Certificate DER length.
   *  14:	INT32		0
   *  18:	INT32BE		0x0475	Length + (0x75 - 0x36).
   *  1c:	INT32BE		0x0479
   *  20:	INT32BE		0x047d
   *  24:	INT32BE		0x0499
   *  28:	INT32BE		0x04b5
   *  2c:	INT32BE		0x0539
   *  30:	INT32BE		0x05bd
   *  34:	INT32BE		0x05c5
   *  38:	INT32BE		0x05dd
   *
   * All following items are padded to 32 bits.
   *
   *  3c:	STRING		DER certificate with the length as above.
   *
   *  43c	INT32		Unknown, seems to always be the value 0x01.
   *  440	INT32		Flags? Values seen are 0 and 3.
   *  444	HSTRING		CN?
   *  45c	HSTRING		CN?
   *  478	HSTRING		DER CN?
   *  -		HSTRING		DER CN?
   *  -		HSTRING		More flags?
   *  -		HSTRING		DER?
   *  -		HSTRING		DER?
   */

  buf = read_hbuffer(buf);

  int entry_no = buf->read_int32();

  int unknown = buf->read_int32();	// Other cert no. Often ~3 larger.

  array(int) index = buf->read_ints(12, 4);

  WERR("Certificate index: %O\n", index);

  // This is the DER data for the cert.
  string(8bit) der = read_string(buf, index[1]);

  int unknown1 = buf->read_int32();
  if (unknown1 != 1) {
    WERR("Certificate #%d has unknown1 != 1: 0x%08x!\n",
	 entry_no, unknown_one);
  }
  int version = buf->read_int32();	// X.509 version?

  string(8bit) name = read_string(buf, buf->read_int32());

  // The alternative name when not identical to name above,
  // is usually the email adress from the certifivate field
  // "X509v3 Subject Alternative Name".
  string(8bit) alternative_name = read_string(buf, buf->read_int32());

  if (name != alternative_name) {
    WERR("Cert #%d: Names 1 & 2 differ: %O != %O\n",
	 entry_no, name, alternative_name);
  }

  // Subject (eg CN="Developer ID Certification Authority"...).
  string(8bit) subject_der = read_string(buf, buf->read_int32());

  // Issued by (eg CN="Apple Root CA"...).
  // Usually the same as Subject (ie self-signed CA).
  string(8bit) issuer_der = read_string(buf, buf->read_int32());

  string(8bit) serial_no = read_string(buf, buf->read_int32());

  // This seems to be the certificate field "X509v3 subjectKeyIdentifier".
  string(8bit) subject_key_identifier = read_string(buf, buf->read_int32());

  // Unknown, often the same as subject_key_identifier.
  // Possibly the subjectKeyIdentifier of the certificate
  // at position @[unknown] above.
  string(8bit) der3 = read_string(buf, buf->read_int32());

  WERR("Cert #%d, Other: #%d, X.509 Version: %d\n"
       "Name: %O\n"
       "AltName: %O\n"
       "Subject: %O\n"
       "Issuer: %O\n"
       "Serial no: 0x%s\n"
       "Subject Key Identifier: 0x%s\n"
       "Unknown Key Identifier: 0x%s\n"
       "%s\n",
       entry_no, unknown,
       version,
       name, alternative_name,
       Standards.ASN1.Decode.secure_der_decode(subject_der),
       Standards.ASN1.Decode.secure_der_decode(issuer_der),
       String.string2hex(serial_no),
       String.string2hex(subject_key_identifier),
       String.string2hex(der3),
       Standards.PEM.build("CERTIFICATE", der));

  Standards.X509.TBSCertificate tbs =
    Standards.X509->decode_certificate(der);
  WERR("TBS: %O\n", tbs);

  // NB: We accept all certificates that decode correctly here.
  //     It is up to the user to actually validate that they
  //     are proper for use.
  if (tbs) {
    WERR("Decoded cert #%d: %O\n", entry_no, name);
    certs += ({ tbs });
  } else {
    werror("Warning: Failed to decode certificate %O (cert #%d)\n",
	   name, entry_no);
  }

  if (sizeof(buf)) {
    WERR("Remaining: %O (%d bytes)\n", buf, sizeof(buf));
  }
}

protected void parse_item(Stdio.Buffer buf)
{
  if (sizeof(buf) < 8) return;

  buf = read_hbuffer(buf);

  int type = buf->read_int32();

  WERR("Item type: %d (0x%08x)\n", type, type);

  array(int) index;
  int val;
  switch(type) {
  case 0x000c:		// Entire file.
    index = ({});
    int offset = 8;
    do {
      int i = buf->read_int32();
      index += ({ i });
      offset += 4;
    } while (offset < index[0]);

    WERR("Keychain Main Index: \n"
	 "%{  %d,\n%}",
	 index);
    break;
  case 0x80001000:	// X509 Certificate table.
    // Begins with three offsets followed by a NULL.
    index = ({});
    while (val = buf->read_int32()) {
      index += ({ val });
    }
    WERR("X509 Initial Index: \n"
	 "%{  %d,\n%}",
	 index);

    // Then there's an index table.
    int num_cert = buf->read_int32();
    WERR("Number of certificates: %d\n", num_cert);
    index = allocate(num_cert);
    for (int i = 0; i < num_cert; i++) {
      index[i] = buf->read_int32();
    }
    WERR("X509 Index: \n"
	 "%{  %d,\n%}",
	 index);

    // Then come the certificates.
    // ...
    WERR("Certificates:\n");
    for (int i = 0; i < num_cert; i++) {
      parse_cert(buf);
    }

    while (sizeof(buf)) {
      Stdio.Buffer sub = read_hbuffer(buf);
      WERR("Lookup table?: %O\n", sub);
    }
    break;
  default:
    WERR("Ignored item of type %d (0x%08x) (%d bytes).\n",
	 type, type, sizeof(buf));
    return;
  }
    
  while (sizeof(buf)) {
    parse_item(buf);
  }
}

void add_keychain(string(8bit) keychain)
{
  if (!has_prefix(keychain, "kych")) return;

  Stdio.Buffer buf = Stdio.Buffer(keychain);
  buf->read(4);

  /* File format:
   *
   * In general:
   *   32bit big-endian, holerith strings with 32bit length followed
   *   by data followed by padding to even 32bit.
   *
   * Starts with 0x14 bytes of header:
   *
   * Offset	Type	Content		Description
   * 0x00	STRING	"kych"		Magic cookie.
   * 0x04	INT32	0x10000		Flags?
   * 0x08	INT32	0x10		Unknown
   * 0x0c	INT32	0x14		Unknown
   * 0x10	INT32	0x00		Unknown
   *
   * This is followed by a block of data containing the
   * rest of the file.
   * 0x14	HSTRING	0x45150		Rest of file.
   * 0x18	INT32	0x0c		Block type
   * 0x1c	INT32   0x38		Points to 0x4c if relative to 0x14.
   * 0x20	INT32	0x364		D:o 0x378
   * 0x24	INT32	0x1b70		D:o 0x1b84
   * 0x28	INT32	0x4de8		D:o 0x4dfc
   * 0x2c	INT32	0x4e10		D:o 0x4e24
   * 0x30	INT32	0x4f88		D:o 0x4f9c
   * 0x34	INT32	0x5100		D:o 0x5114
   * 0x38	INT32	0x5278		D:o 0x528c
   * 0x3c	INT32	0x52f8		D:o 0x530c
   * 0x40	INT32	0x5418		D:o 0x542c
   * 0x44	INT32	0x54d8		D:o 0x54ec	Certs are found here.
   * 0x48	INT32	0x45068		D:o 0x4507c
   *
   * 0x4c	HSTRING	0x32c		Block of data.
   * 0x50	INT32	0x00		Block type
   *
   * 0x378	HSTRING	0x180c		Block of data.
   * 0x37c	INT32	0x01		Block type
   *
   * 0x1b84	HSTRING	0x3278		Block of data
   * 0x1b88	INT32	0x02		Block type
   *
   * 0x4dfc	HSTRING	0x28		Block of data
   * 0x4e00	INT32	0x06		Block type
   *
   * 0x4e24	HSTRING	0x178		Block of data
   * 0x4e28	INT32	0x0f		Block type
   *
   * 0x4f9c	HSTRING 0x178		Block of data
   * 0x4fa0	INT32	0x10		Block type
   */

  string header = buf->read(16);

  WERR("Header: %O\n", header);

  parse_item(buf);
}
