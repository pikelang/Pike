#pike __REAL_VERSION__

/* Old crypto module */
inherit _Crypto;

#if constant(Nettle.HashInfo)

private inherit Nettle;

//!
class HashAlgorithm
{
  inherit HashInfo;

  HashState `()();
  
  //! @decl string hash(string data)
  //!
  //!  Works as a shortcut for @expr{obj->update(data)->digest()@}.
  //!
  //! @note
  //!  The hash buffer will not be cleared before @[data] is added
  //!  to the buffer, so data added with calls to @[update] will be
  //!  prepended to the @[data].
  //!
  //! @seealso
  //!   @[update] and @[digest].
  string hash(string data)
  {
    return `()()->update(data)->digest();
  }
}

class MD5_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit MD5_Info;
  inherit HashAlgorithm;

  MD5_State `()() { return MD5_State(); }
}

MD5_Algorithm MD5 = MD5_Algorithm();

class SHA1_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit SHA1_Info;
  inherit HashAlgorithm;

  SHA1_State `()() { return SHA1_State(); }
}

SHA1_Algorithm SHA1 = SHA1_Algorithm();

class SHA256_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit SHA256_Info;
  inherit HashAlgorithm;

  SHA256_State `()() { return SHA256_State(); }
}

SHA256_Algorithm SHA256 = SHA256_Algorithm();

#endif /* constant(Nettle.HashInfo) */
