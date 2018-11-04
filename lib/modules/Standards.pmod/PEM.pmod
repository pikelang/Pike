#pike __REAL_VERSION__
#pragma strict_types

//! Support for parsing PEM-style messages, defined in @rfc{1421@}.
//! Encapsulation defined in @rfc{934@}.

//! Key derivation function used in PEM.
//!
//! @fixme
//!   Derived from OpenSSL. Is there any proper specification?
//!
//!   It seems to be related to PBKDF1 from @rfc{2898@}.
string(8bit) derive_key(string(8bit) password, string(8bit) salt, int bytes)
{
  string(8bit) out = "";
  string(8bit) hash = "";
  string(8bit) junk = password + salt;
  password = "CENSORED";
  while (sizeof(out) < bytes) {
    hash = Crypto.MD5.hash(hash + junk);
    out += hash;
  }
  return out[..bytes-1];
}

//! Decrypt a PEM body.
//!
//! @param dek_info
//!   @expr{"dek-info"@} header from the @[Message].
//!
//! @param body
//!   Encypted PEM body.
//!
//! @param password
//!   Decryption password.
//!
//! @returns
//!   Returns the decrypted body text.
string decrypt_body(string(8bit) dek_info, string(8bit) body, string(8bit) password)
{
  string(8bit) key = password;
  password = "CENSORED";
  if (!dek_info) return body;
  array(string) d = dek_info/",";
  if (sizeof(d) != 2) error("Unsupported DEK-Info.\n");
  string method = lower_case(String.trim(d[0]));
  Crypto.AES.CBC._Buffer cipher = ([
    "des-cbc": Crypto.DES.CBC.Buffer,
    "des-ede3-cbc": Crypto.DES3.CBC.Buffer,
    "aes-128-cbc": Crypto.AES.CBC.Buffer,
    "aes-192-cbc": Crypto.AES.CBC.Buffer,
    "aes-256-cbc": Crypto.AES.CBC.Buffer,
  ])[method];
  if (!cipher) error("Unsupported cipher suite.\n");
  int key_size = ([
    "des-cbc": 8,
    "aes-128-cbc": 16,
    "aes-256-cbc": 32,
  ])[method] || 24;
  string(8bit) iv = String.hex2string(d[1]);
  key = derive_key(key, iv[..7], key_size);
  Crypto.AES.CBC.Buffer.State decoder = cipher();
  decoder->set_decrypt_key(key);
  return decoder->unpad(iv + body, Crypto.PAD_PKCS7)[sizeof(iv)..];
}

//! Decrypt a PEM Message.
//!
//! @param body
//!   Fragment with encypted PEM body.
//!
//! @param password
//!   Decryption password.
//!
//! @returns
//!   Returns the decrypted body text.
string decrypt_fragment(Message m, string(8bit) pwd)
{
  // FIXME: Check proc-type = "4,ENCRYPTED"?
  string(8bit) dek = m->headers["dek-info"];
  if(!dek) return 0;;
  return decrypt_body(dek, m->body, pwd);
}

//! Represents a PEM-style message.
class Message
{
  //! Pre-encapsulation boundary string.
  //!
  //! Typically a string like @expr{"CERTIFICATE"@} or @expr{"PRIVATE KEY"@}.
  string pre;

  //! Post-encapsulation boundary string.
  //!
  //! Usually the same value as @[pre].
  string post;

  //! Encapsulated headers. If headers occur multiple times, they
  //! will be concatenated separated by delimiting NUL characters.
  mapping(string(8bit):string(8bit)) headers;

  //! The decode message body.
  string(8bit) body;

  //! Message trailer, like @rfc{4880@} checksum.
  string trailer;

  protected void create(string|array(string) data)
  {
    array(string) lines;
    if(stringp(data))
    {
       lines = data/"\n";
       while(sizeof(lines) && lines[-1]=="")
         lines = lines[..<1];
    }
    else
      lines = [array(string)]data;

    if( sscanf(lines[0], "%*[ \t]-----BEGIN %s-----", pre)!=2 )
      return;

    lines = lines[1..];
    if( sscanf(lines[-1], "%*[ \t]-----END %s-----", post)==2 )
    {
      lines = lines[..<1];
    }

    if( sizeof(lines[-1]) && lines[-1][0]=='=' )
    {
      trailer = MIME.decode_base64(lines[-1][1..]);
      lines = lines[..<1];
    }

    array res = MIME.parse_headers(lines*"\n");
    headers = [mapping(string(8bit):string(8bit))]res[0];
    body = MIME.decode_base64([string(8bit)]res[1]);
  }
}

//! The Messages class acts as an envelope for a PEM message file or
//! stream.
class Messages
{
  //! The fragments array contains the different message fragments, as
  //! Message objects for decoded messages and strings for
  //! non-messages or incomplete messages.
  array(string|Message) fragments = ({});

