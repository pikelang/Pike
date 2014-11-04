#pike 8.1

inherit Crypto : pre;

protected constant compat_required_methods = ({
  "block_size",
  "key_size",
  "set_encrypt_key",
  "set_decrypt_key",
  "crypt",
});

protected void compat_assert_is_crypto_object(.CipherState obj)
{
  foreach(compat_required_methods, string method) {
    if (!obj[method]) error("Object is missing identifier \"%s\"\n", method);
  }
}

//! @ignore
protected class CompatProxy
{
  inherit .BlockCipher;

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

      protected .CipherState proxy_obj;

      protected .CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected void create(.CipherState|program(.CipherState) fun,
					   mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(.CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(.CipherState)]fun;
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

      protected CipherState proxy_obj;

      protected CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected void create(CipherState|program(CipherState) fun,
					   mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(CipherState)]fun;
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

program(CipherState) CBC = compat_proxy.CBC.State;

//! This class has moved to submodules of the respective ciphers.
program(CipherState) Buffer = compat_proxy.Buffer.State;
