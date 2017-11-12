
//! SCRAM, defined by @rfc{5802@}.

#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.Hash)

private .Hash H;  // hash object

private string(8bit) first, cnonce;

//! Step 0 in the SCRAM handshake, you need to agree
//! with the server on the hashfunction to be used.
//!
//! @param h
//!   The hash object on which the SCRAM object should base its
//!   operations. Typical input is @[Crypto.SHA256].
//!
//! @seealso
//!   @[client_first]
protected void create(.Hash h) {
  H = h;
}

private Crypto.MAC.State HMAC(string(8bit) key) {
  return H->HMAC(key, H->digest_size());
}

//! Step 1 in the SCRAM handshake.
//!
//! @param username
//!   The username to feed to the server.
//!
//! @returns
//!   The request to send to the server.
//!
//! @seealso
//!   @[client_final]
string(7bit) client_first(string username) {
  cnonce = MIME.encode_base64(random_string(18));
  return [string(7bit)](first = [string(8bit)]sprintf("n,,n=%s,r=%s",
    username == "" ? "" : Standards.IDNA.to_ascii(username, 1), cnonce));
}

//! Step 2 in the SCRAM handshake.
//!
//! @param pass
//!   The password to feed to the server.
//!
//! @param line
//!   The challenge received from the server to our @[client_first].
//!
//! @returns
//!   The response to send to the server.
//!
//! @seealso
//!   @[server_final]
string(7bit) client_final(string pass, Stdio.Buffer|string(8bit) line) {
  constant format = "r=%s,s=%s,i=%d";
  string r, salt;
  int iters;
  [r, salt, iters] = stringp(line)
    ? array_sscanf([string]line, format)
    : [array(string|int)](line->sscanf(format));
  if (iters >0 && has_prefix(r, cnonce)) {
    line = [string(8bit)]sprintf("c=biws,r=%s", r);
    r = sprintf("%s,r=%s,s=%s,i=%d,%s", first[3..], r, salt, iters, line);
    if (pass != "")
      pass = Standards.IDNA.to_ascii(pass);
    salt = MIME.decode_base64(salt);
    if (!(first = .SCRAM_get_salted_password(H, pass, salt, iters))) {
      first = [string(8bit)]H->pbkdf2(pass, salt, iters, H->digest_size());
      .SCRAM_set_salted_password(first, H, pass, salt, iters);
    }
    Crypto.MAC.State hmacfirst = HMAC(first);
    first = 0;                         // Free memory
    salt = hmacfirst("Client Key");
    salt = sprintf("%s,p=%s", line,
      MIME.encode_base64(salt ^ HMAC(H->hash(salt))(r)));
    cnonce = HMAC(hmacfirst("Server Key"))(r);
  } else
    salt = 0;
  return salt;
}

//! Final step 3 in the SCRAM handshake.  If we get this far, the
//! server has already verified that we supplied the correct credentials.
//! If this step fails, it means the server does not have our
//! credentials at all.
//!
//! @param line
//!   The verification received from the server to our @[client_final].
//!
//! @returns
//!   True if the server is valid, false if the server is invalid.
int(0..1) server_final(Stdio.Buffer|string line) {
  constant format = "v=%s";
  string(8bit) v;
  [v] = stringp(line)
    ? array_sscanf(line, format) : line->sscanf(format);
  return MIME.decode_base64(v) == cnonce;
}
