#pike 8.1

protected constant compat_required_methods = ({
  "block_size",
  "key_size",
  "set_encrypt_key",
  "set_decrypt_key",
  "crypt",
});

protected void compat_assert_is_crypto_object(Crypto.CipherState obj)
{
  foreach(compat_required_methods, string method) {
    if (!obj[method]) error("Object is missing identifier \"%s\"\n", method);
  }
}

//! @ignore
protected class CompatProxy
{
  inherit Crypto.BlockCipher;

  class _CBC {
    inherit ::this_program;

    class State
    {
      inherit ::this_program;
      //! @endignore

      //! @class CBC
      //!
      //! This class has moved to submodules of the respective ciphers.
      //!
      //! @deprecated BlockCipher.CBC

      protected Crypto.CipherState proxy_obj;

      protected Crypto.CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected void create(Crypto.CipherState|program(Crypto.CipherState) fun,
                            mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(Crypto.CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(Crypto.CipherState)]fun;
	} else {
	  error("Bad argument 1 to create(). Expected program|object|function.\n");
	}
	compat_assert_is_crypto_object(proxy_obj);
	::create();
      }

      //! @endclass

      //! @ignore
    }
  }

  class _Buffer {
    inherit ::this_program;

    class State
    {
      inherit ::this_program;

      //! @endignore

      //! @class Buffer
      //!
      //! This class has moved to submodules of the respective ciphers.
      //!
      //! @deprecated BlockCipher.Buffer

      protected Crypto.CipherState proxy_obj;

      protected Crypto.CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected void create(Crypto.CipherState|program(Crypto.CipherState) fun,
                            mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(Crypto.CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(Crypto.CipherState)]fun;
	} else {
	  error("Bad argument 1 to create(). Expected program|object|function.\n");
	}
	compat_assert_is_crypto_object(proxy_obj);
	::create();
      }

      //! @endclass

      //! @ignore
    }
  }
}

//! @endignore

protected CompatProxy compat_proxy = CompatProxy();

program(Crypto.CipherState) CBC = compat_proxy.CBC.State;

//! This class has moved to submodules of the respective ciphers.
program(Crypto.CipherState) Buffer = compat_proxy.Buffer.State;