  //! This is a mapping from encapsulation boundary string to Message
  //! objects. All message objects and surrounding text will be
  //! listed, in order, in @[fragments].
  mapping(string:array(Message)) parts = ([]);

  //! A Messages object is created with the file or stream data.
  protected void create(string data)
  {
    array(string) current = ({});
    foreach(data/"\n";; string line)
    {
      if( has_prefix(line, "-----BEGIN ") )
      {
        // RFC1421 section 4.4p3 allows for multiple pre-EB
        fragments += process(current);
        current = ({ line });
      }
      else if( has_prefix(line, "-----END ") )
      {
        current += ({ line });
        fragments += process(current);
        current = ({});
      }
      else
        current += ({ line });
    }
    if(sizeof(current))
      fragments += process(current);

    foreach(fragments;; string|Message part)
      if(objectp(part))
      {
        Message msg = [object(Message)]part;
        parts[msg->pre] += ({ msg });
      }
  }

  protected array(string|Message) process(array(string) lines)
  {
    if( !sizeof(lines) || !has_prefix(lines[0], "-----BEGIN ") )
      return ({ lines*"\n" });
    return ({ Message(lines) });
  }

  //! Returns an array of all fragments with any of the given
  //! @[labels] in the boundy preamble.
  array(Message) get_fragments(multiset labels)
  {
    array(Message) ret = ({});
    foreach(fragments, string|Message m)
    {
      if(objectp(m) && labels[m->pre])
        ret += ({ m });
    }
    return ret;
  }

  //! Returns an array of the string bodies of all fragments with any
  //! of the given @[labels] in the boundy preamble.
  array(string) get_fragment_bodies(multiset labels)
  {
    return get_fragments(labels)->body;
  }

  //! Returns an array of all the bodies of @expr{"CERTIFICATE"@} and
  //! @expr{"X509 CERTIFICATE"@} fragments.
  array(string) get_certificates()
  {
    // FIXME: Support "TRUSTED CERTIFICATE" that contains additional
    // information (trusted uses, rejected uses, alias and key id).
    return get_fragment_bodies( (< "CERTIFICATE", "X509 CERTIFICATE" >) );
  }

  //! Convenience wrapper for @[get_certificates] that returns the
  //! first available certificate, or @expr{0@}.
  string get_certificate()
  {
    return get_certificates()[?0];
  }

  protected array(Message) low_get_private_keys()
  {
    return get_fragments( (< "RSA PRIVATE KEY", "DSA PRIVATE KEY",
                             "EC PRIVATE KEY", "ANY PRIVATE KEY" >) );
  }

  //! Returns an array of all the bodies of @expr{"RSA PRIVATE KEY"@},
  //! @expr{"DSA PRIVATE KEY"@}, @expr{"EC PRIVATE KEY"@} and
  //! @expr{"ANY PRIVATE KEY"@} fragments.
  array(string) get_private_keys()
  {
    return low_get_private_keys()->body;
  }

  //! Convenience wrapper for @[get_private_key] that returns the
  //! first available key, or @expr{0@}.
  string get_private_key()
  {
    return get_private_keys()[?0];
  }

  //! Returns the first key, decoded by the @[pwd] password.
  string get_encrypted_private_key(string(8bit) pwd)
  {
    Message m = low_get_private_keys()[?0];
    if(!m) return 0;
    return decrypt_fragment(m, pwd);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("Standards.PEM.Message%O", parts);
  }
}

//! Convenience function that decodes a PEM message containing only
//! one part, and returns it as a string. Returns @expr{0@} for indata
//! containing no or multiple parts.
string simple_decode(string pem)
{
  Messages m = Messages(pem);
  return sizeof(m->parts)==1 &&
    sizeof(values(m->parts)[0])==1 &&
    values(m->parts)[0][0]->body;
}

//! Creates a PEM message, wrapped to 64 character lines.
//!
//! @param tag
//!   The encapsulation boundary string.
//!
//! @param data
//!   The data to be encapsulated.
//!
//! @param headers
//!   Optional mapping containing encapsulated headers as name value
//!   pairs.
//!
//! @param checksum
//!   Optional checksum string, added as per @rfc{4880@}.
string build(string tag, string data,
             void|mapping(string:string) headers,
             void|string checksum)
{
  String.Buffer out = String.Buffer();
  out->add("-----BEGIN ", tag, "-----\n");
  if(headers)
  {
    if (headers["proc-type"]) {
      // The Proc-Type header MUST come first.
      // RFC 1421 4.6.1.1.
      out->add("Proc-Type: ", headers["proc-type"], "\n");
    }
    foreach(sort(indices([mapping(string:string)]headers)), string name) {
      if (name == "proc-type") continue;
      out->add(name, ": ", headers[name], "\n");
    }
  }

  foreach(MIME.encode_base64(data,1)/64.0, string line)
    out->add("\n", line);

  if( checksum )
    out->add("\n=", MIME.encode_base64(checksum, 1));
  out->add("\n-----END ", tag, "-----\n");
  return (string)out;
}
