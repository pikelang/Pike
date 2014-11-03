#pike 8.1

inherit Crypto.DH : pre;

class Parameters
{
  inherit pre::Parameters;

  //! Alias for @[q].
  //!
  //! @deprecated q
  Gmp.mpz `order()
  {
    return q;
  }

  //! Alias for @[q].
  //!
  //! @deprecated q
  void `order=(Gmp.mpz val)
  {
    q = val;
  }

}