#pike __REAL_VERSION__

/* Old crypto module */
inherit _Crypto;

#if constant(Nettle.HashInfo)

inherit Nettle;

class HashAlgorithm
{
  inherit HashInfo;

  HashState `();
  
  string hash(string data)
  {
    return `()()->update(data)->digest();
  }
}

class MD5_Algorithm
{
  /* NOTE: Depends on the order of INIT invocations.
   */
  inherit MD5_Info;
  inherit HashAlgorithm;

  MD5_State `()() { return MD5_State(); }
}

MD5_Algorithm MD5 = MD5_Algorithm();

#endif /* constant(Nettle.HashInfo) */
